#include "behavior_server.hpp"
#include "behavior_trees/behavior_tree_graph.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "behavior_trees/behavior_trees.hpp"
#include "blackboard.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/graph_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/templates/pair.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "mutex_helpers.hpp"
#include "pipelines/node_interfaces.hpp"

#include <cstdint>
#include <functional>
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
		HANDLE_CLEANUP(IPipeline, bt, pipeline)
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

#define BLACKBOARDS_LOCK() LOCK_ONE(_blackboard_mutex)
	
#define PIPELINES_LOCK_V(fail_result) LOCK_ONE_V(_pipeline_mutex, fail_result)

#define PIPELINES_LOCK() LOCK_ONE(_pipeline_mutex)

#define GRAPHS_LOCK_V(fail_result) LOCK_ONE_V(_graph_mutex, fail_result)

#define GRAPHS_LOCK() LOCK_ONE(_graph_mutex)


void BehaviorServer::_blackboard_acquire(Blackboard *p_blackboard) {
	BLACKBOARDS_LOCK();

	RID rid = _blackboard_owner.make_rid(p_blackboard);
	p_blackboard->set_self(rid);

	const Blackboard *parent = p_blackboard->get_parent();
	if (likely(parent == nullptr)) {
		_blackboard_add_child(parent->get_self(), rid);
	}
}

void BehaviorServer::_blackboard_release(Blackboard *p_blackboard) {
	ERR_FAIL_NULL(p_blackboard);
	BLACKBOARDS_LOCK();

	RID rid = p_blackboard->get_self();

	const Blackboard *parent = p_blackboard->get_parent();
	if (likely(parent != nullptr)) {
		_blackboard_remove_child(parent->get_self(), rid);
	}

	_blackboard_owner.free(rid);
	p_blackboard->set_self(RID());
}

RID BehaviorServer::blackboard_create() {
	BLACKBOARDS_LOCK_V(RID());
	RID rid = _blackboard_create_helper();

	if (likely(rid.is_valid())) {
		_blackboard_emit_created(rid);
	}

	return rid;
}

RID BehaviorServer::graph_create(const StringName &p_registration_key) {
	LOCK_ONE_V(_graph_mutex, RID());
	const auto iter = _pipeline_db.find(p_registration_key);
	ERR_FAIL_COND_V(iter == _pipeline_db.end(), RID());

	const PipelineRegistration &reg = iter->value;
	RID rid = reg.create_graph(p_registration_key);

	if (likely(rid.is_valid())) {
		_graph_emit_created(rid);
	}

	return rid;
}

RID BehaviorServer::pipeline_create(RID p_graph, RID p_blackboard) {
	ERR_FAIL_COND_V(!p_graph.is_valid(), RID());
	LOCK_THREE_V(_pipeline_mutex, _blackboard_mutex, _graph_mutex, RID());

	bool owns_source_blackboard = false;
	if (unlikely(!p_blackboard.is_valid())) {
		p_blackboard = blackboard_create();
		owns_source_blackboard = true;
	}

	const Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);
	ERR_FAIL_NULL_V(blackboard, RID());

	IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);
	ERR_FAIL_NULL_V(graph, RID());

	const StringName &reg_key = graph->group_key();
	const auto iter = _pipeline_db.find(reg_key);
	ERR_FAIL_COND_V(iter == _pipeline_db.end(), RID());

	const PipelineRegistration &reg = iter->value;
	RID rid = reg.create_pipeline(reg_key, blackboard, graph, owns_source_blackboard);

	if (likely(rid.is_valid())) {
		_pipeline_emit_created(rid);
	}

	return rid;
}


RID BehaviorServer::agent_create() {
	return {};
}

Error BehaviorServer::init() {
	_blackboard_mutex = memnew(std::recursive_mutex);
	_pipeline_mutex = memnew(std::recursive_mutex);
	_graph_mutex = memnew(std::recursive_mutex);

	register_graph_group<BehaviorTreeGraph, BehaviorTree>(BehaviorTree_name());

 	return OK;
}

template<typename T>
void _cleanup_owned(RID_PtrOwner<T> &p_owner, std::function<void(T *)> p_cleanup) {
	TightLocalVector<RID> rids = {};
	rids.resize(p_owner.get_rid_count());
	p_owner.fill_owned_buffer(rids.ptr());
	for(RID rid : rids) {
		T *owned = p_owner.get_or_null(rid);
		p_cleanup(owned);
		p_owner.free(rid);
		memdelete(owned);
	}
}

void BehaviorServer::finish() {
	_blackboard_parents_to_children.clear();
	_pipeline_db.clear();

	std::function<void(IPipeline *)> cleanup_pipeline = [this](IPipeline *p_pipeline) {
		_pipeline_erase(p_pipeline, true);
	};

	std::function<void(IPipelineGraph *)> cleanup_graphs = [this](IPipelineGraph *p_graph) {
		_graph_erase(p_graph, true);
	};

	std::function<void(Blackboard *)> cleanup_blackboards = [this](Blackboard *	p_blackboard) {
		_blackboard_erase(p_blackboard, true);
	};

	{
		// destroy in reverse order of dependencies
		LOCK_THREE(_pipeline_mutex, _graph_mutex, _blackboard_mutex);
		_cleanup_owned(_pipeline_owner, cleanup_pipeline);
		_cleanup_owned(_graph_owner, cleanup_graphs);
		_cleanup_owned(_blackboard_owner, cleanup_blackboards);
	}
	memdelete(_blackboard_mutex);
	memdelete(_pipeline_mutex);
	memdelete(_graph_mutex);
}

// ---- Blackboard ----

