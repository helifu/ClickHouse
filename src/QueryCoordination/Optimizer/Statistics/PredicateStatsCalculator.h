#pragma once

#include <Interpreters/ActionsDAG.h>
#include <QueryCoordination/Optimizer/Statistics/Statistics.h>

namespace DB
{

class PredicateNodeVisitor : public ActionNodeVisitor<ActionNodeStatistics, std::unordered_map<const ActionsDAG::Node *, ActionNodeStatistics>>
{
public:
    using Base = ActionNodeVisitor<ActionNodeStatistics, std::unordered_map<const ActionsDAG::Node *, ActionNodeStatistics>>;
    using VisitContext = std::unordered_map<const ActionsDAG::Node *, ActionNodeStatistics>;

    PredicateNodeVisitor() : log(&Poco::Logger::get("PredicateNodeVisitor")) {}

    ActionNodeStatistics visit(const ActionsDAGPtr actions_dag_ptr, ContextType & context) override;
    ActionNodeStatistics visit(const ActionsDAG::Node * node, ContextType & context) override;

    ActionNodeStatistics visitChildren(const ActionsDAG::Node * node, ContextType & context) override;

    ActionNodeStatistics visitInput(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitColumn(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitAlias(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitArrayJoin(const ActionsDAG::Node * node, ContextType & context) override;

    /// functions
    ActionNodeStatistics visitAnd(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitOr(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitNot(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitIn(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitEqual(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitNotEqual(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitGreater(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitGreaterOrEqual(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitLess(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitLessOrEqual(const ActionsDAG::Node * node, ContextType & context) override;
    ActionNodeStatistics visitOtherFuncs(const ActionsDAG::Node * node, ContextType & context) override;

private:
    Poco::Logger * log;
};


class PredicateStatsCalculator
{
public:
    static Statistics calculateStatistics(
        const ActionsDAGPtr & predicates, const String & filter_node_name, const Statistics & input, const Names & output_columns = {});
};

}
