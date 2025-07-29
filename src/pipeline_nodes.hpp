//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "blackboard.hpp"
#include "rid_data.hpp"

namespace hydrogen {

using namespace godot;

#define MAKE_BLACKBOARD_ENTRY_NAME(entry_name)			\
	static const StringName &entry_name##_name() {		\
		const static StringName name(#entry_name, true);\
		return name;									\
	}

namespace pipeline_nodes {
	MAKE_BLACKBOARD_ENTRY_NAME(halting)
	MAKE_BLACKBOARD_ENTRY_NAME(error)
	MAKE_BLACKBOARD_ENTRY_NAME(blackboards)
	MAKE_BLACKBOARD_ENTRY_NAME(node_states)
	MAKE_BLACKBOARD_ENTRY_NAME(pipeline)
}

class PipelineNode;

class PipelineNode : public RidData {
	std::atomic<uint32_t> _bind_count = {0};

protected:
	friend class Pipeline;
	_FORCE_INLINE_ void _bind() { ++_bind_count; }
	_FORCE_INLINE_ void _unbind() { --_bind_count; }

	PipelineNode() = default;
public:
	virtual ~PipelineNode() = default;

	_FORCE_INLINE_ uint32_t get_bind_count() const { return _bind_count; }
	_FORCE_INLINE_ bool is_bound() const { return get_bind_count() > 0; }
};

struct IPipelineNodeState;

struct IPipelineNodeStateful {
	virtual ~IPipelineNodeStateful() = default;

	[[nodiscard]] virtual RID state_key() const = 0;

	[[nodiscard]] virtual IPipelineNodeState *create_state() const = 0;
	virtual void cleanup_state(IPipelineNodeState *p_state) const;

protected:
	IPipelineNodeStateful() = default;
};

struct IPipelineNodeState {
	virtual ~IPipelineNodeState() = default;

protected:
	IPipelineNodeState() = default;
};

class IPipelineNodeContainer {

protected:
	IPipelineNodeContainer() = default;

public:
	virtual ~IPipelineNodeContainer() = default;

	virtual uint32_t get_max_child_count() const = 0;
	virtual uint32_t get_child_count() const = 0;
	virtual PipelineNode *get_child(uint32_t p_index) const = 0;

	virtual bool add_child(PipelineNode *p_node) = 0;
	virtual bool remove_child(PipelineNode *p_node) = 0;
	virtual bool remove_child_at(uint32_t p_index) = 0;
	virtual bool swap_children(uint32_t p_first_index, uint32_t p_second_index) = 0;
	virtual bool insert_child(PipelineNode *p_node, uint32_t p_index) = 0;


	[[nodiscard]] virtual bool is_descendent(PipelineNode *p_node) const = 0;
	[[nodiscard]] virtual bool is_child(PipelineNode *p_node) const = 0;

	virtual PipelineNode *get_descendant(RID p_rid) = 0;
};

struct IPipelineNodeWrapper {

	[[nodiscard]] virtual PipelineNode *get_wrapped_node() const = 0;
	virtual bool set_wrapped_node(PipelineNode *p_node) = 0;

	virtual ~IPipelineNodeWrapper() = default;

protected:
	IPipelineNodeWrapper() = default;
};

} // hydrogen

#endif //BEHAVIORS_NODE_BASE_HPP