void BehaviorServer::_blackboard_add_child(RID parent, RID child) {
	BLACKBOARDS_LOCK();
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
	BLACKBOARDS_LOCK();
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

void BehaviorServer::_blackboard_erase(Blackboard *blackboard, bool p_is_final_shutdown) {
	if (unlikely(p_is_final_shutdown)) {
		return;
	}

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

void BehaviorServer::_graph_erase(IPipelineGraph *p_graph, bool p_is_final_shutdown) {
	if (likely(!p_is_final_shutdown)) {
		_graph_emit_destroyed(p_graph->get_id());
	}
}

void BehaviorServer::_pipeline_erase(IPipeline *p_pipeline, bool p_is_final_shutdown) {
	
	Blackboard * blackboard = p_pipeline->get_execution_blackboard();

	if (likely(!p_is_final_shutdown)) {

		_blackboard_release(blackboard);

		const Blackboard *parent = p_pipeline->get_source_blackboard();
		bool delete_source = p_pipeline->owns_source_blackboard();

		_pipeline_emit_destroyed(p_pipeline->get_id());

		if (delete_source && parent) {
			free_rid(parent->get_self());
		}
	}
	else {
		RID rid = blackboard->get_self();
		_blackboard_owner.free(rid);
		blackboard->set_self(RID());
	}
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

#define TRY_GET_CONST_GRAPH_V(fail_result)								\
	GRAPHS_LOCK_V(fail_result);											\
	const IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);	\
	ERR_FAIL_NULL_V(graph, fail_result)									\

#define TRY_GET_CONST_GRAPH()											\
	GRAPHS_LOCK();														\
	const IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);	\
	ERR_FAIL_NULL(graph)												\

#define TRY_GET_GRAPH_V(fail_result)							\
	GRAPHS_LOCK_V(fail_result);									\
	IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);	\
	ERR_FAIL_NULL_V(graph, fail_result)							\

#define TRY_GET_GRAPH()											\
	GRAPHS_LOCK();												\
	IPipelineGraph *graph = _graph_owner.get_or_null(p_graph);	\
	ERR_FAIL_NULL(graph)										\

#define TRY_GET_CONST_NODE_V(fail_result)				\
	const IPipelineNode *node = graph->get_node(p_node);\
	ERR_FAIL_NULL_V(node, fail_result)					\

#define TRY_GET_CONST_NODE()							\
	const IPipelineNode *node = graph->get_node(p_node);\
	ERR_FAIL_NULL(node)									\

#define TRY_GET_NODE_V(fail_result)					\
	IPipelineNode *node = graph->get_node(p_node);	\
	ERR_FAIL_NULL_V(node, fail_result)				\

#define TRY_GET_NODE()								\
	IPipelineNode *node = graph->get_node(p_node);	\
	ERR_FAIL_NULL(node)								\

#define TRY_GET_CONST_GRAPH_AND_NODE_V(fail_result)	\
	TRY_GET_CONST_GRAPH_V(fail_result);				\
	TRY_GET_CONST_NODE_V(fail_result)				\

#define TRY_GET_CONST_GRAPH_AND_NODE()	\
	TRY_GET_CONST_GRAPH();				\
	TRY_GET_CONST_NODE()				\

#define TRY_GET_GRAPH_AND_NODE_V(fail_result)	\
	TRY_GET_GRAPH_V(fail_result);				\
	TRY_GET_NODE_V(fail_result)					\

#define TRY_GET_GRAPH_AND_NODE()\
	TRY_GET_GRAPH_V();			\
	TRY_GET_NODE_V()			\

#define TRY_GET_GRAPH_AND_COMPOSITE_NODE_V(fail_result)									\
	TRY_GET_GRAPH_V(fail_result);														\
	TRY_GET_NODE_V(fail_result);														\
	IPipelineNodeComposite *composite = dynamic_cast<IPipelineNodeComposite *>(node);	\
	ERR_FAIL_NULL_V(composite, fail_result)												\

#define TRY_GET_GRAPH_AND_COMPOSITE_NODE()												\
	TRY_GET_GRAPH();																	\
	TRY_GET_NODE();																		\
	IPipelineNodeComposite *composite = dynamic_cast<IPipelineNodeComposite *>(node);	\
	ERR_FAIL_NULL(composite);															\

#define TRY_GET_GRAPH_AND_DECORATOR_NODE_V(fail_result) 								\
	TRY_GET_GRAPH_V(fail_result);														\
	TRY_GET_NODE_V(fail_result);														\
	IPipelineNodeDecorator *decorator = dynamic_cast<IPipelineNodeDecorator *>(node);	\
	ERR_FAIL_NULL_V(decorator, fail_result);											\

#define TRY_GET_GRAPH_AND_DECORATOR_NODE() 												\
	TRY_GET_GRAPH();																	\
	TRY_GET_NODE();																		\
	IPipelineNodeDecorator *decorator = dynamic_cast<IPipelineNodeDecorator *>(node);	\
	ERR_FAIL_NULL(decorator);															\


#define TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE_V(fail_result) 										\
	TRY_GET_CONST_GRAPH_V(fail_result);																\
	TRY_GET_CONST_NODE_V(fail_result);																\
	const IPipelineNodeDecorator *decorator = dynamic_cast<const IPipelineNodeDecorator *>(node);	\
	ERR_FAIL_NULL_V(decorator, fail_result);														\

#define TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE() 													\
	TRY_GET_CONST_GRAPH();																			\
	TRY_GET_CONST_NODE();																			\
	const IPipelineNodeDecorator *decorator = dynamic_cast<const IPipelineNodeDecorator *>(node);	\
	ERR_FAIL_NULL(decorator);																		\

#define TRY_GET_GRAPH_COMPOSITE_AND_CHILD_V(fail_result)	\
	TRY_GET_GRAPH_AND_COMPOSITE_NODE_V(fail_result);		\
	const IPipelineNode *child = graph->get_node(p_child);	\
	ERR_FAIL_NULL_V(child, fail_result)						\

#define TRY_GET_GRAPH_COMPOSITE_AND_CHILD()					\
	TRY_GET_GRAPH_AND_COMPOSITE_NODE();						\
	const IPipelineNode *child = graph->get_node(p_child);	\
	ERR_FAIL_NULL(child)									\

#define TRY_GET_GRAPH_EDITABLE_V(fail_result)													\
	TRY_GET_GRAPH_V(fail_result);																\
	ERR_FAIL_COND_V_MSG(graph->is_bound(), fail_result, "Cannot edit graph while it's bound!")	\

#define TRY_GET_GRAPH_EDITABLE()												\
	TRY_GET_GRAPH();															\
	ERR_FAIL_COND_MSG(graph->is_bound(), "Cannot edit graph while it's bound!")	\

#define TRY_GET_GRAPH_AND_NODE_EDITABLE_V(fail_result)	\
	TRY_GET_GRAPH_EDITABLE_V(fail_result);				\
	TRY_GET_NODE_V(fail_result)							\

#define TRY_GET_GRAPH_AND_NODE_EDITABLE()	\
	TRY_GET_GRAPH_EDITABLE();				\
	TRY_GET_NODE()							\

// ---- Graph ----


