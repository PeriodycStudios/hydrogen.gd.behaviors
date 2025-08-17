//
// Created by tkey on 7/31/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP

#include "node_interfaces.hpp"

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
class PipelineGraph<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineGraph {

	static HashMap<StringName, std::function<T *()>> _registered_nodes;

public:
	typedef std::function<bool(const T *)> NodePredicate;

private:
	std::mutex *_mutex;
	std::atomic_uint32_t _bind_count = { 0 };

	Vector<RID> get_rooted_nodes_set() const {
		Vector<RID> nodes = {};
		if (unlikely(_root == nullptr)) {
			return nodes;
		}

		std::function<void(const IPipelineNode* node)> collect_nodes = [&nodes, &collect_nodes](const IPipelineNode* node) {
			nodes.push_back(node->get_self());

			const IPipelineNodeComposite* container = dynamic_cast<const IPipelineNodeComposite *>(node);
			if (unlikely(container != nullptr && !container->is_empty())) {
				const int child_count = container->get_node_count();
				for (int32_t i = 0; i < child_count; ++i) {
					const IPipelineNode *child_node = container->get_child_node(i);
					collect_nodes(child_node);
				}
			}
			const IPipelineNodeDecorator *wrapper = dynamic_cast<const IPipelineNodeDecorator*>(node);
			if (unlikely(wrapper && wrapper->get_pipeline_node() != nullptr)) {
				collect_nodes(wrapper->get_pipeline_node());
			}
		};

		collect_nodes(_root);

		return nodes;
	}

protected:
	HashMap<RID, T*> _nodes = {};
	HashMap<RID, RID> _node_parents = {};
	RID_PtrOwner<T> _nodes_owner = {};
	T *_root = nullptr;

	PipelineGraph() : _mutex(memnew(std::mutex)) {}

	friend class Pipeline;
	_FORCE_INLINE_ void _bind() { ++_bind_count; }
	_FORCE_INLINE_ void _unbind() {
		CRASH_COND(_bind_count == 0);
		--_bind_count;
	}

	template<typename U>
	U* _get_node(RID p_node_id) const {
		auto iter = _nodes.find(p_node_id);
		if (likely(iter != _nodes.end())) {
			return iter->value;
		}
		else {
			return nullptr;
		}
	}

	template<typename U>
	void _visit(U p_node, std::function<void(U)> p_visitor) const {
		p_visitor(p_node);

		const IPipelineNodeComposite* container = dynamic_cast<const IPipelineNodeComposite *>(p_node);
		if (unlikely(container != nullptr && !container->is_empty())) {
			const int child_count = container->get_node_count();
			for (int32_t i = 0; i < child_count; ++i) {
				U child_node = dynamic_cast<U>(container->get_child_node(i));
				ERR_CONTINUE(child_node == nullptr);
				_query_node(child_node, p_visitor);
			}
		}
		const IPipelineNodeDecorator *decorator = dynamic_cast<const IPipelineNodeDecorator*>(p_node);
		if (unlikely(decorator != nullptr)) {
			U decorated_node = dynamic_cast<U>(decorator->get_pipeline_node());
			_query_node(decorated_node, p_visitor);
		}
	}

	template<typename U>
	std::function<U> _create_query_visitor(Vector<U> &p_nodes, std::function<bool(U)> p_predicate) {
		return [&p_nodes, &p_predicate](U p_node) {
			if (likely(p_predicate(p_node))) {
				p_nodes.push_back(p_node);
			}
		};
	}

	template<typename U>
	void _query_node(U p_node, Vector<U> &p_nodes, std::function<bool(U)> p_predicate) const {
		auto visitor = _create_query_visitor(p_nodes, p_predicate);
		return _visit(p_node, visitor);
	}

	template<typename U>
	void _query_nodes(Vector<U> &p_nodes, std::function<bool(U)> p_predicate) const {
		auto visitor = _create_query_visitor(p_nodes, p_predicate);

		for (const auto &kvp : _nodes) {
			visitor(kvp.value);
		}
	}

