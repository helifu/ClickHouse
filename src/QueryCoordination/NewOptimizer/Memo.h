#pragma once

#include <QueryCoordination/NewOptimizer/Group.h>
#include <QueryCoordination/NewOptimizer/SubQueryPlan.h>
#include <Processors/QueryPlan/QueryPlan.h>

namespace DB
{

class Memo
{
public:
    Memo(QueryPlan && plan);

    void addPlanNodeToGroup(const QueryPlan::Node & node, Group & target_group);

    Group & buildGroup(const QueryPlan::Node & node);

    Group & buildGroup(const QueryPlan::Node & node, const std::vector<Group *> children_groups);

    void dump(Group * group);

    void transform();

    void transform(Group * group, std::unordered_map<Group *, std::vector<SubQueryPlan>> & group_transformed_node);

    void enforce();

    void enforce(Group * group);

    void derivationProperties();

    void derivationProperties(Group * group);

private:
    std::vector<Group> groups;

    Group * root_group;
};

}