const StringName &BehaviorServer::graph_get_type(RID p_graph) {
	static const StringName empty = StringName();
	TRY_GET_CONST_GRAPH_V(empty);
	return graph->graph_type();
}

const StringName &BehaviorServer::graph_get_group_key(RID p_graph) {
	static const StringName empty = StringName();
	TRY_GET_CONST_GRAPH_V(empty);
	return graph->group_key();
}

bool BehaviorServer::graph_is_bound(RID p_graph) {
	TRY_GET_CONST_GRAPH_V(false);
	return graph->is_bound();
}

uint32_t BehaviorServer::graph_get_bind_count(RID p_graph) {
	TRY_GET_CONST_GRAPH_V(0);
	return graph->bind_count();
}

RID BehaviorServer::graph_create_node(RID p_graph, const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases) {
	TRY_GET_GRAPH_EDITABLE_V(RID());
	return graph->create_node(p_node_type_name, p_input_aliases, p_output_aliases);
}

bool BehaviorServer::graph_destroy_node(RID p_graph, RID p_node) {
	TRY_GET_GRAPH_EDITABLE_V(false);
	return graph->destroy_node(p_node);
}

uint64_t BehaviorServer::graph_get_node_count(RID p_graph) {
	TRY_GET_CONST_GRAPH_V(0);
	return graph->nodes_count();
}

TypedArray<RID> _nodes_to_ids(const Vector<const IPipelineNode *> p_nodes) {
	if (unlikely(p_nodes.is_empty())) {
		return {};
	}

	const int64_t nodes_count = p_nodes.size();
	TypedArray<RID> results = {};
	results.resize(nodes_count);
	for (int64_t idx = 0; idx < nodes_count; ++idx) {
		results[idx] = p_nodes[idx]->get_id();
	}

	return results;
}

TypedArray<RID> BehaviorServer::graph_get_sub_graphs(RID p_graph) {
	TRY_GET_CONST_GRAPH_V({});

	TypedArray<RID> rids = {};

	Vector<const IPipelineGraph *> sub_graphs = {};
	graph->get_sub_graphs(sub_graphs);
	const int64_t sub_graphs_count = sub_graphs.size();
	if (likely(sub_graphs_count > 0)) {
		rids.resize(sub_graphs.size());
	}
	for (int64_t i = 0; i < sub_graphs.size(); ++i) {
		rids[i] = sub_graphs[i]->get_id();
	}

	return rids;
}

TypedArray<RID> BehaviorServer::graph_get_nodes(RID p_graph) {
	TRY_GET_CONST_GRAPH_V({});

	TypedArray<RID> rids = {};
	
	Vector<const IPipelineNode *> nodes = {};
	graph->get_nodes(nodes);

	return _nodes_to_ids(nodes);
}

bool BehaviorServer::graph_set_root(RID p_graph, RID p_node) {
	TRY_GET_GRAPH_EDITABLE_V(false);
	return graph->set_root(p_node);
}

RID BehaviorServer::graph_get_root(RID p_graph) {
	TRY_GET_CONST_GRAPH_V(RID());
	return graph->get_root();
}

TypedArray<RID> BehaviorServer::graph_query_node(RID p_graph, RID p_node, Callable p_predicate) {
	TRY_GET_CONST_GRAPH_V({});
	ERR_FAIL_COND_V(p_predicate.is_null(), {});

	IPipelineGraph::PipelineNodePredicate predicate_wrapper = [&p_predicate](const IPipelineNode *p_node) -> bool {
		bool result = p_predicate.call(p_node->get_id());
		return result;
	};

	Vector<const IPipelineNode *> nodes = {};
	graph->query_node(p_node, nodes, predicate_wrapper);

	return _nodes_to_ids(nodes);
}

TypedArray<RID> BehaviorServer::graph_query_nodes(RID p_graph, Callable p_predicate) {
	TRY_GET_CONST_GRAPH_V({});
	ERR_FAIL_COND_V(p_predicate.is_null(), {});

	IPipelineGraph::PipelineNodePredicate predicate_wrapper = [&p_predicate](const IPipelineNode *p_node) -> bool {
		bool result = p_predicate.call(p_node->get_id());
		return result;
	};

	Vector<const IPipelineNode *> nodes = {};
	graph->query_nodes(nodes, predicate_wrapper);

	return _nodes_to_ids(nodes);
}

TypedArray<Dictionary> BehaviorServer::graph_get_rooted_statuses(RID p_graph) {
	TRY_GET_CONST_GRAPH_V({});

	Vector<Pair<const IPipelineNode *, bool>> statuses = {};
	graph->get_rooted_statuses(statuses);

	if (likely(statuses.is_empty())) {
		return {};
	}

	const int64_t nodes_count = statuses.size();

	TypedArray<Dictionary> result = {};
	result.resize(nodes_count);

	for (int64_t idx = 0; idx < nodes_count; ++idx) {
		Dictionary dict = {};
		auto pair = statuses[idx];
		dict["node"] = pair.first->get_id();
		dict["is_rooted"] = pair.second;
	}
	
	return result;
}

bool BehaviorServer::graph_node_is_parented(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);
	return graph->is_parented(node);
}

// ---- Graph END ----

// ---- Nodes ----

const StringName &BehaviorServer::node_get_type_name(RID p_graph, RID p_node) {
	static const StringName empty = StringName();
	TRY_GET_CONST_GRAPH_AND_NODE_V(empty);
	return node->get_type_name();
}


TypedArray<Dictionary> BehaviorServer::node_get_ports(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V({});

	const Vector<NodePortInfo> &ports = node->get_ports();
	if (likely(ports.is_empty())) {
		return {};
	}

	Blackboard bb = Blackboard();

	TypedArray<Dictionary> ports_array = {};
	const int64_t ports_count = ports.size();
	ports_array.resize(ports_count);
	for (int64_t i = 0; i < ports_count; ++i) {
		ports_array[i] = ports[i].to_dictionary(node, &bb);
	}
	
	return ports_array;
}

TypedArray<Dictionary> BehaviorServer::node_get_connections(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V({});

	const Vector<NodeConnectionInfo> &connections = node->get_connections();
	if (likely(connections.is_empty())) {
		return {};
	}

	TypedArray<Dictionary> connections_array = {};
	const int64_t connections_count = connections.size();
	connections_array.resize(connections_count);
	for(int64_t idx = 0; idx < connections_count; ++idx) {
		connections_array[idx] = connections[idx].to_dictionary();
	}

	return connections_array;
}

