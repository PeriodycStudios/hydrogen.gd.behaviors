#include "behavior_server.hpp"
#include "behavior_trees/behavior_tree_graph.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "behavior_trees/behavior_trees.hpp"

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/core/class_db.hpp>

#ifdef TESTS_ENABLED
#include "test_runner.hpp"
#endif


namespace hydrogen {

using namespace behavior_trees;

BehaviorServer *BehaviorServer::_singleton = nullptr;

void BehaviorServer::_bind_methods() {}

BehaviorServer *BehaviorServer::get_singleton() {
	return _singleton;
}

BehaviorServer::BehaviorServer() {
	_singleton = this;
}

BehaviorServer::~BehaviorServer() {
	_singleton = nullptr;
}

#define HANDLE_CLEANUP(type, initials, name)												\
	const static auto initials##_cleanup = [this](type *item) { _##name##_erase(item); };	\
	_free_ptr_resource<type>(_##name##_owner, _##name##_mutex, p_rid, initials##_cleanup);	\

void BehaviorServer::free_rid(RID p_rid) {
	if (_pipeline_owner.owns(p_rid)) {
		HANDLE_CLEANUP(Pipeline, bt, pipeline)
	}
	else if (_graph_owner.owns(p_rid)) {
		HANDLE_CLEANUP(IPipelineGraph, btg, graph)
	}
	else if (_blackboard_owner.owns(p_rid)) {
		HANDLE_CLEANUP(Blackboard, bb, blackboard)
	}
	else {
		ERR_FAIL_MSG("Invalid ID.");
	}
}

#undef HANDLE_CLEANUP

#define BLACKBOARDS_LOCK_V(fail_result) LOCK_ONE_V(_blackboard_mutex, fail_result)
#define BLACKBOARDS_LOCK LOCK_ONE(_blackboard_mutex)
	
#define PIPELINES_LOCK_V(fail_result) LOCK_ONE_V(_pipeline_mutex, fail_result)
#define PIPELINES_LOCK LOCK_ONE(_pipeline_mutex)

#define GRAPHS_LOCK_V(fail_result) LOCK_ONE_V(_graph_mutex, fail_result)
#define GRAPHS_LOCK LOCK_ONE(_graph_mutex)

RID BehaviorServer::blackboard_create() {
	RID rid = _blackboard_create_helper();

	_blackboard_emit_created(rid);

	return rid;
}

RID BehaviorServer::graph_create(GraphType p_graph_type) {

	switch (p_graph_type) {
	case BEHAVIOR_TREE:
		return _graph_create_helper<BehaviorTreeGraph>();
	default:
		ERR_FAIL_V(RID());
	}
}

RID BehaviorServer::pipeline_create(GraphType p_graph_type, RID p_blackboard, RID p_graph) {

	switch (p_graph_type) {
	case BEHAVIOR_TREE:
		return _pipeline_create_helper<BehaviorTree>(p_blackboard, p_graph);
	default:
		ERR_FAIL_V(RID());
	}
}

#undef SELECT_CREATE

RID BehaviorServer::agent_create() {
	return {};
}

Error BehaviorServer::init() {
	_blackboard_mutex = memnew(std::mutex);
	_pipeline_mutex = memnew(std::mutex);
	_graph_mutex = memnew(std::mutex);

 	return OK;
}

template<typename T>
void _cleanup_owned(RID_PtrOwner<T> &p_owner) {
	TightLocalVector<RID> rids = {};
	rids.resize(p_owner.get_rid_count());
	p_owner.fill_owned_buffer(rids.ptr());
	for(RID rid : rids) {
		T *bb = p_owner.get_or_null(rid);
		p_owner.free(rid);
		memdelete(bb);
	}
}

void BehaviorServer::finish() {

	_blackboard_parents_to_children.clear();
	{
		// destroy in reverse order of dependencies
		LOCK_THREE(_pipeline_mutex, _graph_mutex, _blackboard_mutex);
		_cleanup_owned(_pipeline_owner);
		_cleanup_owned(_graph_owner);
		_cleanup_owned(_blackboard_owner);
	}
	memdelete(_blackboard_mutex);
	memdelete(_pipeline_mutex);
	memdelete(_graph_mutex);
}

// ---- Blackboard ----

void BehaviorServer::_blackboard_add_child(RID parent, RID child) {
	BLACKBOARDS_LOCK;
	const auto iter = _blackboard_parents_to_children.find(parent);
	if (iter == _blackboard_parents_to_children.end()) {
		_blackboard_parents_to_children[parent] = LocalVector{child};
	}
	else {
		LocalVector<RID> &children = iter->value;
		children.push_back(child);
	}
}

void BehaviorServer::_blackboard_remove_child(RID parent, RID child) {
	BLACKBOARDS_LOCK;
	const auto iter = _blackboard_parents_to_children.find(parent);
	if (iter == _blackboard_parents_to_children.end()) {
		return;
	}

	LocalVector<RID> &children = iter->value;
	children.erase(child);
	if (children.is_empty()) {
		_blackboard_parents_to_children.remove(iter);
	}
}

void BehaviorServer::_blackboard_erase(Blackboard *blackboard) {
	BLACKBOARDS_LOCK;
	const RID self = blackboard->get_self();

	if (const Blackboard *parent = blackboard->get_parent()) {
		const RID parent_rid = parent->get_self();
		const auto iter = _blackboard_parents_to_children.find(parent_rid);
		if (iter != _blackboard_parents_to_children.end()) {
			LocalVector<RID> &children = iter->value;
			children.erase(self);

			if (children.is_empty()) {

				_blackboard_parents_to_children.remove(iter);
			}
		}
	}

	LocalVector<RID> children = {};
	const auto iter = _blackboard_parents_to_children.find(self);
	if (iter != _blackboard_parents_to_children.end()) {
		children = iter->value;
		for (const RID& child_rid : children) {
			Blackboard *child = _blackboard_owner.get_or_null(child_rid);
			ERR_CONTINUE(child == nullptr);
			child->set_parent(nullptr);

		}
		children.clear();
		_blackboard_parents_to_children.remove(iter);
	}

	for (const auto& child : children) {
		_blackboard_emit_set_parent(child, RID());
	}

	_blackboard_emit_destroyed(self);
}

_FORCE_INLINE_ void BehaviorServer::_blackboard_emit_set_parent(RID p_child, RID p_parent) {
	HydrogenBehaviorServer::get_singleton()->_blackboard_emit_set_parent(p_child, p_parent);
}

_FORCE_INLINE_ void BehaviorServer::_blackboard_emit_created(RID p_blackboard) {
	HydrogenBehaviorServer::get_singleton()->_blackboard_emit_created(p_blackboard);
}

_FORCE_INLINE_ void BehaviorServer::_blackboard_emit_destroyed(RID p_blackboard) {
	HydrogenBehaviorServer::get_singleton()->_blackboard_emit_destroyed(p_blackboard);
}

#define TRY_GET_CONST_BLACKBOARD(fail_result)									\
	BLACKBOARDS_LOCK_V(fail_result);											\
	const Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);	\
	ERR_FAIL_NULL_V(blackboard, fail_result)									\

#define TRY_GET_BLACKBOARD(fail_result)										\
	BLACKBOARDS_LOCK_V(fail_result);										\
	Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);	\
	ERR_FAIL_NULL_V(blackboard, fail_result)								\

