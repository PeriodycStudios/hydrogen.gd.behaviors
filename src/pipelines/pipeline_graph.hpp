//
// Created by tkey on 7/31/25.
//
#pragma once

#include "mutex_helpers.hpp"
#include "node_interfaces.hpp"
#include "pipeline_node.hpp"
#include "rid_data.hpp"

#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/pair.hpp>

#include <cstdint>
#include <type_traits>
#include <atomic>
#include <mutex>
#include <functional>

namespace hydrogen::pipelines {

template<typename TNODE, typename = void>
class PipelineGraph {};

template <typename TNODE>
class PipelineGraph<TNODE, std::enable_if_t<std::is_base_of_v<PipelineNode, TNODE>>> : public RidData, public IPipelineGraph {
public:
	typedef const TNODE * c_ptr;
	typedef TNODE * ptr;

	typedef std::function<bool(c_ptr)> NodePredicate;

private:
	static HashMap<StringName, std::function<ptr()>> _registered_nodes;
	static std::mutex *_register_mutex;

	std::mutex *_mutex;
	std::atomic_uint32_t _bind_count = { 0 };

	StringName _plugin_name;

protected:

	HashMap<RID, ptr> _nodes = {};
	HashMap<RID, IPipelineNodeParent *> _parent_lookup = {};
	Vector<TNODE *> _connectors = {};
	RID_PtrOwner<TNODE> _nodes_owner = {};
	ptr _root = nullptr;

	PipelineGraph(const StringName &p_group_key) : _plugin_name(p_group_key), _mutex(memnew(std::mutex)) {}

	_FORCE_INLINE_ static void _init() {
		_register_mutex = memnew(std::mutex);
	}

	_FORCE_INLINE_ static void _finish() {
		memdelete(_register_mutex);
		_registered_nodes.clear();
	}

	c_ptr _get_node(RID p_node) const {
		auto iter = _nodes.find(p_node);
		if (likely(iter != _nodes.end())) {
			return iter->value;
		}
		else {
			return nullptr;
		}
	}

	ptr _get_node(RID p_node) {
		auto iter = _nodes.find(p_node);
		if (likely(iter != _nodes.end())) {
			return iter->value;
		}
		else {
			return nullptr;
		}
	}

	void _visit(c_ptr p_node, std::function<void(c_ptr)> p_visitor) const {
		Vector<const TNODE *> nodes = { p_node };
		p_node->get_descendants(nodes);

		for (c_ptr node : nodes) {
			p_visitor(node);
		}
	}

	void _visit_all(std::function<void(const TNODE *)> p_visitor) const {
		for (const auto &kvp : _nodes) {
			c_ptr node = kvp.value;
			p_visitor(node);
		}
	}

	RID _create_node(const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases) override {
		LOCK_TWO_V(_mutex, _register_mutex, RID());

		const auto iter = _registered_nodes.find(p_node_type_name);
        ERR_FAIL_COND_V(iter == _registered_nodes.end(), RID());

		ptr node = iter->value();
		ERR_FAIL_NULL_V(node, RID());
		RID rid = _nodes_owner.make_rid(node);
		node->set_self(rid);
		_nodes[rid] = node;
		node->set_input_aliases(p_input_aliases);
		node->set_output_aliases(p_output_aliases);

		const Vector<NodeConnectionInfo> &connections = node->get_connections();
		if (unlikely(!connections.is_empty())) {
			_connectors.push_back(node);
		}
		
        return rid;
	}

	void _clear_connections(TNODE *p_node) {
		for (ptr connector : _connectors) {
			const auto &connections = connector->get_connections();
			for (const auto &info : connections) {
				if (unlikely(connector->get_connection(info.name) == p_node)) {
					connector->set_connection(info.name, nullptr);
				}
			}
		}
		_connectors.erase(p_node);
	}