bool BehaviorServer::node_set_connection(RID p_graph, RID p_node, const StringName &p_name, RID p_target_node) {
	TRY_GET_GRAPH_AND_NODE_EDITABLE_V(false);
	const IPipelineNode *target = graph->get_node(p_target_node);
	ERR_FAIL_NULL_V(target, false);
	return node->set_connection(p_name, target);
}

RID BehaviorServer::node_get_connection(RID p_graph, RID p_node, const StringName &p_name) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(RID());
	const IPipelineNode *target = node->get_connection(p_name);
	ERR_FAIL_NULL_V(target, RID());
	return target->get_id();
}

bool BehaviorServer::node_is_parent(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE_V(false);
	return dynamic_cast<const IPipelineNodeParent *>(node) != nullptr;
}

bool BehaviorServer::node_is_composite(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);
	return dynamic_cast<const IPipelineNodeComposite *>(node) != nullptr;
}

bool BehaviorServer::node_is_decorator(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);
	return dynamic_cast<const IPipelineNodeDecorator *>(node) != nullptr;
}

void BehaviorServer::node_set_name(RID p_graph, RID p_node, const String &p_name) {
	TRY_GET_GRAPH_AND_NODE();
	node->set_name(p_name);
}

const String &BehaviorServer::node_get_name(RID p_graph, RID p_node) {
	static const String empty = "";
	TRY_GET_CONST_GRAPH_AND_NODE_V(empty);
	return node->get_name();
}

void BehaviorServer::node_set_input_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases) {
	TRY_GET_GRAPH_AND_NODE();
	node->set_input_aliases(p_aliases);
}

PortAliases BehaviorServer::get_input_aliases(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V({});
	return node->get_input_aliases();
}

void BehaviorServer::node_set_output_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases) {
	TRY_GET_GRAPH_AND_NODE();
	node->set_output_aliases(p_aliases);
}

PortAliases BehaviorServer::node_get_output_aliases(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V({});
	return node->get_output_aliases();
}

bool BehaviorServer::node_supports_children(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);
	return node->supports_children();
}

bool BehaviorServer::node_has_children(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);
	return node->has_children();
}

TypedArray<RID> BehaviorServer::node_get_children(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V({});

	Vector<const IPipelineNode *> children = {};
	node->get_children(children);

	return _nodes_to_ids(children);
}

TypedArray<RID> BehaviorServer::node_get_descendants(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_NODE_V({});

	Vector<const IPipelineNode *> descendants = {};
	node->get_descendants(descendants);
	return _nodes_to_ids(descendants);
}

bool BehaviorServer::node_has_child(RID p_graph, RID p_node, RID p_candidate) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);

	const IPipelineNode *candidate = graph->get_node(p_candidate);
	ERR_FAIL_NULL_V(candidate, false);

	return node->has_child(candidate);
}

bool BehaviorServer:: node_has_descendant(RID p_graph, RID p_node, RID p_candidate) {
	TRY_GET_CONST_GRAPH_AND_NODE_V(false);

	const IPipelineNode *candidate = graph->get_node(p_candidate);
	ERR_FAIL_NULL_V(candidate, false);

	return node->has_descendant(candidate);
}

bool BehaviorServer::node_parent_has_child(RID p_graph, RID p_node, RID p_child) {
	TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE_V(false);
	const IPipelineNode *child = graph->get_node(p_child);
	ERR_FAIL_NULL_V(child, false);

	const IPipelineNodeParent *parent = dynamic_cast<const IPipelineNodeParent *>(node);
	ERR_FAIL_NULL_V(parent, false);

	return parent->has_child(child);
}

bool BehaviorServer::node_composite_add_child(RID p_graph, RID p_node, RID p_child) {
	TRY_GET_GRAPH_COMPOSITE_AND_CHILD_V(false);
	ERR_FAIL_COND_V(child->has_descendant(node), false);
	bool result = composite->add_child_node(child);
	if (likely(result)) {
		graph->update_parent(child, composite);
	}
	return result;
}

bool BehaviorServer::node_composite_remove_child(RID p_graph, RID p_node, RID p_child) {
	TRY_GET_GRAPH_COMPOSITE_AND_CHILD_V(false);
	bool result = composite->remove_child_node(child);
	if (likely(result)) {
		graph->update_parent(child);
	}

	return false;
}

bool BehaviorServer::node_composite_remove_child_at(RID p_graph, RID p_node, int64_t p_child_index) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE_V(false);

	const IPipelineNode *child = composite->get_child_node(p_child_index);
	ERR_FAIL_NULL_V(child, false);
	bool result = composite->remove_child_node_at(p_child_index);
	
	if (likely(result)) {
		graph->update_parent(child);
	}

	return result;
}

void BehaviorServer::node_composite_clear(RID p_graph, RID p_node) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE();
	Vector<const IPipelineNode *> nodes = {};
	node->get_children(nodes);
	for(const IPipelineNode *child : nodes) {
		graph->update_parent(child);
	}
	composite->clear();
}

RID BehaviorServer::node_composite_get_child(RID p_graph, RID p_node, int64_t p_child_index) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE_V(RID());
	const IPipelineNode *child = composite->get_child_node(p_child_index);
	ERR_FAIL_NULL_V(child, RID());
	return child->get_id();
}

void BehaviorServer::node_composite_set_child(RID p_graph, RID p_node, int64_t p_child_index, RID p_child) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE();
	ERR_FAIL_INDEX(p_child_index, composite->child_count());

	const IPipelineNode *child = graph->get_node(p_child);
	ERR_FAIL_NULL(child);
	ERR_FAIL_COND(child->has_descendant(node));
	const IPipelineNode *prev_child = composite->get_child_node(p_child_index);
	if (likely(prev_child)) {
		graph->update_parent(prev_child);
	}
	graph->update_parent(child, composite);
	composite->set_child_node(p_child_index, child);
}

int64_t BehaviorServer::node_composite_child_count(RID p_graph, RID p_node) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE_V(0);
	return composite->child_count();
}

void BehaviorServer::node_composite_swap_children(RID p_graph, RID p_node, uint64_t p_first_index, uint64_t p_second_index) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE();
	composite->swap_child_nodes(p_first_index, p_second_index);
}