bool BehaviorServer::blackboard_is_empty(RID p_blackboard) {
	TRY_GET_CONST_BLACKBOARD(false);
	return blackboard->is_empty();
}

uint32_t BehaviorServer::blackboard_get_size(RID p_blackboard) {
	TRY_GET_CONST_BLACKBOARD(0);
	return blackboard->size();
}

bool BehaviorServer::blackboard_set_parent(RID p_blackboard, RID p_parent) {
	BLACKBOARDS_LOCK_V(false);
	Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);
	const Blackboard *parent = _blackboard_owner.get_or_null(p_parent);

	ERR_FAIL_NULL_V(blackboard, false);

	const Blackboard *previous = blackboard->get_parent();
	const bool success = blackboard->set_parent(parent);

	if (previous) {
		_blackboard_remove_child(previous->get_self(), p_blackboard);
	}

	if (likely(success)) {
		if (likely(parent)) {
			_blackboard_add_child(p_parent, p_blackboard);
		}

		_blackboard_emit_set_parent(p_blackboard, p_parent);
	}

	return success;
}

RID BehaviorServer::blackboard_get_parent(RID p_blackboard) {
	TRY_GET_CONST_BLACKBOARD(RID());
	const Blackboard *parent = blackboard->get_parent();
	return parent ? parent->get_self() : RID();
}

bool BehaviorServer::blackboard_is_ancestor(RID p_blackboard, RID p_candidate) {
	BLACKBOARDS_LOCK_V(false);
	const Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);
	const Blackboard *candidate = _blackboard_owner.get_or_null(p_candidate);
	ERR_FAIL_NULL_V(blackboard, false);
	ERR_FAIL_NULL_V(candidate, false);
	return  blackboard->is_ancestor(candidate);
}

