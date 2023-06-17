#include <Processors/QueryPlan/ExchangeDataStep.h>
#include <QueryPipeline/Pipe.h>
#include <QueryPipeline/QueryPipelineBuilder.h>
#include <QueryCoordination/ExchangeDataReceiver.h>
#include <QueryPipeline/StreamLocalLimits.h>

namespace DB
{

void ExchangeDataStep::initializePipeline(QueryPipelineBuilder & pipeline, const BuildQueryPipelineSettings & /*settings*/)
{
    Pipes pipes;

    for (const auto & source : sources)
    {
        auto receiver = std::make_shared<ExchangeDataReceiver>(output_stream.value(), plan_id, source);
        pipes.emplace_back(receiver);
    }

    auto pipe = Pipe::unitePipes(std::move(pipes));

    for (const auto & processor : pipe.getProcessors())
        processor->setStorageLimits(storage_limits);

    /// TODO if has merge sort info, add MergeSortingTransform
    pipe.resize(1);
    pipeline.init(std::move(pipe));
}

}