Error BehaviorServer::node_composite_insert_child(RID p_graph, RID p_node, int64_t p_pos, RID p_child) {
	TRY_GET_GRAPH_AND_COMPOSITE_NODE_V(Error::ERR_INVALID_PARAMETER);

	const IPipelineNode *child = graph->get_node(p_child);
	ERR_FAIL_NULL_V(child, Error::ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(child->has_descendant(node), Error::ERR_CYCLIC_LINK);

	Error result = composite->insert_child_node(p_pos, child);
	if (likely(result == godot::OK)) {
		graph->update_parent(child, composite);
	}
	return result;
}

void BehaviorServer::node_composite_append_children(RID p_graph, RID p_node, const TypedArray<RID> &p_children) {
	if (unlikely(p_children.is_empty())) {
		WARN_PRINT("Cannot append empty child vector!");
		return;
	}

	TRY_GET_GRAPH_AND_COMPOSITE_NODE();

	Vector<const IPipelineNode *> children = {};
	const int64_t child_count = p_children.size();
	for (int64_t idx = 0; idx < child_count; ++idx) {
		RID child_rid = p_children[idx];
		const IPipelineNode *child = graph->get_node(child_rid);
		ERR_CONTINUE(child == nullptr);
		ERR_CONTINUE(child->has_descendant(node));
		graph->update_parent(child, composite);
		children.push_back(child);
	}

	composite->append_child_nodes(children);
}

RID BehaviorServer::node_decorator_get_child(RID p_graph, RID p_node) {
	TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE_V(RID());
	const IPipelineNode *child = decorator->get_child();
	ERR_FAIL_NULL_V(child, RID());
	return child->get_id();
}

void BehaviorServer::node_decorator_set_child(RID p_graph, RID p_node, RID p_child) {
	TRY_GET_GRAPH_AND_DECORATOR_NODE();

	const IPipelineNode *child = nullptr;
	if (likely(p_child.is_valid())) {
		child = graph->get_node(p_child);
		ERR_FAIL_NULL(child);
		ERR_FAIL_COND(child->has_descendant(node));
		graph->update_parent(child);
	}

	const IPipelineNode *prev_child = decorator->get_child();
	if (unlikely(prev_child != nullptr)) {
		graph->update_parent(prev_child);
	}

	decorator->set_child(child);
}

#undef TRY_GET_CONST_GRAPH
#undef TRY_GET_GRAPH
#undef TRY_GET_CONST_NODE
#undef TRY_GET_NODE
#undef TRY_GET_CONST_GRAPH_AND_NODE
#undef TRY_GET_GRAPH_AND_NODE
#undef TRY_GET_GRAPH_AND_COMPOSITE_NODE
#undef TRY_GET_GRAPH_COMPOSITE_AND_CHILD
#undef TRY_GET_GRAPH_AND_DECORATOR_NODE
#undef TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE
#undef TRY_GET_GRAPH_EDITABLE
#undef TRY_GET_GRAPH_AND_NODE_EDITABLE

#undef TRY_GET_CONST_GRAPH_V
#undef TRY_GET_GRAPH_V
#undef TRY_GET_CONST_NODE_V
#undef TRY_GET_NODE_V
#undef TRY_GET_CONST_GRAPH_AND_NODE_V
#undef TRY_GET_GRAPH_AND_NODE_V
#undef TRY_GET_GRAPH_AND_COMPOSITE_NODE_V
#undef TRY_GET_GRAPH_COMPOSITE_AND_CHILD_V
#undef TRY_GET_GRAPH_AND_DECORATOR_NODE_V
#undef TRY_GET_CONST_GRAPH_AND_DECORATOR_NODE_V
#undef TRY_GET_GRAPH_EDITABLE_V
#undef TRY_GET_GRAPH_AND_NODE_EDITABLE_V

#undef BLACKBOARDS_LOCK_V
#undef BLACKBOARDS_LOCK
#undef GRAPHS_LOCK_V
#undef GRAPHS_LOCK

// Nodes END

// Pipelines

const StringName &BehaviorServer::pipeline_get_group_key(RID p_pipeline) {
	static const StringName empty = StringName();
	PIPELINES_LOCK_V(empty);
	IPipeline *pipeline = _pipeline_owner.get_or_null(p_pipeline);
	ERR_FAIL_NULL_V(pipeline, empty);

	return pipeline->group_key();
}

void BehaviorServer::pipeline_execute(RID p_pipeline) {
	PIPELINES_LOCK();
	IPipeline *pipeline = _pipeline_owner.get_or_null(p_pipeline);
	ERR_FAIL_NULL(pipeline);
	pipeline->execute();
}

void BehaviorServer::pipeline_halt(RID p_pipeline) {
	PIPELINES_LOCK();
	IPipeline *pipeline = _pipeline_owner.get_or_null(p_pipeline);
	ERR_FAIL_NULL(pipeline);
	pipeline->halt();
}

#undef PIPELINES_LOCK_V
#undef PIPELINES_LOCK

// Pipelines END

// ---- Behavior Tree END ----

HydrogenBehaviorServer *HydrogenBehaviorServer::singleton = nullptr;
HydrogenBehaviorServer *HydrogenBehaviorServer::get_singleton() {
	return singleton;
}

void HydrogenBehaviorServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("free_rid", "rid"), &HydrogenBehaviorServer::free_rid);
	ClassDB::bind_method(D_METHOD("blackboard_create"), &HydrogenBehaviorServer::blackboard_create);
	ClassDB::bind_method(D_METHOD("graph_create", "graph_group"), &HydrogenBehaviorServer::graph_create);
	ClassDB::bind_method(D_METHOD("pipeline_create", "graph", "blackboard"), &HydrogenBehaviorServer::pipeline_create);
	ClassDB::bind_method(D_METHOD("agent_create"), &HydrogenBehaviorServer::agent_create);

	// ---- Blackboard ----

	ADD_SIGNAL(MethodInfo("blackboard_created", PropertyInfo(Variant::RID, "blackboard_rid")));
	ADD_SIGNAL(MethodInfo("blackboard_destroyed", PropertyInfo(Variant::RID, "blackboard_rid")));
	ADD_SIGNAL(MethodInfo("blackboard_parent_set", PropertyInfo(Variant::RID, "child_rid"), PropertyInfo(Variant::RID, "parent_rid")));

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

	// ---- Blackboard End ----

	// ---- Graphs ----

	ADD_SIGNAL(MethodInfo("graph_created", PropertyInfo(Variant::RID, "graph")));
	ADD_SIGNAL(MethodInfo("graph_destroyed", PropertyInfo(Variant::RID, "graph")));

	
	ClassDB::bind_method(D_METHOD("graph_get_type", "graph"), &HydrogenBehaviorServer::graph_get_type);
	ClassDB::bind_method(D_METHOD("graph_get_group_key", "graph"), &HydrogenBehaviorServer::graph_get_group_key);
	ClassDB::bind_method(D_METHOD("graph_is_bound", "graph"), &HydrogenBehaviorServer::graph_is_bound);
	ClassDB::bind_method(D_METHOD("graph_get_bind_count", "graph"), &HydrogenBehaviorServer::graph_get_bind_count);
	ClassDB::bind_method(D_METHOD("graph_create_node", "graph", "node_type_name", "input_aliases", "output_aliases"), &HydrogenBehaviorServer::graph_create_node, DEFVAL(PortAliases()), DEFVAL(PortAliases()));
	ClassDB::bind_method(D_METHOD("graph_destroy_node", "graph", "node"), &HydrogenBehaviorServer::graph_destroy_node);
	ClassDB::bind_method(D_METHOD("graph_get_node_count", "graph"), &HydrogenBehaviorServer::graph_get_node_count);
	ClassDB::bind_method(D_METHOD("graph_get_sub_graphs", "graph"), &HydrogenBehaviorServer::graph_get_sub_graphs);
	ClassDB::bind_method(D_METHOD("graph_get_nodes", "graph"),&HydrogenBehaviorServer::graph_get_nodes);
	ClassDB::bind_method(D_METHOD("graph_set_root", "graph", "node"), &HydrogenBehaviorServer::graph_set_root);
	ClassDB::bind_method(D_METHOD("graph_get_root", "graph"), &HydrogenBehaviorServer::graph_get_root);
	ClassDB::bind_method(D_METHOD("graph_query_node", "graph", "node", "predicate"), &HydrogenBehaviorServer::graph_query_node);
	ClassDB::bind_method(D_METHOD("graph_query_nodes", "graph", "predicate"), &HydrogenBehaviorServer::graph_query_nodes);
	ClassDB::bind_method(D_METHOD("graph_get_rooted_statuses", "graph"), &HydrogenBehaviorServer::graph_get_rooted_statuses);
	ClassDB::bind_method(D_METHOD("graph_node_is_parented", "graph", "node"), &HydrogenBehaviorServer::graph_node_is_parented);
	// ---- Graphs End ----

	// ---- Nodes ----

	ClassDB::bind_method(D_METHOD("node_get_type_name", "graph", "node"), &HydrogenBehaviorServer::node_get_type_name);
	ClassDB::bind_method(D_METHOD("node_get_ports", "graph", "node"), &HydrogenBehaviorServer::node_get_ports);
	ClassDB::bind_method(D_METHOD("node_get_connections", "graph", "node"), &HydrogenBehaviorServer::node_get_connections);
	ClassDB::bind_method(D_METHOD("node_set_connection", "graph", "node", "name", "target_node"), &HydrogenBehaviorServer::node_set_connection);
	ClassDB::bind_method(D_METHOD("node_get_connection", "graph", "node", "name"), &HydrogenBehaviorServer::node_get_connection);
	ClassDB::bind_method(D_METHOD("node_is_parent", "graph", "node"), &HydrogenBehaviorServer::node_is_parent);
	ClassDB::bind_method(D_METHOD("node_is_composite", "graph", "node"), &HydrogenBehaviorServer::node_is_composite);
	ClassDB::bind_method(D_METHOD("node_is_decorator", "graph", "node"), &HydrogenBehaviorServer::node_is_decorator);
	ClassDB::bind_method(D_METHOD("node_set_name", "graph", "node", "name"), &HydrogenBehaviorServer::node_set_name);
	ClassDB::bind_method(D_METHOD("node_get_name", "graph", "node"), &HydrogenBehaviorServer::node_get_name);
	ClassDB::bind_method(D_METHOD("node_set_input_aliases", "graph", "node", "aliases"), &HydrogenBehaviorServer::node_set_input_aliases);
	ClassDB::bind_method(D_METHOD("node_get_input_alias", "graph", "node"), &HydrogenBehaviorServer::node_get_input_aliases);
	ClassDB::bind_method(D_METHOD("node_set_output_aliases", "graph", "node", "aliases"), &HydrogenBehaviorServer::node_set_output_aliases);
	ClassDB::bind_method(D_METHOD("node_get_output_aliases", "graph", "node"), &HydrogenBehaviorServer::node_get_output_aliases);
	ClassDB::bind_method(D_METHOD("node_supports_children", "graph", "node"), &HydrogenBehaviorServer::node_supports_children);
	ClassDB::bind_method(D_METHOD("node_has_children", "graph", "node"), &HydrogenBehaviorServer::node_has_children);
	ClassDB::bind_method(D_METHOD("node_get_children", "graph", "node"), &HydrogenBehaviorServer::node_get_children);
	ClassDB::bind_method(D_METHOD("node_get_descendants", "graph", "node"), &HydrogenBehaviorServer::node_get_descendants);
	ClassDB::bind_method(D_METHOD("node_has_child", "graph", "node", "candidate"), &HydrogenBehaviorServer::node_has_child);
	ClassDB::bind_method(D_METHOD("node_has_descendant", "graph", "node", "candidate"), &HydrogenBehaviorServer::node_has_descendant);
	ClassDB::bind_method(D_METHOD("node_parent_has_child", "graph", "node", "child"), &HydrogenBehaviorServer::node_parent_has_child);
	ClassDB::bind_method(D_METHOD("node_composite_add_child", "graph", "node", "child"), &HydrogenBehaviorServer::node_composite_add_child);
	ClassDB::bind_method(D_METHOD("node_composite_remove_child", "graph", "node", "child"), &HydrogenBehaviorServer::node_composite_remove_child);
	ClassDB::bind_method(D_METHOD("node_remove_child_at", "graph", "node", "child_index"), &HydrogenBehaviorServer::node_composite_remove_child_at);
	ClassDB::bind_method(D_METHOD("node_composite_clear", "graph", "node"), &HydrogenBehaviorServer::node_composite_clear);
	ClassDB::bind_method(D_METHOD("node_composite_get_child", "graph", "node", "child_index"), &HydrogenBehaviorServer::node_composite_get_child);
	ClassDB::bind_method(D_METHOD("node_composite_set_child", "graph", "node", "child_index", "child"), &HydrogenBehaviorServer::node_composite_set_child);
	ClassDB::bind_method(D_METHOD("node_composite_swap_children", "graph", "node", "first_index", "second_index"), &HydrogenBehaviorServer::node_composite_swap_children);
	ClassDB::bind_method(D_METHOD("node_composite_insert_child"), &HydrogenBehaviorServer::node_composite_insert_child);
	ClassDB::bind_method(D_METHOD("node_composite_append_children", "graph", "node", "children"), &HydrogenBehaviorServer::node_composite_append_children);
	ClassDB::bind_method(D_METHOD("node_decorator_get_child", "graph", "node"), &HydrogenBehaviorServer::node_decorator_get_child);
	ClassDB::bind_method(D_METHOD("node_decorator_set_child", "graph", "node", "child"), &HydrogenBehaviorServer::node_decorator_set_child);

	// ---- Nodes End ----

	// ---- Pipelines ----

	ADD_SIGNAL(MethodInfo("pipeline_created", PropertyInfo(Variant::RID, "tree")));
	ADD_SIGNAL(MethodInfo("pipeline_destroyed", PropertyInfo(Variant::RID, "tree")));

	ClassDB::bind_method(D_METHOD("pipeline_get_group_key", "pipeline"), &HydrogenBehaviorServer::pipeline_get_group_key);

	ClassDB::bind_method(D_METHOD("pipeline_execute", "pipeline"), &HydrogenBehaviorServer::pipeline_execute);
	ClassDB::bind_method(D_METHOD("pipeline_halt", "pipeline"), &HydrogenBehaviorServer::pipeline_halt);

	ClassDB::bind_method(D_METHOD("pipeline_get_execution_blackboard", "pipeline"), &HydrogenBehaviorServer::pipeline_get_execution_blackboard);
	ClassDB::bind_method(D_METHOD("pipeline_get_source_blackboard", "pipeline"), &HydrogenBehaviorServer::pipeline_get_source_blackboard);
	ClassDB::bind_method(D_METHOD("pipeline_get_graph", "pipeline"), &HydrogenBehaviorServer::pipeline_get_graph);
	ClassDB::bind_method(D_METHOD("pipeline_get_error", "pipeline"), &HydrogenBehaviorServer::pipeline_get_error);

	// ---- Pipelines End ----

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

