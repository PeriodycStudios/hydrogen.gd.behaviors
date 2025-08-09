//
// Created by tkey on 7/31/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP
#include "pipeline_nodes.hpp"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/rid_owner.hpp>

#include <type_traits>
#include <atomic>
#include <mutex>

namespace hydrogen::pipelines {

class Pipeline;

class IPipelineGraph {

	friend class Pipeline;

	[[nodiscard]] virtual const IPipelineNode *get_node(RID p_node_id) const = 0;
	[[nodiscard]] virtual const IPipelineNode *get_pipeline_root() const = 0;
	[[nodiscard]] virtual Vector<const IPipelineGraph *> get_pipeline_sub_graphs() const = 0;
	[[nodiscard]] virtual Vector<const IPipelineNode *> get_pipeline_nodes() const = 0;

protected:
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

	[[nodiscard]] virtual Vector<RID> get_nodes_unconnected_to_root() const = 0;
	[[nodiscard]] virtual Vector<RID> get_nodes_connected_to_root() const = 0;
};

template<typename T, typename = void>
class PipelineGraph {};

template <typename T>
class PipelineGraph<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public RidData, public IPipelineGraph {
	std::mutex _mutex = {};
	std::atomic_uint32_t _bind_count = { 0 };

protected:
	RID_PtrOwner<T> _nodes = {};
	T *_root = nullptr;

	PipelineGraph() = default;

	friend class Pipeline;
	_FORCE_INLINE_ void _bind() { ++_bind_count; }
	_FORCE_INLINE_ void _unbind() {
		CRASH_COND(_bind_count == 0);
		--_bind_count;
	}

public:
	~PipelineGraph() override {
		_root = nullptr;
		for (auto node : _nodes) {
			memdelete(node);
		}
		_nodes.clear();
	}

	[[nodiscard]] bool is_bound() const override { return _bind_count > 0; }

	bool set_root_id(RID p_node_id) override {
		ERR_FAIL_COND_V_EDMSG(is_bound(), false, "Cannot modify graph while it is bound.");

		std::scoped_lock lock(_mutex);
		ERR_FAIL_COND_V(is_bound(), false);

		T *node = _nodes.get_or_null(p_node_id);
		if (unlikely(node == nullptr)) {
			ERR_FAIL_V(false);
		}

		_root = node;
		return true;
	}

	[[nodiscard]] RID get_root_id() override {
		std::scoped_lock lock(_mutex);
		return _root != nullptr ? _root->get_self() : RID();
	}

	[[nodiscard]] int32_t get_input_port_count(RID p_node_id) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		if (unlikely(node == nullptr)) {
			ERR_FAIL_V_MSG(0, "Unknown node id.");
		}
		return node->get_input_port_count();
	}

	[[nodiscard]] int32_t get_output_port_count(RID p_node_id) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		if (unlikely(node == nullptr)) {
			ERR_FAIL_V_MSG(0, "Unknown node id.");
		}
		return node->get_output_port_count(p_node_id);
	}

	[[nodiscard]] StringName get_input_port_type_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		if (unlikely(node == nullptr)) {
			ERR_FAIL_V_MSG(StringName(), "Unknown node id.");
		}

		return node->get_input_port_type_name(p_port);
	}

	[[nodiscard]] StringName get_output_port_type_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		if (unlikely(node == nullptr)) {
			ERR_FAIL_V_MSG(StringName(), "Unknown node id.");
		}

		return node->get_output_port_type_name(p_port);
	}

	[[nodiscard]] StringName get_input_port_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		ERR_FAIL_COND_V_MSG(node == nullptr, StringName(), "Unknown node id.");
		return node->get_input_port_name(p_port);
	}

	[[nodiscard]] StringName get_output_port_name(RID p_node_id, int32_t p_port) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		ERR_FAIL_COND_V_MSG(node == nullptr, StringName(), "Unknown node id.");
		return node->get_output_port_name(p_port);
	}

	void get_input_port_infos(RID p_node_id, Vector<NodePortInfo> &p_infos) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		ERR_FAIL_COND_MSG(node == nullptr, "Unknown node id.");
		node->get_input_port_infos(p_infos);
	}

	void get_output_port_infos(RID p_node_id, Vector<NodePortInfo> &p_infos) const override {
		std::scoped_lock lock(_mutex);
		T *node = _nodes.get_or_null(p_node_id);
		ERR_FAIL_COND_MSG(node == nullptr, "Unknown node id.");
		node->get_output_port_infos(p_infos);
	}
};

} // hydrogen

#endif //PIPELINE_GRAPH_HPP
