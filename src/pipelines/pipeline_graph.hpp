//
// Created by tkey on 7/31/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP


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

class Pipeline;

template<typename T, typename = void>
class PipelineGraph {};

template <typename T>
class PipelineGraph<T, std::enable_if_t<std::is_base_of_v<PipelineNode, T>>> : public RidData, public IPipelineGraph {

	static HashMap<StringName, std::function<T *()>> _registered_nodes;
	static std::mutex *_register_mutex;

public:
	typedef std::function<bool(const T *)> NodePredicate;

private:
	std::mutex *_mutex;
	std::atomic_uint32_t _bind_count = { 0 };

protected:
	HashMap<RID, T*> _nodes = {};
	HashMap<RID, RID> _child_to_parent = {};
	RID_PtrOwner<T> _nodes_owner = {};
	T *_root = nullptr;

	PipelineGraph() : _mutex(memnew(std::mutex)) {}

	_FORCE_INLINE_ static void _init() {
		_register_mutex = memnew(std::mutex);
	}

	_FORCE_INLINE_ static void _finish() {
		memdelete(_register_mutex);
		_registered_nodes.clear();
	}

	friend class Pipeline;
	_FORCE_INLINE_ void _bind() { ++_bind_count; }
	_FORCE_INLINE_ void _unbind() {
		CRASH_COND(_bind_count == 0);
		--_bind_count;
	}

	const T *_get_node(RID p_node) const {
		auto iter = _nodes.find(p_node);
		if (likely(iter != _nodes.end())) {
			return iter->value;
		}
		else {
			return nullptr;
		}
	}

	T *_get_node(RID p_node) {
		auto iter = _nodes.find(p_node);
		if (likely(iter != _nodes.end())) {
			return iter->value;
		}
		else {
			return nullptr;
		}
	}

	void _visit(const T * p_node, std::function<void(const T *)> p_visitor) const {
		Vector<const T *> nodes = { p_node };
		p_node->get_descendants(nodes);

		for (const T * node : nodes) {
			p_visitor(node);
		}
	}

	void _visit_all(std::function<void(const T *)> p_visitor) const {
		for (const auto &kvp : _nodes) {
			const T *node = kvp.value;
			p_visitor(node);
		}
	}

	void _unparent_helper(const T *p_node, RID p_parent_id) {
		T *parent = _get_node(p_parent_id);

		if (likely(parent)) {
			IPipelineNodeComposite *composite = dynamic_cast<IPipelineNodeComposite *>(parent);
			if (likely(composite != nullptr)) {
				bool result = composite->remove_child_node(p_node);
				if (unlikely(!result)) {
					ERR_PRINT("Node already removed, this shouldn't happen.");
				}
			}

			IPipelineNodeDecorator *decorator = dynamic_cast<IPipelineNodeDecorator *>(parent);
			if (unlikely(decorator != nullptr)) {
				decorator->set_node(nullptr);
			}
		}
		else {
			ERR_PRINT("Couldn't find parent node.");
		}
		_child_to_parent.erase(p_node->get_self());
	}

	void _unparent(const T *p_node) {
		auto parent_rid_iter = _child_to_parent.find(p_node->get_self());
		if (likely(parent_rid_iter != _child_to_parent.end())) {
			_unparent_helper(p_node, parent_rid_iter->value);
		}
	}

	void _reparent(const T *p_parent, const T *p_node) {

		RID node_rid = p_node->get_self();
		auto iter = _child_to_parent.find(node_rid);
		if (likely(iter != _child_to_parent.end())) {
			_unparent_helper(p_node, iter->value);
		}

		_child_to_parent[node_rid] = p_parent->get_self();
	}

	RID _create_node(const StringName &p_node_type_name) override {
		LOCK_TWO_V(_mutex, _register_mutex, RID());

		const auto iter = _registered_nodes.find(p_node_type_name);
        ERR_FAIL_COND_V(iter == _registered_nodes.end(), RID());

		T *node = iter->value();
		RID rid = _nodes_owner.make_rid(node);
		node->set_self(rid);
		_nodes[rid] = node;
		
        return rid;
	}

	bool _destroy_node(RID p_node) override {
		LOCK_ONE_V(_mutex, false);
		
		T *node = _get_node(p_node);
		ERR_FAIL_NULL_V(node, false);

		_unparent(node);

		Vector<const T*> children = {};
		node->get_children(children);
		for(const IPipelineNode* child : children) {
			RID child_rid = child->get_id();
			_child_to_parent.erase(child_rid);
		}

		RID rid = node->get_id();
		_nodes.erase(rid);
		_nodes_owner.free(rid);
		memdelete(node);
		return true;
	}