	template<typename U>
	Vector<U> _get_nodes() const {
		Vector<U> nodes = {};
		if (unlikely(_nodes.is_empty())) {
			return nodes;
		}

		nodes.resize(_nodes.size());

		int32_t idx = 0;
		for (const auto &kvp : _nodes) {
			nodes[idx] = kvp.value;
			++idx;
		}

		return nodes;
	}

	template<typename U> 
	Vector<U> _collect_nodes() const {
		std::scoped_lock lock(*_mutex);

		const uint32_t nodes_count = _nodes.size();
		if (unlikely(nodes_count == 0)) {
			return;
		}

		Vector<U> collected_nodes = {};
		for(const auto &kvp : _nodes) {
			collected_nodes.push_back(kvp.value);
		}
	
		return collected_nodes;
	}

	void _unparent(const T *p_node) {
		auto parent_rid_iter = _node_parents.find(p_node);
		if (likely(parent_rid_iter != _node_parents.end())) {
			T *parent = get_node(parent_rid_iter->value);

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
					decorator->set_pipline_node(nullptr);
				}
			}
			else {
				ERR_PRINT("Couldn't find parent node.");
			}
			_node_parents.erase(p_node->get_self());
		}
	}

	RID _create_node(const StringName &p_node_type_name) override {
		const auto iter = _registered_nodes.find(p_node_type_name);
        ERR_FAIL_COND_V(iter == _registered_nodes.end(), RID());

		T *node = iter->value();
		RID rid = _nodes_owner.make_rid(node);
		node->set_self(rid);
		_nodes[rid] = node;
		
        return rid;
	}

	bool _destroy_node(RID p_node) override {
		
		T *node = get_node(p_node);
		ERR_FAIL_NULL_V(node, false);

		_unparent(node);

		Vector<T*> children = {};
		node->get_children(children);
		for(T* child : children) {
			RID child_rid = child->get_self();
			_node_parents.erase(child_rid);
		}

		RID rid = node->get_self();
		_nodes.erase(rid);
		_nodes_owner.free(rid);
		memdelete(node);
		return true;
	}

