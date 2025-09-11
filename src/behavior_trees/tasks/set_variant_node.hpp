#ifndef SET_VALUE_NODE_HPP
#define SET_VALUE_NODE_HPP

#include "behavior_trees/behavior_tree_node.hpp"
#include "behavior_trees/behavior_tree_context.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
namespace hydrogen::behavior_trees {
class SetVariantNode : public BehaviorTreeNode {
    DECLARE_PIPELINE_NODE(SetVariantNode, BehaviorTreeNode);

    DECLARE_INPUT_PORT(value, Variant);
    DECLARE_OUTPUT_PORT(target, Variant);

    BEGIN_NODE_PORTS()
        INPUT_PORT(value)
        OUTPUT_PORT(target)
    END_NODE_PORTS()

protected:
    Result _execute(BehaviorTreeContext &p_context) const override {
        Variant value = GET_PORT(value);
        SET_PORT(target, value);
        return SUCCESS;
    }
public:
    DEFINE_GET_PORTS();
};
}

#endif