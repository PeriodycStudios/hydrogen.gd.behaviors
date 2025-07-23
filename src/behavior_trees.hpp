#ifndef BEHAVIOR_TREES_HPP
#define BEHAVIOR_TREES_HPP

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/pair.hpp>

#include "behaviors_node_base.hpp"
#include "pipeline.hpp"

#define MAKE_BEHAVIOR_TREE_ENTRY_NAME(entry_name)		\
	static const StringName &entry_name##_name() {		\
		const static StringName name(#entry_name, true);\
		return name;									\
	}


namespace hydrogen {

using namespace godot;

class BehaviorTreeTaskNode : public BehaviorsNodeBase {

protected:
	friend class BehaviorTree;
	uint64_t bind_count = 0;

public:
	enum Result {
		SUCCESS = 0,
		FAILURE = 1,
		RUNNING = 2,
	};

	virtual Result execute(Blackboard *p_blackboard) const = 0;

	[[nodiscard]] virtual bool is_leaf() const = 0;
	[[nodiscard]] virtual bool is_descendent(BehaviorTreeTaskNode *p_node) const = 0;
	[[nodiscard]] virtual bool is_child(BehaviorTreeTaskNode *p_node) const = 0;

	virtual void collect_descendents(Vector<BehaviorTreeTaskNode*> &descendents) const = 0;

	virtual BehaviorTreeTaskNode *get_node_by_id(RID p_rid) = 0;

	[[nodiscard]] _FORCE_INLINE_ uint64_t get_bind_count() const { return bind_count; }

	MAKE_BEHAVIOR_TREE_ENTRY_NAME(blackboards)
	MAKE_BEHAVIOR_TREE_ENTRY_NAME(nodes)
	MAKE_BEHAVIOR_TREE_ENTRY_NAME(error)
	MAKE_BEHAVIOR_TREE_ENTRY_NAME(halting)
	MAKE_BEHAVIOR_TREE_ENTRY_NAME(last_result)
	MAKE_BEHAVIOR_TREE_ENTRY_NAME(node_states)
};

struct IStatefulBehaviorTreeTaskNode;

struct IBehaviorTreeTaskState {

protected:
	explicit IBehaviorTreeTaskState(const IStatefulBehaviorTreeTaskNode *p_owner) : owner{p_owner} {};

public:
	const IStatefulBehaviorTreeTaskNode *owner;
	virtual ~IBehaviorTreeTaskState() = default;
};

struct IStatefulBehaviorTreeTaskNode {
	IStatefulBehaviorTreeTaskNode() = default;
	virtual ~IStatefulBehaviorTreeTaskNode() = default;

	virtual IBehaviorTreeTaskState *create_state_value() const = 0;
	virtual bool reset_state_value(IBehaviorTreeTaskState *p_state) const {
		return p_state != nullptr && p_state->owner == this;
	}
};

struct BehaviorTreeTaskNodeHasher {
	static _FORCE_INLINE_ uint32_t hash(BehaviorTreeTaskNode *p_node) {
		return p_node ? HashMapHasherDefault::hash(p_node->get_self()) :
		HashMapHasherDefault::hash(RID());
	}
};

struct BehaviorTreeTaskNodeComparator {
	static _FORCE_INLINE_ bool compare(BehaviorTreeTaskNode *p_node_a, BehaviorTreeTaskNode *p_node_b) {
		const RID rid_a = p_node_a ? p_node_a->get_self() : RID();
		const RID rid_b = p_node_b ? p_node_b->get_self() : RID();
		return rid_a == rid_b;
	}
};

class BehaviorTree final : public Pipeline {

	friend class BehaviorTreeTaskNode;

	typedef HashSet<BehaviorTreeTaskNode *, BehaviorTreeTaskNodeHasher, BehaviorTreeTaskNodeComparator> NodeSet;
	typedef HashMap<RID, IBehaviorTreeTaskState *> NodeStateMap;

	Blackboard *state_blackboard;
	Vector<Pair<RID, Blackboard *>> blackboards_stack = {};
	Vector<BehaviorTreeTaskNode *> nodes_stack = {};
	NodeSet bound_nodes = {};
	NodeStateMap node_states = {};

	[[nodiscard]] _FORCE_INLINE_ BehaviorTreeTaskNode *get_task_root() { return dynamic_cast<BehaviorTreeTaskNode *>(get_root()); }

	[[nodiscard]] _FORCE_INLINE_ const Pair<RID, Blackboard *> &get_current_blackboard() const { return blackboards_stack[blackboards_stack.size() - 1]; }
	[[nodiscard]] _FORCE_INLINE_ BehaviorTreeTaskNode *get_current_node() const { return nodes_stack[nodes_stack.size() - 1]; }

	void _update_dirty() override;

public:

	static void register_types();

	BehaviorTree(Blackboard *p_blackboard, BehaviorTreeTaskNode *p_root);
	~BehaviorTree() override;

	void execute() override;
	void halt() override;
	[[nodiscard]] bool is_fully_halted() const override;
	void clear_halt() override;
	String get_error() const override;

	[[nodiscard]] _FORCE_INLINE_ bool is_node_bound(BehaviorTreeTaskNode *p_node) const { return bound_nodes.has(p_node); }

	[[nodiscard]] _FORCE_INLINE_ const BehaviorTreeTaskNode *get_task_root() const { return dynamic_cast<const BehaviorTreeTaskNode *>(get_root()); }
};

}

#endif