bool BehaviorServer::blackboard_erase_entry(RID p_blackboard, const StringName &p_name) {
	TRY_GET_BLACKBOARD(false);
	return blackboard->erase_entry(p_name);
}

bool BehaviorServer::blackboard_has_entry(RID p_blackboard, const StringName &p_name, bool p_check_parents) {
	TRY_GET_CONST_BLACKBOARD(false);
	return blackboard->has_entry(p_name, p_check_parents);
}

bool BehaviorServer::blackboard_import_entries(RID p_blackboard, const TypedDictionary<StringName, Variant> &p_values) {
	TRY_GET_BLACKBOARD(false);
	return blackboard->import_entries(p_values);
}

Dictionary BehaviorServer::blackboard_export_entries(RID p_blackboard, const bool p_include_parents) {
	TRY_GET_CONST_BLACKBOARD({});
	return blackboard->export_entries(p_include_parents);
}

Dictionary BehaviorServer::blackboard_export_type_infos() {
	return Blackboard::export_type_infos();
}

#undef TRY_GET_CONST_BLACKBOARD
#undef TRY_GET_BLACKBOARD

// ---- Blackboard END ----

// ---- Behavior Tree ----

void BehaviorServer::_graph_erase(IPipelineGraph *p_graph) {
	_graph_emit_destroyed(p_graph->get_self());
}

void BehaviorServer::_pipeline_erase(Pipeline *p_behavior_tree) {
	_pipeline_emit_destroyed(p_behavior_tree->get_self());
}

void BehaviorServer::_graph_emit_created(RID p_graph) {
	HydrogenBehaviorServer::get_singleton()->_graph_emit_created(p_graph);
}

void BehaviorServer::_graph_emit_destroyed(RID p_graph) {
	HydrogenBehaviorServer::get_singleton()->_graph_emit_destroyed(p_graph);
}

void BehaviorServer::_pipeline_emit_created(RID p_pipeline) {
	HydrogenBehaviorServer::get_singleton()->_pipeline_emit_created(p_pipeline);
}

void BehaviorServer::_pipeline_emit_destroyed(RID p_behavior_tree) {
	return HydrogenBehaviorServer::get_singleton()->_pipeline_emit_destroyed(p_behavior_tree);
}

#define TRY_GET_CONST_GRAPH(fail_result)								\
	GRAPHS_LOCK_V(fail_result);											\
	const IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);	\
	ERR_FAIL_NULL_V(graph, fail_result)									\

#define TRY_GET_GRAPH(fail_result)								\
	GRAPHS_LOCK_V(fail_result);									\
	IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);	\
	ERR_FAIL_NULL_V(graph, fail_result)							\

#define TRY_GET_CONST_NODE(fail_result)								\
	const IPipelineNode *node = graph->get_pipeline_node(p_node);	\
	ERR_FAIL_NULL_V(node, fail_result)								\

#define TRY_GET_NODE(fail_result)							\
	IPipelineNode *node = graph->get_pipeline_node(p_node);	\
	ERR_FAIL_NULL_V(node, fail_result)						\

#define TRY_GET_CONST_GRAPH_AND_NODE(fail_result)	\
	TRY_GET_CONST_GRAPH(fail_result);				\
	TRY_GET_CONST_NODE(fail_result)					\

#define TRY_GET_GRAPH_AND_NODE(fail_result)	\
	TRY_GET_GRAPH(fail_result);				\
	TRY_GET_NODE(fail_result)				\

#define TRY_GET_GRAPH_EDITABLE(fail_result)														\
	TRY_GET_GRAPH(fail_result);																	\
	ERR_FAIL_COND_V_MSG(graph->is_bound(), fail_result, "Cannot edit graph while it's bound!")	\

#define TRY_GET_GRAPH_AND_NODE_EDITABLE(fail_result)	\
	TRY_GET_GRAPH_EDITABLE(fail_result);				\
	TRY_GET_NODE(fail_result)							\

// ---- Graph ----

TypedArray<RID> BehaviorServer::graph_get_sub_graphs(RID p_graph) {
	TRY_GET_CONST_GRAPH({});

	TypedArray<RID> rids = {};

	Vector<const IPipelineGraph *> sub_graphs = graph->get_pipeline_sub_graphs();
	const int64_t sub_graphs_count = sub_graphs.size();
	if (likely(sub_graphs_count > 0)) {
		rids.resize(sub_graphs.size());
	}
	for (int64_t i = 0; i < sub_graphs.size(); ++i) {
		rids[i] = sub_graphs[i]->get_self();
	}

	return rids;
}