	void _clear_parentage(TNODE *p_node) {
		IPipelineNodeParent *parent = dynamic_cast<IPipelineNodeParent *>(p_node);
		if (unlikely(parent)) {
			Vector<const TNODE *> children = {};
			p_node->get_children(children);
			for (const TNODE *child : children) {
				_parent_lookup.erase(child->get_id());
			}
			parent->remove_all_child_nodes();
		}

		RID rid = p_node->get_id();
		auto iter = _parent_lookup.find(rid);
		if (likely(iter != _parent_lookup.end())) {
			IPipelineNodeParent *parent = iter->value;
			parent->remove_child_node(p_node);
			_parent_lookup.erase(rid);
		}
	}

	bool _destroy_node(RID p_node) override {
		LOCK_ONE_V(_mutex, false);
		
		ptr node = _get_node(p_node);
		ERR_FAIL_NULL_V(node, false);

		_clear_connections(node);
		_clear_parentage(node);
		
		RID rid = node->get_id();
		_nodes.erase(rid);
		_nodes_owner.free(rid);
		memdelete(node);
		return true;
	}

	template<typename U>
	static void _register_node() {
		if constexpr (!std::is_base_of_v<TNODE, U>) {
			ERR_PRINT("Unable to register invalid node type!");
			return;
		}
		else {
			StringName node_type_name = U::get_node_name();
			auto iter = _registered_nodes.find(node_type_name);
			if (likely(iter == _registered_nodes.end())) {
				auto create_func = []() { return memnew(U); };
				_registered_nodes[node_type_name] = create_func;
			}
			else {
				WARN_PRINT_ED("Node type already registered");
			}
		}
	}

	template<typename U>
	static void _unregister_node() {
		if constexpr (!std::is_base_of_v<TNODE, U>) {
			ERR_PRINT("Unable to unregister invalid node type!");
			return;
		}
		else {
			StringName node_type_name = U::get_node_name();
			auto iter = _registered_nodes.find(node_type_name);
			if (likely(iter != _registered_nodes.end())) {
				_registered_nodes.erase(iter);
			}
			else {
				WARN_PRINT_ED("Node type not registered!");
			}
		}
	}

	static LocalVector<StringName> _get_registered_node_type_names() {
		LOCK_ONE_V(_register_mutex, {});
		
		LocalVector<StringName> type_names = {};
		type_names.resize(_registered_nodes.size());
		int32_t idx = 0;
		for (const auto &kvp : _registered_nodes) {
			type_names[idx++] = kvp.key;
		}
		return type_names;
	}

public:

	_FORCE_INLINE_ void bind() { ++_bind_count; }
	_FORCE_INLINE_ void unbind() {
		CRASH_COND(_bind_count == 0);
		--_bind_count;
	}

	~PipelineGraph() override {
		_root = nullptr;
		for (const auto &kvp : _nodes) {
			RID rid = kvp.key;
			ptr node = kvp.value;
			_nodes_owner.free(rid);
			memdelete(node);
		}
		_nodes.clear();
		_connectors.clear();
		_parent_lookup.clear();
		memdelete(_mutex);
	}

	[[nodiscard]] const StringName &plugin_name() const override {
		return _plugin_name;
	}

	[[nodiscard]] RID get_id() const override { return get_self(); }
	
	[[nodiscard]] bool is_bound() const override { return _bind_count > 0; }
	[[nodiscard]] uint32_t bind_count() const override { return _bind_count; }

	[[nodiscard]] RID create_node(const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases) override {
		ERR_FAIL_COND_V_MSG(is_bound(), RID(), "Cannoot create nodes while bound!");
		return _create_node(p_node_type_name, p_input_aliases, p_output_aliases);
	}

	bool destroy_node(RID p_node) override {
		ERR_FAIL_COND_V_MSG(is_bound(), false, "Cannot destroy nodes while bound!");
		return _destroy_node(p_node);
	}

	[[nodiscard]] uint64_t nodes_count() const override {
		return _nodes.size();
	}