	template<typename U, typename = void>
	static void _register_node() {}

	template<typename U,
		std::enable_if_t<
			std::is_base_of_v<T, U> &&
			!std::is_abstract_v<U>>
	>
	static void _register_node() {
		StringName node_type_name = U::get_node_name();
		auto iter = _registered_nodes.find(node_type_name);
		if (iter == _registered_nodes.end()) {
			auto create_func = []() { return memnew(U); };
			_registered_nodes[node_type_name] = create_func;
		}
		else {
			WARN_PRINT_ED("Node type already registered");
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

	~PipelineGraph() override {
		_root = nullptr;
		for (const auto &kvp : _nodes) {
			RID rid = kvp.key;
			IPipelineNode *node = kvp.value;
			_nodes_owner.free(rid);
			memdelete(node);
		}
		
		_nodes.clear();
		memdelete(_mutex);
	}

	RID create_node(const StringName &p_node_type_name) override {
		ERR_FAIL_COND_V_MSG(is_bound(), RID(), "Cannot create nodes while bound!");
		return _create_node(p_node_type_name);
	}

	bool destroy_node(RID p_node) override {
		ERR_FAIL_COND_V_MSG(is_bound(), false, "Cannot destroy nodes while bound!");
		return _destroy_node(p_node);
	}

	RID get_id() const override { return get_self(); }
	
	[[nodiscard]] bool is_bound() const override { return _bind_count > 0; }

	void update_node(const IPipelineNode *p_node, const IPipelineNode *p_parent = nullptr) override {
		LOCK_ONE(_mutex);
		if (likely(p_parent != nullptr)) {
			const T *parent = dynamic_cast<const T *>(p_parent);
			ERR_FAIL_NULL(parent);

			const T *node = dynamic_cast<const T *>(p_node);
			ERR_FAIL_NULL(node);

			_reparent(parent, node);
		}
		else {
			const T *node = dynamic_cast<const T *>(p_node);
			ERR_FAIL_NULL(node);

			_unparent(node);	
		}
	}

	void get_sub_graphs(Vector<const IPipelineGraph *> &p_graphs) const override {
		LOCK_ONE(_mutex);

		auto visitor = [&p_graphs](const T *node) {
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

	const IPipelineNode* get_node(RID p_node_id) const override {
		LOCK_ONE_V(_mutex, nullptr);
		return _get_node(p_node_id);
	}

	const IPipelineNode *get_root_node() const override {
		LOCK_ONE_V(_mutex, nullptr);
		return _root;
	}

	bool set_root(RID p_node) override {
		ERR_FAIL_COND_V_EDMSG(is_bound(), false, "Cannot modify graph while it is bound.");

		LOCK_ONE_V(_mutex, false);
		auto iter = _nodes.find(p_node);
		ERR_FAIL_COND_V(iter == _nodes.end(), false);
		T *node = iter->value;

		_root = node;
		return true;
	}

	[[nodiscard]] RID get_root() const override {
		LOCK_ONE_V(_mutex, RID());
		return _root != nullptr ? _root->get_self() : RID();
	}

	void query_node(RID p_node_id, Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const override {
		LOCK_ONE(_mutex);

		const T * node = _get_node(p_node_id);
		ERR_FAIL_NULL(node);

		auto visitor = [&p_nodes, &p_predicate](const T *n) {
			if (unlikely(p_predicate(n))) {
				p_nodes.push_back(n);
			}
		};
		
		_visit(node, visitor);
	}

	void query_nodes(Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const override {
		LOCK_ONE(_mutex);
		
		auto visitor = [&p_nodes, &p_predicate](const T *node) {
			if (unlikely(p_predicate(node))) {
				p_nodes.push_back(node);
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
		
		auto visitor = [&rooted_ids](const T * node) {
			rooted_ids.insert(node->get_id());
		};

		_visit(_root, visitor);

		for(const auto &kvp : _nodes) {
			const IPipelineNode *node = kvp.value;
			p_statuses.push_back(Pair(node, rooted_ids.has(node->get_id())));
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
	template<typename T>															\
	_FORCE_INLINE_ static void register_node() {									\
		_register_node<T>();														\
	}																				\
																					\
	_FORCE_INLINE_ static LocalVector<StringName> get_registered_node_type_names() {\
		return _get_registered_node_type_names();									\
	}																				\
private:																			\

} // hydrogen



#endif //PIPELINE_GRAPH_HPP
