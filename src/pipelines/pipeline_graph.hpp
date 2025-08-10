//
// Created by tkey on 7/31/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP

#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "node_interfaces.hpp"

#include <functional>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <cstdint>
#include <type_traits>
#include <atomic>
#include <mutex>

namespace hydrogen::pipelines {

class Pipeline;

class IPipelineGraph {
protected:
	friend class Pipeline;

	[[nodiscard]] virtual const IPipelineNode *get_node(RID p_node_id) = 0;
	[[nodiscard]] virtual const IPipelineNode *get_pipeline_root() const = 0;
	[[nodiscard]] virtual Vector<const IPipelineGraph *> get_pipeline_sub_graphs() = 0;
	[[nodiscard]] virtual Vector<const IPipelineNode *> get_pipeline_nodes() = 0;

	IPipelineGraph() = default;

public:
	virtual ~IPipelineGraph() = default;

	[[nodiscard]] virtual bool is_bound() const = 0;

	[[nodiscard]] virtual RID create_node(const StringName &p_node_type_name) = 0;
	virtual bool destroy_node(RID p_node_id) = 0;
	virtual bool set_root_id(RID p_node_id) = 0;
	[[nodiscard]] virtual RID get_root_id() = 0;

	[[nodiscard]] virtual int32_t get_input_port_count(RID p_node_id) const = 0;
	[[nodiscard]] virtual int32_t get_output_port_count(RID p_node_id) const = 0;

	[[nodiscard]] virtual StringName get_input_port_type_name(RID p_node_id, int32_t p_port) const = 0;
	[[nodiscard]] virtual StringName get_output_port_type_name(RID p_node_id, int32_t p_port) const = 0;

	[[nodiscard]] virtual StringName get_input_port_name(RID p_node_id, int32_t p_port) const = 0;
	[[nodiscard]] virtual StringName get_output_port_name(RID p_node_id, int32_t p_port) const = 0;

	virtual void get_input_port_infos(RID p_node_id, Vector<NodePortInfo> &p_infos) const = 0;
	virtual void get_output_port_infos(RID p_node_id, Vector<NodePortInfo> &p_infos) const = 0;

	[[nodiscard]] virtual Vector<RID> get_unrooted_nodes() const = 0;
	[[nodiscard]] virtual Vector<RID> get_rooted_nodes() const = 0;
};

template<typename T, typename = void>
class PipelineGraph {};

template <typename T>
class PipelineGraph<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public RidData, public IPipelineGraph {
	std::mutex *_mutex;
	std::atomic_uint32_t _bind_count = { 0 };

	HashSet<RID> get_rooted_nodes_set() const {
		HashSet<RID> nodes = {};
		if (unlikely(_root == nullptr)) {
			return nodes;
		}

		std::function<void(const IPipelineNode* node)> collect_nodes = [&nodes, &collect_nodes](const IPipelineNode* node) {
			nodes.insert(node->get_self());

			const IPipelineNodeContainer* container = dynamic_cast<const IPipelineNodeContainer *>(node);
			if (unlikely(container != nullptr && !container->is_empty())) {
				const int child_count = container->get_node_count();
				for (int32_t i = 0; i < child_count; ++i) {
					const IPipelineNode *child_node = container->get_node(i);
					collect_nodes(child_node);
				}
			}
			const IPipelineNodeWrapper *wrapper = dynamic_cast<const IPipelineNodeWrapper*>(node);
			if (unlikely(wrapper && wrapper->get_pipeline_node() != nullptr)) {
				collect_nodes(wrapper->get_pipeline_node());
			}
		};

		collect_nodes(_root);

		return nodes;
	}

protected:
	HashMap<RID, T*> _nodes = {};
	RID_PtrOwner<T> _nodes_owner = {};
	T *_root = nullptr;

	PipelineGraph() : _mutex(memnew(std::mutex)) {}

	friend class Pipeline;
	_FORCE_INLINE_ void _bind() { ++_bind_count; }
	_FORCE_INLINE_ void _unbind() {
		CRASH_COND(_bind_count == 0);
		--_bind_count;
	}

	const IPipelineNode* get_node(RID p_node_id) override {
		return _nodes_owner.get_or_null(p_node_id);
	}

	const IPipelineNode *get_pipeline_root() const override {
		return _root;
	}

	Vector<const IPipelineGraph *> get_pipeline_sub_graphs() override {
		std::scoped_lock lock(*_mutex);

		const uint32_t nodes_count = _nodes.size();
		if (unlikely(nodes_count == 0)) {
			return {};
		}

		Vector<const IPipelineGraph *> sub_graphs = {};
		for (const auto &kvp : _nodes) {
			const IPipelineNode *node = kvp.value;
			const IPipelineNodeSubGraph *sub_graph_node = dynamic_cast<const IPipelineNodeSubGraph *>(node);
			if (unlikely(sub_graph_node != nullptr && sub_graph_node->get_sub_graph() != nullptr)) {
				sub_graphs.push_back(sub_graph_node->get_sub_graph());
			}
		}
		
		return sub_graphs;
	}

	Vector<const IPipelineNode *> get_pipeline_nodes() override {
		std::scoped_lock lock(*_mutex);

		const uint32_t nodes_count = _nodes_owner.get_rid_count();
		if (unlikely(nodes_count == 0)) {
			return {};
		}

		Vector<const IPipelineNode *> collected_nodes = {};
		for(const auto &kvp : _nodes) {
			collected_nodes.push_back(kvp.value);
		}
	
		return collected_nodes;
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

	[[nodiscard]] RID get_root_id() override {
		std::scoped_lock lock(*_mutex);
		return _root != nullptr ? _root->get_self() : RID();
	}

	[[nodiscard]] int32_t get_input_port_count(RID p_node_id) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V_MSG(iter == _nodes.end(), 0, "Unknown node id.");
		T* node = iter->value;
		return node->get_input_port_count();
	}

	[[nodiscard]] int32_t get_output_port_count(RID p_node_id) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V_MSG(iter == _nodes.end(), 0, "Unknown node id.");
		T* node = iter->value;
		return node->get_output_port_count();
	}

	[[nodiscard]] StringName get_input_port_type_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V_MSG(iter == _nodes.end(), StringName(), "Unknown node id.");
		T* node = iter->value;
		return node->get_input_port_type_name(p_port);
	}

	[[nodiscard]] StringName get_output_port_type_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V_MSG(iter == _nodes.end(), StringName(), "Unknown node id.");
		T* node = iter->value;
		return node->get_output_port_type_name(p_port);
	}

	[[nodiscard]] StringName get_input_port_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V_MSG(iter == _nodes.end(), StringName(), "Unknown node id.");
		T *node = iter->value;
		return node->get_input_port_name(p_port);
	}

	[[nodiscard]] StringName get_output_port_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_V_MSG(iter == _nodes.end(), StringName(), "Unknown node id.");
		T *node = iter->value;
		return node->get_output_port_name(p_port);
	}

	void get_input_port_infos(RID p_node_id, Vector<NodePortInfo> &p_infos) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_MSG(iter == _nodes.end(), "Unknown node id.");
		T *node = iter->value;
		node->get_input_port_infos(p_infos);
	}

	void get_output_port_infos(RID p_node_id, Vector<NodePortInfo> &p_infos) const override {
		std::scoped_lock lock(*_mutex);
		auto iter = _nodes.find(p_node_id);
		ERR_FAIL_COND_MSG(iter == _nodes.end(), "Unknown node id.");
		T *node = iter->value;
		node->get_output_port_infos(p_infos);
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