	void get_sub_graphs(Vector<const IPipelineGraph *> &p_graphs) const override {
		LOCK_ONE(_mutex);

		auto visitor = [&p_graphs](const TNODE *node) {
			const IPipelineNodeSubGraph *sub_graph_node = dynamic_cast<const IPipelineNodeSubGraph *>(node);
			if (unlikely(sub_graph_node != nullptr)) {
				const IPipelineGraph *graph = sub_graph_node->get_sub_graph();
				if (likely(graph != nullptr)) {
					p_graphs.push_back(graph);
				}
			}
		};

		_visit_all(visitor);
	}

	void get_nodes(Vector<const IPipelineNode *> &p_nodes) const override {
		LOCK_ONE(_mutex);

		for (const auto &kvp : _nodes) {
			p_nodes.push_back(kvp.value);
		}
	}

	[[nodiscard]] _FORCE_INLINE_ c_ptr get_node_typed(RID p_node) const {
		LOCK_ONE_V(_mutex, nullptr);
		return _get_node(p_node);
	}

	[[nodiscard]] _FORCE_INLINE_ ptr get_node_typed(RID p_node) {
		LOCK_ONE_V(_mutex, nullptr);
		return _get_node(p_node);
	}

	[[nodiscard]] const IPipelineNode* get_node(RID p_node) const override {
		return get_node_typed(p_node);
	}

	[[nodiscard]] IPipelineNode *get_node(RID p_node) override {
		return get_node_typed(p_node);
	}

	[[nodiscard]] _FORCE_INLINE_ c_ptr get_root_typed() const {
		LOCK_ONE_V(_mutex, nullptr);
		return _root;
	}

	[[nodiscard]] _FORCE_INLINE_ ptr get_root_typed() {
		LOCK_ONE_V(_mutex, nullptr);
		return _root;
	}

	[[nodiscard]] const IPipelineNode *get_root_node() const override {
		return get_root_typed();
	}

	[[nodiscard]] IPipelineNode *get_root_node() override {
		return get_root_typed();
	}

	bool set_root(RID p_node) override {
		ERR_FAIL_COND_V_EDMSG(is_bound(), false, "Cannot modify graph while it is bound.");

		LOCK_ONE_V(_mutex, false);
		auto iter = _nodes.find(p_node);
		ERR_FAIL_COND_V(iter == _nodes.end(), false);
		TNODE *node = iter->value;

		_root = node;
		return true;
	}

	[[nodiscard]] RID get_root() const override {
		LOCK_ONE_V(_mutex, RID());
		return _root != nullptr ? _root->get_self() : RID();
	}

	typedef std::function<bool(const TNODE *)> TypedNodePredicate;

