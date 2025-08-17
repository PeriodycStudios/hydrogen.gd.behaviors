//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "../rid_data.hpp"
#include "../name_helpers.hpp"
#include "blackboard.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <cstdint>
#include <functional>

namespace hydrogen::pipelines {

using namespace godot;

DEFINE_NAME_STATIC(error)

class IPipelineNode;

struct NodePortInfo {

	typedef std::function<void(Blackboard *)> DefaultSetter;

	const StringName name;
	const StringName type_name;
	const DefaultSetter set_default;

	template<typename T>
	static NodePortInfo create_port(const StringName &p_name, DefaultSetter p_setter) {
		return NodePortInfo(p_name, get_type_name_static<T>(), p_setter);
	}

private:
	friend class IPipelineNode;

	NodePortInfo(
		const StringName &p_name, 
		const StringName &p_type_name, 
		const DefaultSetter p_setter) : 
		name(p_name), 
		type_name(p_type_name), 
		set_default(p_setter) {}
};

class IPipelineGraph;

class IPipelineNode : public RidData {
protected:
	IPipelineNode() = default;

public:
	virtual ~IPipelineNode() = default;

	[[nodiscard]] virtual StringName get_type_name() const = 0;
	[[nodiscard]] virtual bool is_compatible(const IPipelineNode *p_other_node) const = 0;

	[[nodiscard]] virtual int32_t get_input_port_count() const = 0;
	[[nodiscard]] virtual int32_t get_output_port_count() const = 0;

	[[nodiscard]] virtual StringName get_input_port_type_name(int32_t p_port) const = 0;
	[[nodiscard]] virtual StringName get_output_port_type_name(int32_t p_port) const = 0;

	[[nodiscard]] virtual StringName get_input_port_name(int32_t p_port) const = 0;
	[[nodiscard]] virtual StringName get_output_port_name(int32_t p_port) const = 0;

	virtual void get_input_port_infos(Vector<NodePortInfo> &p_infos) const = 0;
	virtual void get_output_port_infos(Vector<NodePortInfo> &p_infos) const = 0;

	[[nodiscard]] virtual bool is_node_compatible(const IPipelineNode *p_other) const = 0;

	[[nodiscard]] virtual bool is_leaf() const = 0;
	[[nodiscard]] virtual bool is_branch() const = 0;
	[[nodiscard]] virtual bool is_vertex() const = 0;

	[[nodiscard]] virtual bool has_children() const = 0;
	virtual void get_children(Vector<IPipelineNode *> &p_children_buffer) = 0;
	virtual void get_children(Vector<const IPipelineNode *> &p_children_buffer) const = 0;

	virtual void get_descendants(Vector<IPipelineNode *> &p_descendents_buffer) = 0;
	virtual void get_descendants(Vector<const IPipelineNode *> &p_descendents_buffer) const = 0;
};

struct IPipelineNodeState;

struct IPipelineNodeStateful {
	virtual ~IPipelineNodeStateful() = default;

	[[nodiscard]] virtual RID state_key() const = 0;

	[[nodiscard]] virtual IPipelineNodeState *create_state() const = 0;

protected:
	IPipelineNodeStateful() = default;
};

struct IPipelineNodeState {
	virtual ~IPipelineNodeState() = default;

protected:
	IPipelineNodeState() = default;
};

typedef HashMap<RID, IPipelineNodeState *> NodeStateMap;


class IPipelineNodeComposite {
protected:
	IPipelineNodeComposite() = default;
public:
	virtual ~IPipelineNodeComposite() = default;

	virtual void add_child_node(const IPipelineNode *p_node) = 0;
	virtual	bool remove_child_node(const IPipelineNode *p_node) = 0;
	virtual bool remove_child_node_at(int64_t p_index) = 0;
	virtual void clear() = 0;
	[[nodiscard]] virtual bool is_empty() const = 0;
	[[nodiscard]] virtual const IPipelineNode *get_child_node(int64_t p_index) const = 0;
	virtual void set_child_node(int64_t p_index, const IPipelineNode *p_node) = 0;
	[[nodiscard]] virtual int64_t get_node_count() const = 0;

