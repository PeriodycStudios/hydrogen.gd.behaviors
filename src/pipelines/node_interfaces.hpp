//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "../rid_data.hpp"
#include "../name_helpers.hpp"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <cstdint>

namespace hydrogen::pipelines {

using namespace godot;

DEFINE_NAME_STATIC(error)

class IPipelineNode;

struct NodePortInfo {
	const StringName name;
	const StringName type_name;

	template<typename T>
	static NodePortInfo create_port(const StringName &p_name) {
		return NodePortInfo(p_name, get_type_name_static<T>());
	}

private:
	friend class IPipelineNode;

	NodePortInfo(const StringName &p_name, const StringName &p_type_name) : name(p_name), type_name(p_type_name) {}
};

class IPipelineNode : public RidData {
protected:
	IPipelineNode() = default;

public:
	virtual ~IPipelineNode() = default;

	[[nodiscard]] virtual int32_t get_input_port_count() const = 0;
	[[nodiscard]] virtual int32_t get_output_port_count() const = 0;

	[[nodiscard]] virtual StringName get_input_port_type_name(int32_t p_port) const = 0;
	[[nodiscard]] virtual StringName get_output_port_type_name(int32_t p_port) const = 0;

	[[nodiscard]] virtual StringName get_input_port_name(int32_t p_port) const = 0;
	[[nodiscard]] virtual StringName get_output_port_name(int32_t p_port) const = 0;

	virtual void get_input_port_infos(Vector<NodePortInfo> &p_infos) const = 0;
	virtual void get_output_port_infos(Vector<NodePortInfo> &p_infos) const = 0;
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

class IPipelineNodeContainer {
protected:
	IPipelineNodeContainer() = default;
public:
	virtual ~IPipelineNodeContainer() = default;
	[[nodiscard]] virtual bool is_empty() const = 0;
	[[nodiscard]] virtual int64_t get_node_count() const = 0;
	[[nodiscard]] virtual IPipelineNode *get_node(int64_t p_index) const = 0;
};

class IPipelineNodeWrapper {
protected:
	IPipelineNodeWrapper() = default;
public:
	virtual ~IPipelineNodeWrapper() = default;
	[[nodiscard]] virtual IPipelineNode *get_pipeline_node() const = 0;
};

class IPipelineGraph;

class IPipelineNodeSubGraph {
protected:
	IPipelineNodeSubGraph() = default;
public:
	virtual ~IPipelineNodeSubGraph() = default;
	[[nodiscard]] virtual const IPipelineGraph *get_sub_graph() const;
};
} // hydrogen

#endif //BEHAVIORS_NODE_BASE_HPP
