//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "../name_helpers.hpp"
#include "blackboard.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/type_info.hpp"
#include "godot_cpp/templates/pair.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "variant_type_traits.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <cstdint>
#include <functional>
#include <optional>

namespace hydrogen::pipelines {

using namespace godot;

DEFINE_NAME_STATIC(error)

class IPipelineNode;

typedef std::function<void(Blackboard *)> InputNodeDefaultSetter;

struct NodePortInfo {

	enum PORT_KIND {
		INPUT = 1 << 0,
		OUTPUT = 1 << 1,
		IN_OUT = INPUT | OUTPUT,                                                                                                                                                                                                                                                      
	};

	const StringName name;
	const StringName type_name;
	const Variant::Type variant_type;
	const std::optional<InputNodeDefaultSetter> default_setter;
	const PORT_KIND port_kind;

	_FORCE_INLINE_ bool is_input() const { return (port_kind & INPUT) != 0; }
	_FORCE_INLINE_ bool is_output() const { return (port_kind & OUTPUT) != 0; }

	constexpr static bool is_true() { return true; }
	constexpr static bool is_false() { return false; }

	Dictionary to_dictionary() const {
		Dictionary dict = {};

		dict["name"] = name;
		dict["type_name"] = type_name;
		dict["variant_type"] = variant_type;
		
		if (default_setter.has_value() && variant_type != Variant::NIL) {
			Blackboard bb = Blackboard();
			default_setter.value()(&bb);
			Variant default_value = bb.get_entry<Variant>(name);
			dict["default_value"] = default_value;
		}

		dict["port_kind"] = port_kind;
		dict["is_input"] = is_input();
		dict["is_output"] = is_output();

		return dict;
	}

	template<typename T> 
	static NodePortInfo create_input(const StringName &p_name, const InputNodeDefaultSetter p_setter) {
		return create<T>(p_name, INPUT, p_setter);
	}

	template<typename T>
	static NodePortInfo create_output(const StringName &p_name) {
		return create<T>(p_name, OUTPUT);
	}

	template<typename T>
	static NodePortInfo create_in_out(const StringName &p_name, const InputNodeDefaultSetter p_setter) {
		return create<T>(p_name, IN_OUT, p_setter);
	}

private:

	template<typename T>
	static NodePortInfo create(const StringName &p_name, PORT_KIND p_kind, std::optional<InputNodeDefaultSetter> p_setter = nullptr) {

		Variant::Type variant_type;
		if constexpr(traits::is_variant_type_v<T>) {
			variant_type = static_cast<Variant::Type>(GetTypeInfo<T>::VARIANT_TYPE);
		}
		else {
			variant_type = Variant::NIL;
		}

		const StringName &type_name = get_type_name_static<T>();

		return NodePortInfo(p_name, type_name, variant_type, p_setter, p_kind);
	}

	NodePortInfo(const StringName &p_name, const StringName &p_type_name, const Variant::Type p_variant_type, const std::optional<InputNodeDefaultSetter> p_setter, PORT_KIND p_kind) 
	: name(p_name), type_name(p_type_name), variant_type(p_variant_type), default_setter(p_setter), port_kind(p_kind) {}
};

#define DECLARE_PORT_NAME(port_name) DEFINE_NAME_STATIC(port_name)

#define PORT(port) port##_name()

#define BEGIN_NODE_PORTS()								\
	static const Vector<NodePortInfo> &get_ports() {	\
		static const Vector<NodePortInfo> ports = {		\

#define INPUT_PORT(port_name, port_type, default_value) 											\
			NodePortInfo::create_input<port_type>(PORT(port_name), [](Blackboard *p_blackboard) {	\
				static const port_type def_val = default_value;										\
				p_blackboard->set_entry_fast<port_type>(PORT(port_name), def_val);					\
			}),																						\

#define INPUT_OUTPUT_PORT(port_name, port_type, default_value)										\
			NodePortInfo::create_in_out<port_type>(PORT(port_name), [](Blackboard *p_blackboard) {	\
				static const port_type def_val = default_value;										\
				p_blackboard->set_entry_fast<port_type>(def_val);									\
			}),																						\

#define OUTPUT_PORT(port_name, port_type)							\
			NodePortInfo::create_output<port_type>(PORT(port_name)),\

#define END_NODE_PORTS()\
		};				\
		return ports;	\
	}					\

class IPipelineGraph;

class IPipelineNode {
protected:
	IPipelineNode() = default;

public:
	virtual ~IPipelineNode() = default;

