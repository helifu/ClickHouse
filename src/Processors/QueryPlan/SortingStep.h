#pragma once
#include <Processors/QueryPlan/ITransformingStep.h>
#include <Core/SortDescription.h>
#include <QueryPipeline/SizeLimits.h>
#include <Interpreters/TemporaryDataOnDisk.h>

namespace DB
{

/// Sort data stream
class SortingStep : public ITransformingStep
{
public:
    /// Only work on query coordination
    enum Phase
    {
        Final,
        Preliminary,
        Unknown,
    };

    enum class Type
    {
        Full,
        FinishSorting,
        MergingSorted,
    };

    struct Settings
    {
        size_t max_block_size;
        SizeLimits size_limits;
        size_t max_bytes_before_remerge = 0;
        double remerge_lowered_memory_bytes_ratio = 0;
        size_t max_bytes_before_external_sort = 0;
        TemporaryDataOnDiskScopePtr tmp_data = nullptr;
        size_t min_free_disk_space = 0;

        explicit Settings(const Context & context);
        explicit Settings(size_t max_block_size_);
    };

    /// Full
    SortingStep(
        const DataStream & input_stream,
        SortDescription description_,
        UInt64 limit_,
        const Settings & settings_,
        bool optimize_sorting_by_input_stream_properties_);

    /// Full with partitioning
    SortingStep(
        const DataStream & input_stream,
        const SortDescription & description_,
        const SortDescription & partition_by_description_,
        UInt64 limit_,
        const Settings & settings_,
        bool optimize_sorting_by_input_stream_properties_);

    /// FinishSorting
    SortingStep(
        const DataStream & input_stream_,
        SortDescription prefix_description_,
        SortDescription result_description_,
        size_t max_block_size_,
        UInt64 limit_);

    /// MergingSorted
    SortingStep(
        const DataStream & input_stream,
        SortDescription sort_description_,
        size_t max_block_size_,
        UInt64 limit_ = 0,
        bool always_read_till_end_ = false
    );

    String getName() const override { return "Sorting"; }

    void transformPipeline(QueryPipelineBuilder & pipeline, const BuildQueryPipelineSettings &) override;

    void describeActions(JSONBuilder::JSONMap & map) const override;
    void describeActions(FormatSettings & settings) const override;

    UInt64 getLimit() const { return limit; }
    /// Add limit or change it to lower value.
    void updateLimit(size_t limit_);

    const SortDescription & getSortDescription() const { return result_description; }

    void convertToFinishSorting(SortDescription prefix_description);

    Type getType() const { return type; }
    const Settings & getSettings() const { return sort_settings; }

    static void fullSortStreams(
        QueryPipelineBuilder & pipeline,
        const Settings & sort_settings,
        const SortDescription & result_sort_desc,
        UInt64 limit_,
        bool skip_partial_sort = false);

    Phase getPhase() const { return phase; }

    void setPhase(Phase phase_) { phase = phase_; }

    StepType stepType() const override
    {
        return Sort;
    }

    std::shared_ptr<SortingStep> clone()
    {
        switch (type)
        {
            case Type::Full:
                return std::make_shared<SortingStep>(input_streams[0], result_description, limit, sort_settings, optimize_sorting_by_input_stream_properties);
            case Type::FinishSorting:
                return std::make_shared<SortingStep>(input_streams[0], prefix_description, result_description, sort_settings.max_block_size, limit);
            case Type::MergingSorted:
                return std::make_shared<SortingStep>(input_streams[0], result_description, sort_settings.max_block_size, limit, always_read_till_end);
        }
    }

    const SortDescription & getPrefixDescription() const
    {
        return prefix_description;
    }

private:
    void scatterByPartitionIfNeeded(QueryPipelineBuilder& pipeline);
    void updateOutputStream() override;

    static void mergeSorting(
        QueryPipelineBuilder & pipeline,
        const Settings & sort_settings,
        const SortDescription & result_sort_desc,
        UInt64 limit_);

    void mergingSorted(
        QueryPipelineBuilder & pipeline,
        const SortDescription & result_sort_desc,
        UInt64 limit_);
    void finishSorting(
        QueryPipelineBuilder & pipeline,
        const SortDescription & input_sort_desc,
        const SortDescription & result_sort_desc,
        UInt64 limit_);
    void fullSort(
        QueryPipelineBuilder & pipeline,
        const SortDescription & result_sort_desc,
        UInt64 limit_,
        bool skip_partial_sort = false);

    Type type;

    SortDescription prefix_description;
    const SortDescription result_description;

    SortDescription partition_by_description;

    UInt64 limit;
    bool always_read_till_end = false;

    Settings sort_settings;

    const bool optimize_sorting_by_input_stream_properties = false;

    Phase phase = Phase::Unknown;
};

}
