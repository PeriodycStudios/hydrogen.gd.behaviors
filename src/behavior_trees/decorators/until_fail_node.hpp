#ifndef UNTIL_FAIL_HPP
#define UNTIL_FAIL_HPP

#include "../decorator_node.hpp"
#include "behavior_trees/behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "pipelines/pipeline_node.hpp"

namespace hydrogen::behavior_trees {
class UntilFailNode : public DecoratorNode {
    DECLARE_PIPELINE_NODE(UntilFailNode);

protected:

    Result _execute(BehaviorTreeContext &p_context) const override {
        ERR_FAIL_NULL_V(_decorated_node, FAILURE);

        Result result = _decorated_node->execute(p_context);
        if (unlikely(result == FAILURE)) {
            return SUCCESS;
        }
        else {
            return RUNNING;
        }
    }

public:
    UntilFailNode() = default;
    ~UntilFailNode() override = default;
};
}

#endif