TypedArray<RID> BehaviorServer::graph_get_nodes(RID p_graph) {
	TRY_GET_CONST_GRAPH({});

	TypedArray<RID> rids = {};
	
	Vector<const IPipelineNode *> nodes = graph->get_pipeline_nodes();
	const int64_t nodes_count = nodes.size();
	if (likely(nodes_count > 0)) {
		rids.resize(nodes_count);
	}

	for(int64_t i = 0; i < nodes_count; ++i) {
		rids[i] = nodes[i]->get_self();
	}

	return rids;
}

RID BehaviorServer::graph_create_node(RID p_graph, const StringName &p_node_type_name) {
	TRY_GET_GRAPH_EDITABLE(RID());
	return graph->create_node(p_node_type_name);
}

bool BehaviorServer::graph_destroy_node(RID p_graph, RID p_node) {
	TRY_GET_GRAPH_EDITABLE(false);
	return graph->destroy_node(p_node);
}

bool BehaviorServer::graph_is_bound(RID p_graph) {
	TRY_GET_CONST_GRAPH(false);
	return graph->is_bound();
}

bool BehaviorServer::graph_set_root(RID p_graph, RID p_node) {
	TRY_GET_GRAPH_EDITABLE(false);
	return graph->set_root_id(p_node);
}

RID BehaviorServer::graph_get_root(RID p_graph) {
	TRY_GET_CONST_GRAPH(RID());
	return graph->get_root_id();
}

TypedArray<RID> prepare_nodes(const Vector<RID> &p_nodes) {
	TypedArray<RID>	result = {};
	if (p_nodes.is_empty()) {
		return result;
	}

	const int32_t nodes_count = p_nodes.size();
	result.resize(nodes_count);
	for (int32_t i = 0; i < nodes_count; ++i) {
		result[i] = p_nodes[i];
	}

	return result;
}

TypedArray<RID> BehaviorServer::graph_get_unrooted_nodes(RID p_graph) {
	TRY_GET_CONST_GRAPH({});
	return prepare_nodes(graph->get_unrooted_nodes());
}

TypedArray<RID> BehaviorServer::graph_get_rooted_nodes(RID p_graph) {
	TRY_GET_CONST_GRAPH({});
	return prepare_nodes(graph->get_rooted_nodes());
}

// ---- Graph END ----

// ---- Nodes ----

int32_t BehaviorServer::node_get_input_port_count(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE(0);
	return node->get_input_port_count();
}

int32_t BehaviorServer::node_get_output_port_count(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE(0);
	return node->get_output_port_count();
}

StringName BehaviorServer::node_get_input_port_type_name(RID p_graph, RID p_node, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH_AND_NODE(StringName());
	return node->get_input_port_type_name(p_port_index);
}

StringName BehaviorServer::node_get_output_port_type_name(RID p_graph, RID p_node, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH_AND_NODE(StringName());
	return node->get_output_port_type_name(p_port_index);
}

StringName BehaviorServer::node_get_input_port_name(RID p_graph, RID p_node, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH_AND_NODE(StringName());
	return node->get_input_port_name(p_port_index);
}

StringName BehaviorServer::node_get_output_port_name(RID p_graph, RID p_node, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH_AND_NODE(StringName());
	return node->get_output_port_name(p_port_index);
}

TypedArray<Dictionary> prepare_port_infos(const Vector<NodePortInfo> &port_infos) {
	TypedArray<Dictionary> result = {};
	if(port_infos.is_empty()) {
		return result;
	}
	const int32_t port_infos_count = port_infos.size();
	result.resize(port_infos_count);
	for (int32_t i = 0; i < port_infos_count; ++i) {
		const NodePortInfo &port_info = port_infos[i];
		Dictionary out_port_info = Dictionary();
		out_port_info["name"] = port_info.name;
		out_port_info["type_name"] = port_info.type_name;
	}

	return result;
}

TypedArray<Dictionary> BehaviorServer::node_get_input_port_infos(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE({});
	
	Vector<NodePortInfo> port_infos = {};
	node->get_input_port_infos(port_infos);
	return prepare_port_infos(port_infos);
}

TypedArray<Dictionary> BehaviorServer::node_get_output_port_infos(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE({});

	Vector<NodePortInfo> port_infos;
	node->get_output_port_infos(port_infos);
	return prepare_port_infos(port_infos);
}

#undef TRY_GET_CONST_GRAPH
#undef TRY_GET_GRAPH
#undef TRY_GET_CONST_NODE
#undef TRY_GET_NODE
#undef TRY_GET_CONST_GRAPH_AND_NODE
#undef TRY_GET_GRAPH_AND_NODE
#undef TRY_GET_GRAPH_EDITABLE
#undef TRY_GET_GRAPH_AND_NODE_EDITABLE

