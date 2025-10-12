//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "../name_helpers.hpp"
#include "blackboard.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/type_info.hpp"
#include "godot_cpp/templates/pair.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "variant_type_traits.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>

namespace hydrogen::pipelines {

using namespace godot;

DEFINE_NAME_STATIC(_error);

class IPipelineNode;

typedef std::function<void(const IPipelineNode *p_node, Blackboard *)> NodePortDefaultSetter;
typedef std::function<Variant()> NodePortDefaultGetter;

struct NodePortInfo {

	enum PORT_DIRECTION {
		NONE = 0,
		INPUT = 1 << 0,
		OUTPUT = 1 << 1,
		IN_OUT = INPUT | OUTPUT,                                                                                                                                                                                                                                                      
	};

	StringName name = "";
	StringName type_name = "";
	Variant::Type variant_type = Variant::NIL;
	std::optional<NodePortDefaultSetter> default_setter = nullptr;
	std::optional<NodePortDefaultGetter> default_getter = nullptr;
	PORT_DIRECTION direction = NONE;
	bool is_graph_port = false;

	_FORCE_INLINE_ bool is_input() const { return (direction & INPUT) != 0; }
	_FORCE_INLINE_ bool is_output() const { return (direction & OUTPUT) != 0; }

	constexpr static bool is_true() { return true; }
	constexpr static bool is_false() { return false; }

	Dictionary to_dictionary(const IPipelineNode *p_node, Blackboard *_blackboard) const;

	void from_dictionary(Dictionary p_dict);

	template<typename T> 
	_FORCE_INLINE_ static NodePortInfo create_input(const StringName &p_name, const NodePortDefaultSetter p_setter) {
		return create<T>(p_name, INPUT, p_setter);
	}

	template<typename T>
	_FORCE_INLINE_ static NodePortInfo create_output(const StringName &p_name) {
		return create<T>(p_name, OUTPUT);
	}

	template<typename T>
	_FORCE_INLINE_ static NodePortInfo create_in_out(const StringName &p_name, const NodePortDefaultSetter p_setter) {
		return create<T>(p_name, IN_OUT, p_setter);
	}

	template<typename T>
	_FORCE_INLINE_ static NodePortInfo create_graph_port(const StringName &p_name, const NodePortDefaultSetter p_setter) {
		return create<T>(p_name, INPUT, p_setter, true);
	}

	NodePortInfo() {}

private:

	template<typename T>
	static NodePortInfo create(const StringName &p_name, PORT_DIRECTION p_kind, std::optional<NodePortDefaultSetter> p_setter = nullptr, bool p_is_graph_port = false) {

		Variant::Type variant_type;
		if constexpr(traits::is_variant_type_v<T>) {
			variant_type = static_cast<Variant::Type>(GetTypeInfo<T>::VARIANT_TYPE);
		}
		else {
			variant_type = Variant::NIL;
		}

		const StringName &type_name = get_type_name_static<T>();

		return NodePortInfo(p_name, type_name, variant_type, p_setter, p_kind, p_is_graph_port);
	}

	NodePortInfo(const StringName &p_name, const StringName &p_type_name, const Variant::Type p_variant_type, const std::optional<NodePortDefaultSetter> p_setter, PORT_DIRECTION p_kind, bool p_is_graph_port) 
	: name(p_name), type_name(p_type_name), variant_type(p_variant_type), default_setter(p_setter), direction(p_kind), is_graph_port(p_is_graph_port) {}
};

struct NodeConnectionInfo {
	StringName name = "";
	StringName type_name = "";

	template<typename T>
	static NodeConnectionInfo create(const StringName &p_name) {
		return NodeConnectionInfo(p_name, get_type_name_static<T>());
	}

	Dictionary to_dictionary() const {
		Dictionary dict = {};
		dict["name"] = name;
		dict["type_name"] = type_name;
		return dict;
	}

