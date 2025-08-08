//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "../rid_data.hpp"

#include <cstdint>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#define MAKE_BLACKBOARD_ENTRY_NAME(entry_name)			\
	static const StringName &entry_name##_name() {		\
		const static StringName name(#entry_name, true);\
		return name;									\
	}

namespace hydrogen::pipelines {

using namespace godot;

MAKE_BLACKBOARD_ENTRY_NAME(error)

class IPipelineNode;

struct NodePortInfo {
	const StringName name;
	const StringName type_name;

private:
	friend class IPipelineNode;

	NodePortInfo(const StringName &p_name, const StringName &p_type_name) : name(p_name), type_name(p_type_name) {}
};

class IPipelineNode : public RidData {
protected:
	IPipelineNode() = default;

public:
	virtual ~IPipelineNode() = default;

	virtual int32_t get_input_port_count() const = 0;
	virtual int32_t get_output_port_count() const = 0;

	virtual StringName get_input_port_type_name(int32_t p_port) const = 0;
	virtual StringName get_output_port_type_name(int32_t p_port) const = 0;

	virtual StringName get_input_port_name(int32_t p_port) const = 0;
	virtual StringName get_output_port_name(int32_t p_port) const = 0;

	virtual void get_input_port_info(Vector<NodePortInfo> &p_infos) const = 0;
	virtual void get_output_port_info(Vector<NodePortInfo> &p_infos) const = 0;
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

class IPipelineNodeContainer : public IPipelineNode {
protected:
	IPipelineNodeContainer() = default;
public:
	~IPipelineNodeContainer() override = default;
	virtual int64_t get_node_count() const = 0;
	virtual IPipelineNode *get_node(int64_t p_index) const = 0;
};

class IPipelineNodeWrapper : public IPipelineNode {
protected:
	IPipelineNodeWrapper() = default;
public:
	~IPipelineNodeWrapper() override = default;
	virtual IPipelineNode *get_pipeline_node() const = 0;
};
} // hydrogen

#endif //BEHAVIORS_NODE_BASE_HPP
