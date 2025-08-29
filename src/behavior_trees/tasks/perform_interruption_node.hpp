#ifndef PERFORM_INTERRUPTION_NODE_HPP
#define PERFORM_INTERRUPTION_NODE_HPP

#include "../behavior_tree_node.hpp"
#include "behavior_trees/decorators/interrupter_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "pipelines/node_interfaces.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

class PerformInterruptionNode : public BehaviorTreeNode {
    DECLARE_PIPELINE_NODE(PerformInterruptionNode, BehaviorTreeNode);
    
    DECLARE_INPUT_PORT(desiredResult, Result, Result::SUCCESS);
    DECLARE_CONNECTION(interrupter, InterrupterNode);

protected:

    BEGIN_NODE_PORTS()
        INPUT_PORT(desiredResult)
    END_NODE_PORTS()
    
    BEGIN_CONNECTIONS()
        CONNECTION(interrupter)
    END_CONNECTIONS()

    Result _execute(BehaviorTreeContext &p_context) const override {
        if (likely(_interrupter != nullptr)) {
            Result desired = _get_port<Result>(p_context.blackboard(), desiredResult_name());
            _interrupter->set_result(p_context, desired);
            return SUCCESS;
        }
        else {
            return FAILURE;
        }
    }
public:
    DEFINE_CONNECTION(interrupter);

    DEFINE_GET_CONNECTIONS();
    DEFINE_GET_PORTS();

    BEGIN_SET_CONNECTION()
        SET_CONNECTION(interrupter)
    END_SET_CONNECTION()

    BEGIN_GET_CONNECTION()
        GET_CONNECTION(interrupter)
    END_GET_CONNECTION()
};
}

#endif