RID HydrogenBehaviorServer::graph_create(const StringName &p_pipeline_group) {
	return BehaviorServer::get_singleton()->graph_create(p_pipeline_group);
}

RID HydrogenBehaviorServer::behavior_tree_graph_create() {
	return BehaviorServer::get_singleton()->behavior_tree_graph_create();
}

RID HydrogenBehaviorServer::pipeline_create(RID p_graph, RID p_blackboard) {
	return BehaviorServer::get_singleton()->pipeline_create(p_graph, p_blackboard);
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

const StringName &HydrogenBehaviorServer::graph_get_type(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_type(p_graph);
}

const StringName &HydrogenBehaviorServer::graph_get_group_key(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_group_key(p_graph);
}

bool HydrogenBehaviorServer::graph_is_bound(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_is_bound(p_graph);
}

uint32_t HydrogenBehaviorServer::graph_get_bind_count(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_bind_count(p_graph);
}

RID HydrogenBehaviorServer::graph_create_node(RID p_graph, const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases) {
	return BehaviorServer::get_singleton()->graph_create_node(p_graph, p_node_type_name, p_input_aliases, p_output_aliases);
}

bool HydrogenBehaviorServer::graph_destroy_node(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->graph_destroy_node(p_graph, p_node);
}

uint64_t HydrogenBehaviorServer::graph_get_node_count(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_node_count(p_graph);
}

TypedArray<RID> HydrogenBehaviorServer::graph_get_sub_graphs(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_sub_graphs(p_graph);
}

TypedArray<RID> HydrogenBehaviorServer::graph_get_nodes(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_nodes(p_graph);
}

bool HydrogenBehaviorServer::graph_set_root(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->graph_set_root(p_graph, p_node);
}

RID HydrogenBehaviorServer::graph_get_root(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_root(p_graph);
}

TypedArray<RID> HydrogenBehaviorServer::graph_query_node(RID p_graph, RID p_node, Callable p_predicate) {
	return BehaviorServer::get_singleton()->graph_query_node(p_graph, p_node, p_predicate);
}

TypedArray<RID> HydrogenBehaviorServer::graph_query_nodes(RID p_graph, Callable p_predicate) {
	return BehaviorServer::get_singleton()->graph_query_nodes(p_graph, p_predicate);
}

TypedArray<Dictionary> HydrogenBehaviorServer::graph_get_rooted_statuses(RID p_graph) {
	return BehaviorServer::get_singleton()->graph_get_rooted_statuses(p_graph);
}

bool HydrogenBehaviorServer::graph_node_is_parented(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->graph_node_is_parented(p_graph, p_node);
}

// ---- Graphs END ----

// ---- Nodes ----

const StringName &HydrogenBehaviorServer::node_get_type_name(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_type_name(p_graph, p_node);
}

TypedArray<Dictionary> HydrogenBehaviorServer::node_get_ports(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_ports(p_graph, p_node);
}

TypedArray<Dictionary> HydrogenBehaviorServer::node_get_connections(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_connections(p_graph, p_node);
}

bool HydrogenBehaviorServer::node_set_connection(RID p_graph, RID p_node, const StringName &p_name, RID p_target_node) {
	return BehaviorServer::get_singleton()->node_set_connection(p_graph, p_node, p_name, p_target_node);
}

RID HydrogenBehaviorServer::node_get_connection(RID p_graph, RID p_node, const StringName &p_name) {
	return BehaviorServer::get_singleton()->node_get_connection(p_graph, p_node, p_name);
}

bool HydrogenBehaviorServer::node_is_parent(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_is_parent(p_graph, p_node);
}

bool HydrogenBehaviorServer::node_is_composite(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_is_composite(p_graph, p_node);
}

bool HydrogenBehaviorServer::node_is_decorator(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_is_decorator(p_graph, p_node);
}

void HydrogenBehaviorServer::node_set_name(RID p_graph, RID p_node, const String &p_name) {
	BehaviorServer::get_singleton()->node_set_name(p_graph, p_node, p_name);
}

const String &HydrogenBehaviorServer::node_get_name(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_name(p_graph, p_node);
}

void HydrogenBehaviorServer::node_set_input_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases) {
	BehaviorServer::get_singleton()->node_set_input_aliases(p_graph, p_node, p_aliases);
}

