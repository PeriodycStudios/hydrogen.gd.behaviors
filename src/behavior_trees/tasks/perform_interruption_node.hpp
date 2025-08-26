#ifndef PERFORM_INTERRUPTION_NODE_HPP
#define PERFORM_INTERRUPTION_NODE_HPP

#include "../behavior_tree_node.hpp"
namespace hydrogen::behavior_trees {
class PerformInterruptionNode : public BehaviorTreeNode {
    /*
    interrupter

    desiredResult

    func execute() 
        interrupter.setResult(desiredResult)
        return SUCCCESS
    */
};
}

#endif