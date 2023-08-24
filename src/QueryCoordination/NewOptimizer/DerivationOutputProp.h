#pragma once

#include <Interpreters/TableJoin.h>
#include <Processors/QueryPlan/UnionStep.h>
#include <Processors/QueryPlan/AggregatingStep.h>
#include <Processors/QueryPlan/ArrayJoinStep.h>
#include <Processors/QueryPlan/CreateSetAndFilterOnTheFlyStep.h>
#include <Processors/QueryPlan/CreatingSetsStep.h>
#include <Processors/QueryPlan/CubeStep.h>
#include <Processors/QueryPlan/DistinctStep.h>
#include <QueryCoordination/Exchange/ExchangeDataStep.h>
#include <Processors/QueryPlan/ExpressionStep.h>
#include <Processors/QueryPlan/ExtremesStep.h>
#include <Processors/QueryPlan/FillingStep.h>
#include <Processors/QueryPlan/FilterStep.h>
#include <Processors/QueryPlan/JoinStep.h>
#include <Processors/QueryPlan/LimitByStep.h>
#include <Processors/QueryPlan/LimitStep.h>
#include <Processors/QueryPlan/MergingAggregatedStep.h>
#include <Processors/QueryPlan/ReadFromMergeTree.h>
#include <Processors/QueryPlan/ReadNothingStep.h>
#include <Processors/QueryPlan/RollupStep.h>
#include <Processors/QueryPlan/ScanStep.h>
#include <Processors/QueryPlan/SortingStep.h>
#include <Processors/QueryPlan/TotalsHavingStep.h>
#include <Processors/QueryPlan/WindowStep.h>
#include <Processors/QueryPlan/IQueryPlanStep.h>
#include <QueryCoordination/NewOptimizer/PhysicalProperties.h>
#include <QueryCoordination/NewOptimizer/GroupNode.h>


namespace DB
{

class DerivationOutputProp
{
public:
    DerivationOutputProp(GroupNode & group_node_, const PhysicalProperties & required_prop_, const std::vector<PhysicalProperties> & children_prop_)
        : group_node(group_node_), required_prop(required_prop_), children_prop(children_prop_)
    {
    }

    PhysicalProperties derivationOutputProp(ReadFromMergeTree & /*step*/)
    {
        //    TODO sort_description by pk, DistributionType by distributed table
        PhysicalProperties res{.distribution = {.type = PhysicalProperties::DistributionType::Any}};
        return res;
    }

    PhysicalProperties derivationOutputProp(AggregatingStep & step)
    {
        if (step.isFinal())
        {
            /// TODO Hashed by keys
            if (children_prop[0].distribution.type == PhysicalProperties::DistributionType::Hashed)
            {
                return {.distribution = {.type = PhysicalProperties::DistributionType::Hashed}};
            }
        }

        return {.distribution = {.type = PhysicalProperties::DistributionType::Any}};
    }

    PhysicalProperties derivationOutputProp(MergingAggregatedStep & /*step*/)
    {
        /// TODO Hashed by keys
        return {.distribution = {.type = children_prop[0].distribution.type}};
    }

    PhysicalProperties derivationOutputProp(ExpressionStep & /*step*/)
    {
        return {.distribution = {.type = children_prop[0].distribution.type}};
    }

    PhysicalProperties derivationOutputProp(SortingStep & step)
    {
        const auto & sort_description = step.getSortDescription();
        PhysicalProperties properties{.distribution = {.type = children_prop[0].distribution.type}, .sort_description = sort_description};
        return properties;
    }


    PhysicalProperties derivationOutputProp(LimitStep & /*step*/)
    {
        return {.distribution = {.type = children_prop[0].distribution.type}};
    }

    PhysicalProperties derivationOutputProp(JoinStep & /*step*/)
    {
        return {.distribution = {.type = children_prop[0].distribution.type}};
    }

    PhysicalProperties derivationOutputProp(UnionStep & /*step*/)
    {
        PhysicalProperties res;
        return res;
    }

    PhysicalProperties derivationOutputProp(ExchangeDataStep & step)
    {
        return {.distribution = {.type = step.getDistributionType()}, .sort_description = step.getSortDescription()};
    }

    PhysicalProperties derivationOutputProp()
    {
        if (auto * scan_step = dynamic_cast<ReadFromMergeTree *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*scan_step);
        }
        else if (auto * agg_step = dynamic_cast<AggregatingStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*agg_step);
        }
        else if (auto * merge_agg_step = dynamic_cast<MergingAggregatedStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*merge_agg_step);
        }
        else if (auto * sort_step = dynamic_cast<SortingStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*sort_step);
        }
        else if (auto * join_step = dynamic_cast<JoinStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*join_step);
        }
        else if (auto * union_step = dynamic_cast<UnionStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*union_step);
        }
        else if (auto * exchange_step = dynamic_cast<ExchangeDataStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*exchange_step);
        }
        else if (auto * limit_step = dynamic_cast<LimitStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*limit_step);
        }
        else if (auto * expression_step = dynamic_cast<ExpressionStep *>(group_node.getStep().get()))
        {
            return derivationOutputProp(*expression_step);
        }
        else
        {
            return {.distribution = {.type = children_prop[0].distribution.type}};
        }
    }

private:
    GroupNode & group_node;
    PhysicalProperties required_prop;
    std::vector<PhysicalProperties> children_prop;
};

}