PortAliases HydrogenBehaviorServer::node_get_input_aliases(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->get_input_aliases(p_graph, p_node);
}

void HydrogenBehaviorServer::node_set_output_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases) {
	BehaviorServer::get_singleton()->node_set_output_aliases(p_graph, p_node, p_aliases);
}

PortAliases HydrogenBehaviorServer::node_get_output_aliases(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_output_aliases(p_graph, p_node);
}

bool HydrogenBehaviorServer::node_supports_children(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_supports_children(p_graph, p_node);
}

bool HydrogenBehaviorServer::node_has_children(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_has_children(p_graph, p_node);
}

TypedArray<RID> HydrogenBehaviorServer::node_get_children(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_children(p_graph, p_node);
}

TypedArray<RID> HydrogenBehaviorServer::node_get_descendants(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_get_descendants(p_graph, p_node);
}

bool HydrogenBehaviorServer::node_has_child(RID p_graph, RID p_node, RID p_candidate) {
	return BehaviorServer::get_singleton()->node_has_child(p_graph, p_node, p_candidate);
}

bool HydrogenBehaviorServer::node_has_descendant(RID p_graph, RID p_node, RID p_candidate) {
	return BehaviorServer::get_singleton()->node_has_descendant(p_graph, p_node, p_candidate);
}