public:

	template<typename U,
		std::enable_if_t<
			std::is_base_of_v<T, U> &&
			!std::is_abstract_v<U>>
	>
	static void register_node() {
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

	static LocalVector<StringName> get_registered_node_type_names() {
		LocalVector<StringName> type_names = {};
		type_names.resize(_registered_nodes.size());
		int32_t idx = 0;
		for (const auto &kvp : _registered_nodes) {
			type_names[idx++] = kvp.key;
		}
		return type_names;
	}

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

	const IPipelineNode* get_pipeline_node(RID p_node_id) const override {
		return get_node(p_node_id);
	}

	IPipelineNode* get_pipeline_node(RID p_node_id) override {
		return get_node(p_node_id);
	}

	_FORCE_INLINE_ const T *get_node(RID p_node_id) const {
		return _get_node<const T *>(p_node_id);
	}

	_FORCE_INLINE_ T *get_node(RID p_node_id) {
		ERR_FAIL_COND_V_MSG(is_bound(), nullptr, "Cannot get editable node while bound!");
		return _get_node<T *>(p_node_id);
	}

	const IPipelineNode *get_pipeline_root() const override {
		return _root;
	}

	IPipelineNode *get_pipeline_root() override {
		ERR_FAIL_COND_V_MSG(is_bound(), nullptr, "Cannot get editable root node while bound!");
		return _root;
	}

	const T *get_root() const {
		return _root;
	}

	T *get_root() {
		ERR_FAIL_COND_V_MSG(is_bound(), nullptr, "Cannot get editable root node while bound!");
		return _root;
	}
	
	[[nodiscard]] bool is_bound() const override { return _bind_count > 0; }

	bool set_root_id(RID p_node_id) override {
		ERR_FAIL_COND_V_EDMSG(is_bound(), false, "Cannot modify graph while it is bound.");

		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V(iter == _nodes.end(), false);
		T *node = iter->value;

		_root = node;
		return true;
	}

	[[nodiscard]] RID get_root_id() const override {
		std::scoped_lock lock(*_mutex);
		return _root != nullptr ? _root->get_self() : RID();
	}

	Vector<const IPipelineNode *> get_pipeline_nodes() const override {
		return _get_nodes<const IPipelineNode *>();
	}

	Vector<const T *> get_nodes() const {
		return _get_nodes<const T *>();
	}

	Vector<T *> get_nodes() {
		ERR_FAIL_COND_V_MSG(is_bound(), {}, "Cannot get editable nodes while bound !");
		return _get_nodes<T *>();
	}

	void query_pipeline_node(RID p_node_id, Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const override {
		std::scoped_lock lock(*_mutex);
		const IPipelineNode* node = get_node(p_node_id);
		ERR_FAIL_NULL(node);

		_query_node<const IPipelineNode *>(node, p_nodes, p_predicate);
	}

	void query_pipeline_node(RID p_node_id, Vector<IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) override {
		ERR_FAIL_COND_MSG(is_bound(), "Cannot query editable nodes when bound!");

		std::scoped_lock lock(*_mutex);
		IPipelineNode *node = get_node(p_node_id);
		ERR_FAIL_NULL(node);

		_query_node<IPipelineNode *>(node, p_nodes, p_predicate);
	}

	void query_node(RID p_node_id, Vector<const T *> &p_nodes, NodePredicate p_predicate) const {
		std::scoped_lock lock(*_mutex);
		const T *node = get_node(p_node_id);
		ERR_FAIL_NULL(node);

		_query_node(node, p_nodes, p_predicate);
	}

	void query_node(RID p_node_id, Vector<T *> &p_nodes, NodePredicate p_predicate) {
		ERR_FAIL_COND_MSG(is_bound(), "Cannot query editable nodes when bound!");

		std::scoped_lock lock(*_mutex);
		T *node = get_node(p_node_id);
		ERR_FAIL_NULL(node);

		_query_node(node, p_nodes, p_predicate);
	}

	void query_pipeline_nodes(Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const override {
		std::scoped_lock lock(*_mutex);

		_query_nodes<const IPipelineNode *>(p_nodes, p_predicate);
	}

	void query_pipeline_nodes(Vector<IPipelineNode*> &p_nodes, PipelineNodePredicate p_predicate) override {
		ERR_FAIL_COND_MSG(is_bound(), "Cannot query editable nodes when bound!");
		std::scoped_lock lock(*_mutex);
		_query_nodes<IPipelineNode *>(p_nodes, p_predicate);
	}

	void query_nodes(Vector<const T *> &p_nodes, NodePredicate p_predicate) const {
		std::scoped_lock lock(*_mutex);
		_query_nodes<const T *>(p_nodes, p_predicate);
	}

	void query_nodes(Vector<T *> &p_nodes, NodePredicate p_predicate) {
		ERR_FAIL_COND_MSG(is_bound(), "Cannot query editable nodes when bound!");
		std::scoped_lock lock(*_mutex);
		_query_nodes<T *>(p_nodes, p_predicate);
	}

	[[nodiscard]] Vector<RID> get_unrooted_nodes() const override {
		std::scoped_lock lock(*_mutex);

		HashSet<RID> rooted_nodes = get_rooted_nodes_set();
		Vector<RID> unrooted_nodes = {};

		if (unlikely(rooted_nodes.is_empty())) {

			for (const auto &kvp : _nodes) {
				unrooted_nodes.push_back(kvp.key);
			}

			return unrooted_nodes;
		}
		else {
			for (const auto &kvp : _nodes) {
				if (likely(rooted_nodes.has(kvp.key))) {
					unrooted_nodes.push_back(kvp.key);
				}
			}
			return unrooted_nodes;
		}
	}
	[[nodiscard]] Vector<RID> get_rooted_nodes() const override {
		std::scoped_lock lock(*_mutex);

		HashSet<RID> rooted_nodes = get_rooted_nodes_set();
		Vector<RID> collected_nodes = {};

		if (unlikely(rooted_nodes.is_empty())) {
			return collected_nodes;
		}

		for (RID rid : rooted_nodes) {
			collected_nodes.push_back(rid);
		}

		return collected_nodes;
	}
};

} // hydrogen

#endif //PIPELINE_GRAPH_HPP