	NodeConnectionInfo() {}

private:
	NodeConnectionInfo(const StringName &p_name, const StringName &p_type_name) 
		: name(p_name), type_name(p_type_name) 
		{}
};

namespace _detail {

	template<typename T>
	static Vector<T> _concat(const Vector<T> &p_src, std::initializer_list<T> p_init) {
		Vector<T> result = p_src;
		result.append_array(p_init);
		return result;
	}
}

#define DECLARE_INPUT_PORT(port_name, port_type, default_value)	\
	DEFINE_NAME_STATIC(port_name);								\
	typedef port_type port_name##_type;							\
	static port_type get_default_##port_name() {				\
		static const port_type value = default_value;			\
		return value; 											\
	}															\
																\
	static Variant get_default_variant_##port_name() {			\
		if constexpr (traits::is_variant_type_v<port_type>) {	\
			return get_default_##port_name();					\
		}														\
		else {													\
			return Variant();									\
		}														\
	}															\

#define DECLARE_OUTPUT_PORT(port_name, port_type)	\
	DEFINE_NAME_STATIC(port_name);					\
	typedef port_type port_name##_type				\

#define PORT(port) port##_name()

#define BEGIN_NODE_PORTS()																				\
	static const Vector<NodePortInfo> &_get_ports() {													\
		static const Vector<NodePortInfo> &parent_ports = parent::_get_ports();							\
		static const Vector<NodePortInfo> ports = hydrogen::pipelines::_detail::_concat(parent_ports, {	\

#define _IN_PORT(port_name, func)																								\
			NodePortInfo::func<port_name##_type>(PORT(port_name), [](const IPipelineNode *p_node, Blackboard *p_blackboard) {	\
				const StringName &name = PORT(port_name);																		\
				const StringName &alias = p_node->get_input_alias(name);														\
				p_blackboard->set_entry_fast<port_name##_type>(alias, get_default_##port_name());								\
			}),																													\

#define INPUT_PORT(port_name) _IN_PORT(port_name, create_input)
#define INPUT_OUTPUT_PORT(port_name) _IN_PORT(port_name, create_in_out)

#define OUTPUT_PORT(port_name)											\
		NodePortInfo::create_output<port_name##_type>(PORT(port_name)),	\

#define END_NODE_PORTS()	\
		});					\
		return ports;		\
	}						\

#define DEFINE_STATEFUL_FUNCS(state_type) 												\
	IPipelineNodeState * create_state() const override { return memnew(state_type); }	\
	RID state_key() const override { return get_self(); }								\

#define DECLARE_CONNECTION(connection_name, node_type)	\
	DEFINE_NAME_STATIC(connection_name);				\
	const node_type *_##connection_name = nullptr;		\
	typedef const node_type *connection_name##_type		\


#define DEFINE_CONNECTION(connection_name)												\
	_FORCE_INLINE_ void set_##connection_name(connection_name##_type p_connection) {	\
		_##connection_name = p_connection;												\
	}																					\
																						\
	_FORCE_INLINE_ connection_name##_type get_##connection_name() const {				\
		return _##connection_name;														\
	}																					\

	#define BEGIN_CONNECTIONS()																								\
	static const Vector<NodeConnectionInfo> &_get_connections() {															\
		static const Vector<NodeConnectionInfo> &parent_connections = parent::_get_connections();							\
		static const Vector<NodeConnectionInfo> connections = hydrogen::pipelines::_detail::_concat(parent_connections, {	\

#define CONNECTION(connection_name)													\
			NodeConnectionInfo::create<connection_name##_type>(#connection_name),	\

#define END_CONNECTIONS()	\
		});					\
		return connections;	\
	}						\

