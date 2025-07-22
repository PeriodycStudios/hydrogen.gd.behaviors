#ifndef BEHAVIOR_TREES_HPP
#define BEHAVIOR_TREES_HPP

#include "behaviors_node_base.hpp"
#include "pipeline.hpp"


namespace hydrogen {

using namespace godot;

class BehaviorTreeTaskNode : public BehaviorsNodeBase {

public:
	enum Result {
		SUCCESS = 0,
		FAILURE = 1,
		RUNNING = 2,
	};

	virtual Result execute(Blackboard *blackboard) const = 0;
};

class BehaviorTree final : public Pipeline {

	Blackboard *state_blackboard;
	Vector<Blackboard *> blackboards = {};
	Vector<BehaviorTreeTaskNode *> nodes = {};

	static const StringName &blackboards_name() {
		const static StringName name{ "__blackboards" };
		return name;
	}

	static const StringName &nodes_name() {
		const static StringName name{"__nodes"};
		return name;
	}

	static const StringName &last_result_name() {
		const static StringName name{ "__last_result" };
		return name;
	}

	_FORCE_INLINE_ BehaviorTreeTaskNode *get_task_root() { return dynamic_cast<BehaviorTreeTaskNode *>(get_root()); }

	_FORCE_INLINE_ Blackboard *get_current_blackboard() const { return blackboards[blackboards.size() - 1]; }
	_FORCE_INLINE_ BehaviorTreeTaskNode *get_current_node() const { return nodes[nodes.size() - 1]; }

public:

	static void register_types();


	BehaviorTree(Blackboard *p_blackboard, BehaviorTreeTaskNode *p_root);
	~BehaviorTree() override;

	void execute() override;
	void halt() override;
	bool is_fully_halted() const override;
	void clear_halt() override;
	String get_error() const override;

	_FORCE_INLINE_ const BehaviorTreeTaskNode *get_task_root() const { return dynamic_cast<const BehaviorTreeTaskNode *>(get_root()); }
};

}

#endif