#undef BLACKBOARDS_LOCK_V
#undef BLACKBOARDS_LOCK
#undef PIPELINES_LOCK_V
#undef PIPELINES_LOCK
#undef GRAPHS_LOCK_V
#undef GRAPHS_LOCK

// ---- Behavior Tree END ----

HydrogenBehaviorServer *HydrogenBehaviorServer::singleton = nullptr;
HydrogenBehaviorServer *HydrogenBehaviorServer::get_singleton() {
	return singleton;
}

void HydrogenBehaviorServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("free_rid", "rid"), &HydrogenBehaviorServer::free_rid);
	ClassDB::bind_method(D_METHOD("blackboard_create"), &HydrogenBehaviorServer::blackboard_create);
	ClassDB::bind_method(D_METHOD("behavior_tree_create"), &HydrogenBehaviorServer::pipeline_create);
	ClassDB::bind_method(D_METHOD("agent_create"), &HydrogenBehaviorServer::agent_create);

	// ---- Blackboard ----

	ClassDB::bind_method(D_METHOD("blackboard_is_empty", "blackboard_rid"), &HydrogenBehaviorServer::blackboard_is_empty);
	ClassDB::bind_method(D_METHOD("blackboard_get_size", "blackboard_rid"), &HydrogenBehaviorServer::blackboard_get_size);

	ClassDB::bind_method(D_METHOD("blackboard_set_parent", "blackboard_rid", "parent"), &HydrogenBehaviorServer::blackboard_set_parent);
	ClassDB::bind_method(D_METHOD("blackboard_get_parent", "blackboard_rid"), &HydrogenBehaviorServer::blackboard_get_parent);
	ClassDB::bind_method(D_METHOD("blackboard_is_ancestor", "blackboard_rid", "candidate"), &HydrogenBehaviorServer::blackboard_is_ancestor);

	ClassDB::bind_method(D_METHOD("blackboard_try_get_entry", "blackboard_rid", "name", "check_parents"), &HydrogenBehaviorServer::blackboard_try_get_as_variant, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("blackboard_get_entry", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Variant>, DEFVAL(Variant()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_bool", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<bool>, DEFVAL(false), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_int", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<int64_t>, DEFVAL(0), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_float", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<real_t>, DEFVAL(0.0), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_string", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<String>, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector2", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector2>, DEFVAL(Vector2()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector2i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector2i>, DEFVAL(Vector2i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_rect2", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Rect2>, DEFVAL(Rect2()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_rect2i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Rect2i>, DEFVAL(Rect2i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector3", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector3>, DEFVAL(Vector3()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector3i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector3i>, DEFVAL(Vector3i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_transform2d", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Transform2D>, DEFVAL(Transform2D()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector4", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector4>, DEFVAL(Vector4()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector4i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector4i>, DEFVAL(Vector4i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_plane", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Plane>, DEFVAL(Plane()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_quaternion", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Quaternion>, DEFVAL(Quaternion()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_aabb", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<AABB>, DEFVAL(AABB()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_basis", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Basis>, DEFVAL(Basis()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_transform3d", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Transform3D>, DEFVAL(Transform3D()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_projection", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Projection>, DEFVAL(Projection()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_color", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Color>, DEFVAL(Color()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_string_name", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<StringName>, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_node_path", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<NodePath>, DEFVAL(NodePath()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_rid", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<RID>, DEFVAL(RID()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_object", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Object*>, DEFVAL(nullptr), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_ref_counted", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Ref<RefCounted>>, DEFVAL(Ref<RefCounted>()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_callable", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Callable>, DEFVAL(Callable()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_signal", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Signal>, DEFVAL(Signal()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_dictionary", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Dictionary>, DEFVAL(Dictionary()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Array>, DEFVAL(Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_byte_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedByteArray>, DEFVAL(PackedByteArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_int32_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedInt32Array>, DEFVAL(PackedInt32Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_int64_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedInt64Array>, DEFVAL(PackedInt64Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_float32_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedFloat32Array>, DEFVAL(PackedFloat32Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_float64_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedFloat64Array>, DEFVAL(PackedFloat64Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_string_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedStringArray>, DEFVAL(PackedStringArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_vector2_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedVector2Array>, DEFVAL(PackedVector2Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_vector3_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedVector3Array>, DEFVAL(PackedVector3Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_color_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedColorArray>, DEFVAL(PackedColorArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_vector4_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedVector4Array>, DEFVAL(PackedVector4Array()), DEFVAL(true));

	ClassDB::bind_method(D_METHOD("blackboard_set_entry", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Variant>);
	ClassDB::bind_method(D_METHOD("blackboard_set_bool", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<bool>);
	ClassDB::bind_method(D_METHOD("blackboard_set_int", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<int64_t>);
	ClassDB::bind_method(D_METHOD("blackboard_set_float", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<real_t>);
	ClassDB::bind_method(D_METHOD("blackboard_set_string", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<String>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector2", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector2>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector2i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector2i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_rect2", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Rect2>);
	ClassDB::bind_method(D_METHOD("blackboard_set_rect2i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Rect2i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector3", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector3>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector3i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector3i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_transform2d", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Transform2D>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector4", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector4>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector4i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector4i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_plane", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Plane>);
	ClassDB::bind_method(D_METHOD("blackboard_set_quaternion", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Quaternion>);
	ClassDB::bind_method(D_METHOD("blackboard_set_aabb", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<AABB>);
	ClassDB::bind_method(D_METHOD("blackboard_set_basis", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Basis>);
	ClassDB::bind_method(D_METHOD("blackboard_set_transform3d", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Transform3D>);
	ClassDB::bind_method(D_METHOD("blackboard_set_projection", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Projection>);
	ClassDB::bind_method(D_METHOD("blackboard_set_color", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Color>);
	ClassDB::bind_method(D_METHOD("blackboard_set_string_name", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<StringName>);
	ClassDB::bind_method(D_METHOD("blackboard_set_node_path", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<NodePath>);
	ClassDB::bind_method(D_METHOD("blackboard_set_rid", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<RID>);
	ClassDB::bind_method(D_METHOD("blackboard_set_object", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Object*>);
	ClassDB::bind_method(D_METHOD("blackboard_set_ref_counted", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Ref<RefCounted>>);
	ClassDB::bind_method(D_METHOD("blackboard_set_callable", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Callable>);
	ClassDB::bind_method(D_METHOD("blackboard_set_signal", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Signal>);
	ClassDB::bind_method(D_METHOD("blackboard_set_dictionary", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Dictionary>);
	ClassDB::bind_method(D_METHOD("blackboard_set_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_byte_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedByteArray>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_int32_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedInt32Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_int64_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedInt64Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_float32_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedFloat32Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_float64_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedFloat64Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_string_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedStringArray>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_vector2_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedVector2Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_vector3_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedVector3Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_color_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedColorArray>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_vector4_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedVector4Array>);

	ClassDB::bind_method(D_METHOD("blackboard_erase_entry", "blackboard_rid", "name"), &HydrogenBehaviorServer::blackboard_erase_entry);
	ClassDB::bind_method(D_METHOD("blackboard_has_entry", "blackboard_rid", "name"), &HydrogenBehaviorServer::blackboard_has_entry);

	ClassDB::bind_method(D_METHOD("blackboard_import_entries", "blackboard_rid", "data"), &HydrogenBehaviorServer::blackboard_import_entries);
	ClassDB::bind_method(D_METHOD("blackboard_export_entries", "blackboard_rid", "p_include_parents"), &HydrogenBehaviorServer::blackboard_export_entries, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("blackboard_export_type_infos"), &HydrogenBehaviorServer::blackboard_export_type_infos);

	ADD_SIGNAL(MethodInfo("blackboard_created", PropertyInfo(Variant::RID, "blackboard_rid")));
	ADD_SIGNAL(MethodInfo("blackboard_destroyed", PropertyInfo(Variant::RID, "blackboard_rid")));
	ADD_SIGNAL(MethodInfo("blackboard_parent_set", PropertyInfo(Variant::RID, "child_rid"), PropertyInfo(Variant::RID, "parent_rid")));
	// ---- Blackboard End ----

	// ---- Behavior Tree ----

	ClassDB::bind_method(D_METHOD("graph_create", "graph_type"), &HydrogenBehaviorServer::graph_create);
	ClassDB::bind_method(D_METHOD("pipeline_create", "pipeline_type", "blackboard", "graph"), &HydrogenBehaviorServer::pipeline_create);

	ClassDB::bind_method(D_METHOD("graph_create_node", "graph", "node_type_name"), &HydrogenBehaviorServer::graph_create_node);
	ClassDB::bind_method(D_METHOD("graph_destroy_node", "graph", "node"), &HydrogenBehaviorServer::graph_destroy_node);
	ClassDB::bind_method(D_METHOD("graph_is_bound", "graph"), &HydrogenBehaviorServer::graph_is_bound);
	ClassDB::bind_method(D_METHOD("graph_set_root", "graph", "node"), &HydrogenBehaviorServer::graph_set_root);
	ClassDB::bind_method(D_METHOD("graph_get_root", "graph"), &HydrogenBehaviorServer::graph_get_root);
	ClassDB::bind_method(D_METHOD("graph_get_unrooted_nodes", "graph"), &HydrogenBehaviorServer::graph_get_unrooted_nodes);
	ClassDB::bind_method(D_METHOD("graph_get_rooted_nodes", "graph"), &HydrogenBehaviorServer::graph_get_rooted_nodes);

	ClassDB::bind_method(D_METHOD("node_get_input_port_count", "graph", "node"), &HydrogenBehaviorServer::node_get_input_port_count);
	ClassDB::bind_method(D_METHOD("node_get_output_port_count", "graph", "node"), &HydrogenBehaviorServer::node_get_output_port_count);
	ClassDB::bind_method(D_METHOD("node_get_input_port_type_name", "graph", "node", "port_index"), &HydrogenBehaviorServer::node_get_input_port_type_name);
	ClassDB::bind_method(D_METHOD("node_get_output_port_type_name", "graph", "node", "port_index"), &HydrogenBehaviorServer::node_get_output_port_type_name);
	ClassDB::bind_method(D_METHOD("node_get_input_port_name", "graph", "node", "port_index"), &HydrogenBehaviorServer::node_get_input_port_name);
	ClassDB::bind_method(D_METHOD("node_get_output_port_name", "graph", "node", "port_index"), &HydrogenBehaviorServer::node_get_output_port_name);
	ClassDB::bind_method(D_METHOD("node_get_input_port_infos", "graph", "node"), &HydrogenBehaviorServer::node_get_input_port_infos);
	ClassDB::bind_method(D_METHOD("node_get_output_port_infos", "graph", "node"), &HydrogenBehaviorServer::node_get_output_port_infos);
	

	ADD_SIGNAL(MethodInfo("graph_created", PropertyInfo(Variant::RID, "graph")));
	ADD_SIGNAL(MethodInfo("graph_destroyed", PropertyInfo(Variant::RID, "graph")));
	
	ADD_SIGNAL(MethodInfo("pipeline_created", PropertyInfo(Variant::RID, "tree")));
	ADD_SIGNAL(MethodInfo("pipeline_destroyed", PropertyInfo(Variant::RID, "tree")));
	// ---- Behavior Tree End ----

	BIND_ENUM_CONSTANT(BehaviorServer::BEHAVIOR_TREE);
	BIND_ENUM_CONSTANT(BehaviorServer::GraphType::GRAPH_TYPE_MAX);

#if TESTS_ENABLED
	ClassDB::bind_method(D_METHOD("run_tests"), &HydrogenBehaviorServer::run_tests);
#endif
}

// ReSharper disable CppMemberFunctionMayBeStatic
void HydrogenBehaviorServer::free_rid(RID p_rid) {
	BehaviorServer::get_singleton()->free_rid(p_rid);
}

RID HydrogenBehaviorServer::blackboard_create() {
	return BehaviorServer::get_singleton()->blackboard_create();
}

RID HydrogenBehaviorServer::graph_create(BehaviorServer::GraphType p_graph_type) {
	return BehaviorServer::get_singleton()->graph_create(p_graph_type);
}

RID HydrogenBehaviorServer::pipeline_create(BehaviorServer::GraphType p_graph_type, RID p_blackboard, RID p_behavior_tree_graph) {
	return BehaviorServer::get_singleton()->pipeline_create(p_graph_type, p_blackboard, p_behavior_tree_graph);
}

RID HydrogenBehaviorServer::agent_create() {
	return BehaviorServer::get_singleton()->agent_create();
}

// ---- Blackboard ----

void HydrogenBehaviorServer::_blackboard_emit_created(RID p_blackboard) {
	emit_signal("blackboard_created", p_blackboard);
}

void HydrogenBehaviorServer::_blackboard_emit_destroyed(RID p_blackboard) {
	emit_signal("blackboard_destroyed", p_blackboard);
}

void HydrogenBehaviorServer::_blackboard_emit_set_parent(RID p_child, RID p_parent) {
	emit_signal("blackboard_emit_set_parent", p_child, p_parent);
}

bool HydrogenBehaviorServer::blackboard_is_empty(RID p_blackboard) const {
	return BehaviorServer::get_singleton()->blackboard_is_empty(p_blackboard);
}

uint32_t HydrogenBehaviorServer::blackboard_get_size(RID p_blackboard) const {
	return BehaviorServer::get_singleton()->blackboard_get_size(p_blackboard);
}

bool HydrogenBehaviorServer::blackboard_set_parent(RID p_child, RID p_parent) {
	return BehaviorServer::get_singleton()->blackboard_set_parent(p_child, p_parent);
}

RID HydrogenBehaviorServer::blackboard_get_parent(RID p_blackboard) {
	return BehaviorServer::get_singleton()->blackboard_get_parent(p_blackboard);
}

bool HydrogenBehaviorServer::blackboard_is_ancestor(RID p_blackboard, RID p_candidate) {
	return BehaviorServer::get_singleton()->blackboard_is_ancestor(p_blackboard, p_candidate);
}

bool HydrogenBehaviorServer::blackboard_erase_entry(RID p_blackboard, const StringName &p_name) {
	return BehaviorServer::get_singleton()->blackboard_erase_entry(p_blackboard, p_name);
}

bool HydrogenBehaviorServer::blackboard_has_entry(RID p_blackboard, const StringName &p_name, const bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_has_entry(p_blackboard, p_name, p_check_parents);
}

bool HydrogenBehaviorServer::blackboard_import_entries(RID p_blackboard, const TypedDictionary<StringName, Variant> &p_data) {
	return BehaviorServer::get_singleton()->blackboard_import_entries(p_blackboard, p_data);
}

Dictionary HydrogenBehaviorServer::blackboard_export_entries(RID p_blackboard, const bool p_include_parents) {
	return BehaviorServer::get_singleton()->blackboard_export_entries(p_blackboard, p_include_parents);
}

Dictionary HydrogenBehaviorServer::blackboard_export_type_infos() {
	return BehaviorServer::blackboard_export_type_infos();
}
// ---- Blackboard END ----

// ---- Graphs ----

void HydrogenBehaviorServer::_graph_emit_created(RID p_graph) {
	emit_signal("graph_created", p_graph);
}

void HydrogenBehaviorServer::_graph_emit_destroyed(RID p_graph) {
	emit_signal("graph_destroyed", p_graph);
}

RID HydrogenBehaviorServer::graph_create_node(RID p_graph, const StringName &p_node_type_name) {
	return BehaviorServer::get_singleton()->graph_create_node(p_graph, p_node_type_name);
}

bool HydrogenBehaviorServer::graph_destroy_node(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->graph_destroy_node(p_graph, p_node);
}

bool HydrogenBehaviorServer::graph_is_bound(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_is_bound(p_graph);
}

bool HydrogenBehaviorServer::graph_set_root(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->graph_set_root(p_graph, p_node);
}

RID HydrogenBehaviorServer::graph_get_root(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_root(p_graph);
}

TypedArray<RID> HydrogenBehaviorServer::graph_get_unrooted_nodes(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_unrooted_nodes(p_graph);
}

TypedArray<RID> HydrogenBehaviorServer::graph_get_rooted_nodes(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_rooted_nodes(p_graph);
}

// ---- Graphs END ----

// ---- Pipelines ----

void HydrogenBehaviorServer::_pipeline_emit_created(RID p_pipeline) {
	emit_signal("pipeline_created", p_pipeline);
}

void HydrogenBehaviorServer::_pipeline_emit_destroyed(RID p_pipeline) {
	emit_signal("pipeline_destroyed", p_pipeline);
}

// ---- Pipelines END ----

// ---- Nodes ----

int32_t HydrogenBehaviorServer::node_get_input_port_count(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_input_port_count(p_graph, p_node);
}

int32_t HydrogenBehaviorServer::node_get_output_port_count(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_output_port_count(p_graph, p_node);
}

StringName HydrogenBehaviorServer::node_get_input_port_type_name(RID p_graph, RID p_node, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_input_port_type_name(p_graph, p_node, p_port_index);
}

StringName HydrogenBehaviorServer::node_get_output_port_type_name(RID p_graph, RID p_node, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_output_port_type_name(p_graph, p_node, p_port_index);
}

StringName HydrogenBehaviorServer::node_get_input_port_name(RID p_graph, RID p_node, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_input_port_name(p_graph, p_node, p_port_index);
}

StringName HydrogenBehaviorServer::node_get_output_port_name(RID p_graph, RID p_node, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_output_port_name(p_graph, p_node, p_port_index);
}

TypedArray<Dictionary> HydrogenBehaviorServer::node_get_input_port_infos(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_input_port_infos(p_graph, p_node);
}

TypedArray<Dictionary> HydrogenBehaviorServer::node_get_output_port_infos(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_output_port_infos(p_graph, p_node);
}


// ---- Nodes END ----

#if TESTS_ENABLED
void HydrogenBehaviorServer::run_tests() {
	tests::behavior_test_runner();
}
#endif
// ReSharper restore CppMemberFunctionMayBeStatic

}