bool HydrogenBehaviorServer::node_parent_has_child(RID p_graph, RID p_node, RID p_child) {
	return BehaviorServer::get_singleton()->node_parent_has_child(p_graph, p_node, p_child);
}

bool HydrogenBehaviorServer::node_composite_add_child(RID p_graph, RID p_node, RID p_child) {
	return BehaviorServer::get_singleton()->node_composite_add_child(p_graph, p_node, p_child);
}

bool HydrogenBehaviorServer::node_composite_remove_child(RID p_graph, RID p_node, RID p_child) {
	return BehaviorServer::get_singleton()->node_composite_remove_child(p_graph, p_node, p_child);
}

bool HydrogenBehaviorServer::node_composite_remove_child_at(RID p_graph, RID p_node, int64_t p_child_index) {
	return BehaviorServer::get_singleton()->node_composite_remove_child_at(p_graph, p_node, p_child_index);
}

void HydrogenBehaviorServer::node_composite_clear(RID p_graph, RID p_node) {
	BehaviorServer::get_singleton()->node_composite_clear(p_graph, p_node);
}

RID HydrogenBehaviorServer::node_composite_get_child(RID p_graph, RID p_node, int64_t p_child_index) {
	return BehaviorServer::get_singleton()->node_composite_get_child(p_graph, p_node, p_child_index);
}

void HydrogenBehaviorServer::node_composite_set_child(RID p_graph, RID p_node, int64_t p_child_index, RID p_child) {
	BehaviorServer::get_singleton()->node_composite_set_child(p_graph, p_node, p_child_index, p_child);
}

int64_t HydrogenBehaviorServer::node_composite_child_count(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_composite_child_count(p_graph, p_node);
}

void HydrogenBehaviorServer::node_composite_swap_children(RID p_graph, RID p_node, uint64_t p_first_index, uint64_t p_second_index) {
	BehaviorServer::get_singleton()->node_composite_swap_children(p_graph, p_node, p_first_index, p_second_index);
}

Error HydrogenBehaviorServer::node_composite_insert_child(RID p_graph, RID p_node, int64_t p_pos, RID p_child) {
	return BehaviorServer::get_singleton()->node_composite_insert_child(p_graph, p_node, p_pos, p_child);
}

void HydrogenBehaviorServer::node_composite_append_children(RID p_graph, RID p_node, const TypedArray<RID> &p_children) {
	BehaviorServer::get_singleton()->node_composite_append_children(p_graph, p_node, p_children);
}

RID HydrogenBehaviorServer::node_decorator_get_child(RID p_graph, RID p_node) {
	return BehaviorServer::get_singleton()->node_decorator_get_child(p_graph, p_node);
}

void HydrogenBehaviorServer::node_decorator_set_child(RID p_graph, RID p_node, RID p_child) {
	BehaviorServer::get_singleton()->node_decorator_set_child(p_graph, p_node, p_child);
}

// ---- Nodes END ----

// ---- Pipelines ----

void HydrogenBehaviorServer::_pipeline_emit_created(RID p_pipeline) {
	emit_signal("pipeline_created", p_pipeline);
}

void HydrogenBehaviorServer::_pipeline_emit_destroyed(RID p_pipeline) {
	emit_signal("pipeline_destroyed", p_pipeline);
}

const StringName &HydrogenBehaviorServer::pipeline_get_group_key(RID p_pipeline) {
	return BehaviorServer::get_singleton()->pipeline_get_group_key(p_pipeline);
}

void HydrogenBehaviorServer::pipeline_execute(RID p_pipeline) {
	BehaviorServer::get_singleton()->pipeline_execute(p_pipeline);
}

void HydrogenBehaviorServer::pipeline_halt(RID p_pipeline) {
	BehaviorServer::get_singleton()->pipeline_halt(p_pipeline);
}

RID HydrogenBehaviorServer::pipeline_get_execution_blackboard(RID p_pipeline) {
	return BehaviorServer::get_singleton()->pipeline_get_execution_blackboard(p_pipeline);
}

RID HydrogenBehaviorServer::pipeline_get_source_blackboard(RID p_pipeline) {
	return BehaviorServer::get_singleton()->pipeline_get_source_blackboard(p_pipeline);
}

RID HydrogenBehaviorServer::pipeline_get_graph(RID p_pipeline) {
	return BehaviorServer::get_singleton()->pipeline_get_graph(p_pipeline);
}

String HydrogenBehaviorServer::pipeline_get_error(RID p_pipeline) {
	return BehaviorServer::get_singleton()->pipeline_get_error(p_pipeline);
}

// ---- Pipelines END ----

#if TESTS_ENABLED
void HydrogenBehaviorServer::run_tests() {
	tests::behavior_test_runner();
}
#endif
// ReSharper restore CppMemberFunctionMayBeStatic

}