	virtual void resize(uint64_t p_size) = 0;
	virtual void resize_zeroed(uint64_t p_size) = 0;
	virtual void swap_child_nodes(uint64_t p_first_index, uint64_t p_second_index) = 0;

	virtual Error insert_child_node(int64_t p_pos, const IPipelineNode *p_node) = 0;
	virtual void append_child_nodes(const Vector<const IPipelineNode *> &p_nodes) = 0;
	virtual bool is_descendent_node(const IPipelineNode *p_node) = 0;
	virtual bool is_child_node(const IPipelineNode *p_node) = 0;
};

class IPipelineNodeDecorator {
protected:
	IPipelineNodeDecorator() = default;
public:
	virtual ~IPipelineNodeDecorator() = default;

	[[nodiscard]] virtual const IPipelineNode *get_pipeline_node() const = 0;
	virtual void set_pipline_node(const IPipelineNode *p_node) = 0;
};

class IPipelineGraph : public RidData {
protected:

	virtual RID _create_node(const StringName &p_node_type_name) = 0;
	virtual bool _destroy_node(RID p_node_id) = 0;

	friend class Pipeline;
	IPipelineGraph() = default;

public:
	virtual ~IPipelineGraph() = default;

	virtual void update(RID p_node) = 0;
	virtual void update(IPipelineNode *p_node) = 0;

	[[nodiscard]] virtual Vector<const IPipelineGraph *> get_pipeline_sub_graphs() const = 0;

	virtual Vector<const IPipelineNode*> get_pipeline_nodes() const;

	[[nodiscard]] virtual const IPipelineNode *get_pipeline_node(RID p_node_id) const = 0;
	[[nodiscard]] virtual IPipelineNode *get_pipeline_node(RID p_node_id) = 0;
	[[nodiscard]] virtual const IPipelineNode *get_pipeline_root() const = 0;
	[[nodiscard]] virtual IPipelineNode *get_pipeline_root() = 0;

	[[nodiscard]] virtual bool is_bound() const = 0;

	RID create_node(const StringName &p_node_type_name) {
		ERR_FAIL_COND_V_MSG(is_bound(), RID(), "Cannot create nodes while bound!");
		return _create_node(p_node_type_name);
	}

	bool destroy_node(RID p_node_id) {
		ERR_FAIL_COND_V_MSG(is_bound(), false, "Cannot destroy nodes while bound!");
		return _destroy_node(p_node_id);
	}

	virtual bool set_root_id(RID p_node_id) = 0;
	[[nodiscard]] virtual RID get_root_id() const = 0;

	typedef std::function<bool(const IPipelineNode *)> PipelineNodePredicate;

	virtual void query_pipeline_node(RID p_node_id, Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const = 0;
	virtual void query_pipeline_node(RID p_node_id, Vector<IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) = 0;

	virtual void query_pipeline_nodes(Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const = 0;
	virtual void query_pipeline_nodes(Vector<IPipelineNode*> &p_nodes, PipelineNodePredicate p_predicate) = 0;

	[[nodiscard]] virtual Vector<RID> get_unrooted_nodes() const = 0;
	[[nodiscard]] virtual Vector<RID> get_rooted_nodes() const = 0;
};

class IPipelineNodeSubGraph {
protected:
	IPipelineNodeSubGraph() = default;
public:
	virtual ~IPipelineNodeSubGraph() = default;

	[[nodiscard]] virtual const IPipelineGraph *get_pipeline_sub_graph() const = 0;
	virtual void set_pipeline_sub_graph(IPipelineGraph *p_graph) = 0;
};
} // hydrogen

#endif //BEHAVIORS_NODE_BASE_HPP