	virtual RID get_id() const = 0;

	[[nodiscard]] virtual StringName get_type_name() const = 0;
	[[nodiscard]] virtual bool is_compatible(const IPipelineNode *p_other_node) const = 0;

	virtual const Vector<NodePortInfo> &get_port_infos() const = 0;

	[[nodiscard]] virtual bool supports_children() const = 0;
	[[nodiscard]] virtual bool has_children() const = 0;
	virtual void get_children(Vector<const IPipelineNode *> &p_nodes) const = 0;
	virtual void get_descendants(Vector<const IPipelineNode *> &p_nodes) const = 0;
};

struct IPipelineNodeState;

struct IPipelineNodeStateful;


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

	virtual bool add_child_node(const IPipelineNode *p_node) = 0;
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
};

class IPipelineNodeDecorator {
protected:
	IPipelineNodeDecorator() = default;
public:
	virtual ~IPipelineNodeDecorator() = default;

	[[nodiscard]] virtual const IPipelineNode *get_node() const = 0;
	virtual void set_node(const IPipelineNode *p_node) = 0;
};

class IPipelineGraph {
protected:

	virtual RID _create_node(const StringName &p_node_type_name) = 0;
	virtual bool _destroy_node(RID p_node_id) = 0;

	friend class Pipeline;
	IPipelineGraph() = default;

public:
	virtual ~IPipelineGraph() = default;

	virtual RID create_node(const StringName &p_node_type_name) = 0;
	virtual bool destroy_node(RID p_node) = 0;

	[[nodiscard]] virtual bool is_bound() const = 0;

	virtual RID get_id() const = 0;

	virtual void update_node(const IPipelineNode *p_node, const IPipelineNode *p_parent = nullptr) = 0;

	virtual void get_sub_graphs(Vector<const IPipelineGraph *> &p_graphs) const = 0;

	virtual void get_nodes(Vector<const IPipelineNode*> &p_nodes) const = 0;

	[[nodiscard]] virtual const IPipelineNode *get_node(RID p_node_id) const = 0;
	[[nodiscard]] virtual IPipelineNode *get_node(RID p_node) = 0;
	[[nodiscard]] virtual const IPipelineNode *get_root_node() const = 0;
	[[nodiscard]] virtual IPipelineNode *get_root_node() = 0;

	virtual bool set_root(RID p_node) = 0;
	[[nodiscard]] virtual RID get_root() const = 0;

	typedef std::function<bool(const IPipelineNode *)> PipelineNodePredicate;

	virtual void query_node(RID p_node_id, Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const = 0;
	virtual void query_nodes(Vector<const IPipelineNode *> &p_nodes, PipelineNodePredicate p_predicate) const = 0;

	virtual void get_rooted_statuses(Vector<Pair<const IPipelineNode *, bool>> &p_statuses) const = 0;
};

class IPipelineNodeSubGraph {
protected:
	IPipelineNodeSubGraph() = default;
public:
	virtual ~IPipelineNodeSubGraph() = default;

	[[nodiscard]] virtual const IPipelineGraph *get_sub_graph() const = 0;
	virtual void set_sub_graph(IPipelineGraph *p_graph) = 0;
};
} // hydrogen

VARIANT_BITFIELD_CAST(hydrogen::pipelines::NodePortInfo::PORT_KIND);

#endif //BEHAVIORS_NODE_BASE_HPP
