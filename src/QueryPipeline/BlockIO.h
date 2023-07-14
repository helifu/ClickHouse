#pragma once

#include <functional>
#include <QueryPipeline/QueryPipeline.h>
#include <QueryCoordination/Pipelines//Pipelines.h>


namespace DB
{

class ProcessListEntry;

struct BlockIO
{
    BlockIO() = default;
    BlockIO(BlockIO &&) = default;

    BlockIO & operator= (BlockIO && rhs) noexcept;
    ~BlockIO();

    BlockIO(const BlockIO &) = delete;
    BlockIO & operator= (const BlockIO & rhs) = delete;

    std::shared_ptr<ProcessListEntry> process_list_entry;

    QueryPipeline pipeline;

    /// for query coordination
    Pipelines pipelines;

    /// Callbacks for query logging could be set here.
    std::function<void(QueryPipeline &)> finish_callback;
    std::function<void(bool)> exception_callback;

    /// When it is true, don't bother sending any non-empty blocks to the out stream
    bool null_format = false;

    void onFinish();
    void onException();
    void onCancelOrConnectionLoss();

    /// Set is_all_data_sent in system.processes for this query.
    void setAllDataSent() const;

private:
    void reset();
};

}