#define BEGIN_SET_CONNECTION()																\
	bool set_connection(const StringName &p_name, const IPipelineNode *p_node) override {	\
		if (unlikely(parent::set_connection(p_name, p_node))) {								\
			return true;																	\
		}																					\
																							\

#define SET_CONNECTION(connection_name)											\
		if (p_name == connection_name##_name()) {								\
			set_##connection_name(dynamic_cast<connection_name##_type>(p_node));\
			return true;														\
		}																		\

#define END_SET_CONNECTION()									\
		WARN_PRINT(vformat("Unknown connection: {}", p_name));	\
		return false;											\
	}															\

#define BEGIN_GET_CONNECTION()														\
	const IPipelineNode *get_connection(const StringName &p_name) const override {	\
		const IPipelineNode *node = parent::get_connection(p_name);					\
		if (unlikely(node != nullptr)) {											\
			return node;															\
		}																			\

#define GET_CONNECTION(connection_name)				\
		if (p_name == connection_name##_name())	{	\
			return get_##connection_name();			\
		}											\

#define END_GET_CONNECTION()									\
		WARN_PRINT(vformat("Unknown connection: {}", p_name));	\
		return nullptr;											\
	}															\

#define EMPTY_PORT_LIST() static const Vector<NodePortInfo> &_get_ports() { return parent::_get_ports(); }

#define EMPTY_CONNECTION_LIST() static const Vector<NodeConnectionInfo> &_get_connections() { return parent::_get_connections(); }

#define ABSTRACT_PIPELINE_NODE(type_name, parent_type)	\
														\
public:													\
	typedef type_name self;								\
	typedef parent_type parent;							\
														\
private:												\

#define DECLARE_PIPELINE_NODE(type_name, parent_type)	\
	DEFINE_NAME_STATIC(type_name);              		\
                                                		\
public:										    		\
	type_name() = default;								\
	~type_name() override = default;					\
														\
	typedef type_name self;								\
	typedef parent_type parent;							\
														\
	static const StringName &get_node_name() {			\
		return type_name##_name();						\
	}													\
                                                		\
	const StringName & get_type_name() const override {	\
		return type_name##_name();			    		\
	}										    		\
                                                		\
private:                                        		\

#define DEFINE_GET_PORTS()                                         	\
    const Vector<NodePortInfo> &get_ports() const override {   		\
        return _get_ports();                                        \
    }                                                               \

#define DEFINE_GET_CONNECTIONS()																		\
	const Vector<NodeConnectionInfo> &get_connections() const override { return _get_connections(); }	\

class IPipelineGraph;

typedef TypedDictionary<StringName, StringName> PortAliases;

struct IPipelineNode {
	virtual ~IPipelineNode() = default;

	virtual RID get_id() const = 0;

	[[nodiscard]] virtual const StringName &get_type_name() const = 0;

	virtual const Vector<NodePortInfo> &get_ports() const = 0;
	virtual const Vector<NodeConnectionInfo> &get_connections() const = 0;

	virtual bool set_connection(const StringName &p_name, const IPipelineNode *p_target) = 0;
	virtual const IPipelineNode *get_connection(const StringName &p_name) const = 0;

	virtual void set_name(const String &p_name) = 0;
	virtual void set_input_aliases(const PortAliases &p_aliases) = 0;
	virtual void set_output_aliases(const PortAliases &p_aliases) = 0;

	virtual const String &get_name() const = 0;
	virtual PortAliases get_input_aliases() const = 0;
	virtual PortAliases get_output_aliases() const = 0;

	virtual const StringName &get_input_alias(const StringName &p_port) const = 0;
	virtual const StringName &get_output_alias(const StringName &p_port) const = 0;

	[[nodiscard]] virtual bool supports_children() const = 0;
	[[nodiscard]] virtual bool has_children() const = 0;
	virtual void get_children(Vector<const IPipelineNode *> &p_nodes) const = 0;
	virtual void get_descendants(Vector<const IPipelineNode *> &p_nodes) const = 0;

