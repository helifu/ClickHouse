#include <DataTypes/DataTypesNumber.h>
#include <QueryCoordination/Exchange/ExchangeDataSink.h>
#include <QueryCoordination/Exchange/ExchangeManager.h>
#include <Common/logger_useful.h>

namespace DB
{

void ExchangeDataSink::Channel::prepareSendData(const ExchangeDataRequest & prepare_request)
{
    if (!is_local)
        connection->sendExchangeData(prepare_request);
    else if (is_local && !local_receiver)
        local_receiver = ExchangeManager::getInstance().findExchangeDataSource(prepare_request);
}

void ExchangeDataSink::Channel::sendData(const Block & block)
{
    if (is_local)
        local_receiver->receive(block);
    else
        connection->sendData(block, "", false);
}

void ExchangeDataSink::onStart()
{
    if (!was_begin_sent)
    {
        LOG_DEBUG(log, "ExchangeDataSink start for request {}", request.toString());
        for (auto & channel : channels)
            channel.prepareSendData(request);

        was_begin_sent = true;
    }
}

void ExchangeDataSink::calculateKeysPositions()
{
    const auto & sample = getPort().getHeader();
    keys_positions.resize(output_partition.keys_size);
    for (size_t i = 0; i < output_partition.keys_size; ++i)
        keys_positions[i] = sample.getPositionByName(output_partition.keys[i]);
}

void ExchangeDataSink::calcKeysPositions()
{
    const auto & sample = getPort().getHeader();
    keys_positions.resize(output_distribution.keys.size());
    for (size_t i = 0; i < output_distribution.keys.size(); ++i)
        keys_positions[i] = sample.getPositionByName(output_distribution.keys[i]);
}

void ExchangeDataSink::consume(Chunk chunk)
{
    size_t rows = chunk.getNumRows();
    num_rows += rows;

    auto block = getPort().getHeader().cloneWithColumns(chunk.detachColumns());

    /// E.g select count() from distribute_table where name='a'
    /// Filter has empty header. ExchangeData has empty header. But there is real data, and in order to prevent the upper layer from receiving an empty block, we construct a fake header.
    if (unlikely(rows && !block))
    {
        auto column = ColumnUInt64::create();
        column->insertValue(rows);
        block.insert({column->getPtr(), std::make_shared<DataTypeUInt64>(), "_empty_header_rows"});
    }

    if (auto chunk_info = chunk.getChunkInfo())
    {
        if (const auto * agg_info = typeid_cast<const AggregatedChunkInfo *>(chunk_info.get()))
        {
            block.info.bucket_num = agg_info->bucket_num;
            block.info.is_overflows = agg_info->is_overflows;
        }
    }

    if (output_distribution.type == Distribution::Singleton || output_distribution.type == Distribution::Replicated)
    {
        for (auto & channel : channels)
            channel.sendData(block);
    }
    else if (output_distribution.type == Distribution::Hashed)
    {
        if (block.info.bucket_num > -1 && output_distribution.distribution_by_buket_num)
        {
            size_t which_channel = block.info.bucket_num % channels.size();
            channels[which_channel].sendData(block);
        }
        else
        {
            // normal shaffle
            std::vector<MutableColumns> mutable_columns(channels.size());

            for (size_t i = 0; i < channels.size(); ++i)
                mutable_columns[i] = block.cloneEmptyColumns();

            std::vector<SipHash> siphashs(rows);
            for (size_t keys_position : keys_positions)
            {
                const auto column = block.getColumns()[keys_position];
                for (size_t i = 0; i < rows; ++i)
                    column->updateHashWithValue(i, siphashs[i]);
            }

            for (size_t i = 0; i < rows; ++i)
            {
                size_t which_channel = siphashs[i].get64() % channels.size();

                auto & columns = mutable_columns[which_channel];
                auto src_columns = block.getColumns();
                for (size_t j = 0; j < block.columns(); ++j)
                    columns[j]->insertFrom(*src_columns[j], i);
            }

            for (size_t i = 0; i < channels.size(); ++i)
            {
                if (!mutable_columns[i].empty() && !mutable_columns[i][0]->empty())
                {
                    Block block_for_send = block.cloneEmpty();
                    block_for_send.setColumns(std::move(mutable_columns[i]));
                    channels[i].sendData(block_for_send);
                }
            }
        }
    }
}

void ExchangeDataSink::onFinish()
{
    LOG_DEBUG(log, "ExchangeDataSink finish for request {}", request.toString());

    for (auto & channel : channels)
        channel.sendData(Block());
}

}