	void query_node(RID p_node_id, Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const override {
		LOCK_ONE(_mutex);

		c_ptr node = _get_node(p_node_id);
		ERR_FAIL_NULL(node);

		auto visitor = [&p_nodes, &p_predicate](c_ptr n) {
			if (unlikely(p_predicate(n))) {
				p_nodes.push_back(n);
			}
		};
		
		_visit(node, visitor);
	}

	void query_node_typed(RID p_node_id, Vector<c_ptr> &p_nodes, TypedNodePredicate p_predicate) const {
		LOCK_ONE(_mutex);

		c_ptr node = _get_node(p_node_id);
		ERR_FAIL_NULL(node);

		auto visitor = [&p_nodes, &p_predicate](const TNODE *n) {
			if (unlikely(p_predicate(n))) {
				p_nodes.push_back(n);
			}
		};

		_visit(node, visitor);
	}

	void query_nodes(Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const override {
		LOCK_ONE(_mutex);
		
		auto visitor = [&p_nodes, &p_predicate](const TNODE *node) {
			if (unlikely(p_predicate(node))) {
				p_nodes.push_back(node);
			}
		};

		_visit_all(visitor);
	}

	void query_nodes_typed(Vector<c_ptr> &p_nodes, TypedNodePredicate p_predicate) const {
		LOCK_ONE(_mutex);

		auto visitor = [&p_nodes, &p_predicate](c_ptr n) {
			if (unlikely(p_predicate(n))) {
				p_nodes.push_back(n);
			}
		};

		_visit_all(visitor);
	}

	void get_rooted_statuses(Vector<Pair<const IPipelineNode *, bool>> &p_statuses) const override {
		LOCK_ONE(_mutex);

		if (unlikely(_nodes.is_empty())) {
			return;
		}

		if (unlikely(_root == nullptr)) {
			for (const auto &kvp : _nodes) {
				const IPipelineNode *node = kvp.value;
				p_statuses.push_back(Pair(node, false));
			}
			return;
		}

		HashSet<RID> rooted_ids = {};
		
		auto visitor = [&rooted_ids](const TNODE * node) {
			rooted_ids.insert(node->get_id());
		};

		_visit(_root, visitor);

		for(const auto &kvp : _nodes) {
			const IPipelineNode *node = kvp.value;
			p_statuses.push_back(Pair(node, rooted_ids.has(node->get_id())));
		}
	}

	[[nodiscard]] bool is_parented(const IPipelineNode *p_node) const override {
		c_ptr node = dynamic_cast<c_ptr>(p_node);
		ERR_FAIL_NULL_V(node, false);

		return is_parented_typed(node);
	}

	[[nodiscard]] bool is_parented_typed(c_ptr p_node) const {
		LOCK_ONE_V(_mutex, false);

		if (unlikely(p_node == nullptr)) {
			return false;
		}

		if (unlikely(_parent_lookup.is_empty())) {
			return false;
		}

		return _parent_lookup.has(p_node->get_id());
	}

	void update_parent(const IPipelineNode *p_node, IPipelineNodeParent* p_parent = nullptr) override {
		c_ptr node = dynamic_cast<c_ptr>(p_node);
		ERR_FAIL_NULL(node);

		update_parent_typed(node);
	}

	void update_parent_typed(c_ptr p_node, IPipelineNodeParent *p_parent = nullptr) {
		LOCK_ONE(_mutex);

		RID rid = p_node->get_id();

		const bool needs_removal = p_parent != nullptr;

		auto iter = _parent_lookup.find(rid);
		if (likely(iter != _parent_lookup.end())) {
			IPipelineNodeParent *previous = iter->value;
			if (unlikely(needs_removal)) {
				previous->remove_child_node(p_node);
			}

			if (unlikely(p_parent == nullptr)) {
				_parent_lookup.remove(iter);
			}
			else {
				iter->value = p_parent;
			}
		}
		else if (likely(p_parent != nullptr)) {
			_parent_lookup.insert(rid, p_parent);
		}
	}
};

template<typename T>
HashMap<StringName, std::function<T *()>> PipelineGraph<T, std::enable_if_t<std::is_base_of_v<PipelineNode, T>>>::_registered_nodes = {};

template<typename T>
std::mutex *PipelineGraph<T, std::enable_if_t<std::is_base_of_v<PipelineNode, T>>>::_register_mutex = nullptr;

#define DECLARE_PIPELINE_GRAPH()													\
public:																				\
	_FORCE_INLINE_ static void init() {												\
		_init();																	\
		register_core_nodes();														\
	}																				\
																					\
	_FORCE_INLINE_ static void finish() {											\
		_finish();																	\
	}																				\
																					\
	template<typename U>															\
	_FORCE_INLINE_ static void register_node() {									\
		_register_node<U>();														\
	}																				\
																					\
	template<typename U>															\
	_FORCE_INLINE_ static void unregister_node() {									\
		_unregister_node<U>();														\
	}																				\
																					\
	_FORCE_INLINE_ static LocalVector<StringName> get_registered_node_type_names() {\
		return _get_registered_node_type_names();									\
	}																				\
private:																			\

} // hydrogen
