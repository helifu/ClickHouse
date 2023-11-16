#include "CostBasedOptimizer.h"
#include <QueryCoordination/Optimizer/Memo.h>
#include <QueryCoordination/Optimizer/Tasks/OptimizeContext.h>
#include <QueryCoordination/Optimizer/Tasks/OptimizeGroup.h>
#include <QueryCoordination/Optimizer/Tasks/Scheduler.h>
#include <Common/Stopwatch.h>

namespace DB
{

QueryPlan CostBasedOptimizer::optimize(QueryPlan && plan, ContextPtr query_context)
{
    /// init memo by plan
    Memo memo(std::move(plan), query_context);
    PhysicalProperties initial_prop{.distribution = {.type = Distribution::Singleton}};

    /// init scheduler
    Scheduler scheduler(3000); /// TODO add to Settings
    OptimizeContextPtr optimize_context = std::make_shared<OptimizeContext>(memo, scheduler, query_context);

    /// push root task
    TaskContextPtr task_context = std::make_shared<TaskContext>(memo.rootGroup(), initial_prop, optimize_context);
    scheduler.pushTask(std::make_unique<OptimizeGroup>(task_context));

    scheduler.run();
    memo.dump();

    return memo.extractPlan();
}

}