	virtual bool has_child(const IPipelineNode *p_candidate) const = 0;
	virtual bool has_descendant(const IPipelineNode *p_candidate) const = 0;

protected:
	IPipelineNode() = default;
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

struct IPipelineNodeParent { 
	virtual ~IPipelineNodeParent() = default;

	virtual bool has_child_node(const IPipelineNode *p_node) const = 0;
	virtual bool remove_child_node(const IPipelineNode *p_node) = 0;
	virtual void remove_all_child_nodes() = 0;

protected:
	IPipelineNodeParent() = default;
};


struct IPipelineNodeComposite : public IPipelineNodeParent {
	~IPipelineNodeComposite() override = default;

	virtual bool add_child_node(const IPipelineNode *p_node) = 0;
	virtual void remove_child_node_at(int64_t p_index) = 0;
	virtual void clear() = 0;
	[[nodiscard]] virtual bool is_empty() const = 0;
	[[nodiscard]] virtual const IPipelineNode *get_child_node(int64_t p_index) const = 0;
	virtual void set_child_node(int64_t p_index, const IPipelineNode *p_node) = 0;
	[[nodiscard]] virtual int64_t child_count() const = 0;
	virtual void swap_child_nodes(uint64_t p_first_index, uint64_t p_second_index) = 0;
	virtual Error insert_child_node(int64_t p_pos, const IPipelineNode *p_node) = 0;
	virtual void append_child_nodes(const Vector<const IPipelineNode *> &p_nodes) = 0;

protected:
	IPipelineNodeComposite() = default;
};

struct IPipelineNodeDecorator : public IPipelineNodeParent {
	~IPipelineNodeDecorator() override = default;

	[[nodiscard]] virtual const IPipelineNode *get_child() const = 0;
	virtual void set_child(const IPipelineNode *p_node) = 0;

protected:
	IPipelineNodeDecorator() = default;
};

struct IPipelineGraph {
	virtual ~IPipelineGraph() = default;

	virtual const StringName &graph_type() const = 0;
	virtual const StringName &plugin_name() const = 0;

	[[nodiscard]] virtual RID get_id() const = 0;
	[[nodiscard]] virtual bool is_bound() const = 0;
	[[nodiscard]] virtual uint32_t bind_count() const = 0;
	
	[[nodiscard]] virtual RID create_node(const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases) = 0;
	virtual bool destroy_node(RID p_node) = 0;

	[[nodiscard]] virtual uint64_t nodes_count() const = 0;

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

	[[nodiscard]] virtual bool is_parented(const IPipelineNode *p_node) const = 0;
	virtual void update_parent(const IPipelineNode *p_node, IPipelineNodeParent *p_parent = nullptr) = 0;

protected:

	virtual RID _create_node(const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases) = 0;
	virtual bool _destroy_node(RID p_node_id) = 0;

	friend class IPipeline;
	IPipelineGraph() = default;
};

struct IPipelineNodeSubGraph {
	virtual ~IPipelineNodeSubGraph() = default;

	[[nodiscard]] virtual const IPipelineGraph *get_sub_graph() const = 0;
	virtual void set_sub_graph(IPipelineGraph *p_graph) = 0;

protected:
	IPipelineNodeSubGraph() = default;
};

struct IPipeline {

	virtual ~IPipeline() = default;

	virtual RID get_id() const = 0;
	virtual const StringName &plugin_name() const = 0;

	virtual void execute() = 0;
	virtual void halt() = 0;

	virtual bool owns_source_blackboard() const = 0;
	virtual Blackboard *get_execution_blackboard() const = 0;
	virtual const Blackboard *get_source_blackboard() const = 0;

	virtual const IPipelineGraph *get_graph() const = 0;

	virtual const String &get_error() const = 0;
	virtual void clear_error() const = 0;

protected:
	IPipeline() = default;
};

} // hydrogen

VARIANT_BITFIELD_CAST(hydrogen::pipelines::NodePortInfo::PORT_DIRECTION);

#endif //BEHAVIORS_NODE_BASE_HPP
