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
#include "godot_cpp/core/method_bind.hpp"
#include "godot_cpp/core/type_info.hpp"
#include "godot_cpp/templates/pair.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
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
#include <type_traits>

namespace hydrogen::pipelines {

using namespace godot;

DEFINE_NAME_STATIC(_error);

class IPipelineNode;

typedef std::function<void(Blackboard *)> InputNodeDefaultSetter;

enum class NodePortDirection : uint8_t {
	NONE = 0,
	INPUT = 1 << 0,
	OUTPUT = 1 << 1,
	IN_OUT = NodePortDirection::INPUT | NodePortDirection::OUTPUT,
};

inline NodePortDirection operator | (NodePortDirection p_left, NodePortDirection p_right) {
	uint8_t left = static_cast<uint8_t>(p_left);
	uint8_t right = static_cast<uint8_t>(p_right);

	return static_cast<NodePortDirection>(left | right);
}

inline NodePortDirection operator & (NodePortDirection p_left, NodePortDirection p_right) {
	uint8_t left = static_cast<uint8_t>(p_left);
	uint8_t right = static_cast<uint8_t>(p_right);
	return static_cast<NodePortDirection>(left & right);
}

inline bool operator != (NodePortDirection p_left, NodePortDirection p_right) {
	uint8_t left = static_cast<uint8_t>(p_left);
	uint8_t right = static_cast<uint8_t>(p_right);
	return left != right;
}

struct INodePort {
	virtual const StringName &name() const = 0;
	virtual const StringName &value_type_name() const = 0;
	virtual NodePortDirection direction() const = 0;
	virtual const StringName &port_type_name() const = 0;
	virtual Variant::Type variant_type() const = 0;

	_FORCE_INLINE_ bool is_input() { return (direction() & NodePortDirection::INPUT) != NodePortDirection::NONE; }
	_FORCE_INLINE_ bool is_output() { return (direction() & NodePortDirection::OUTPUT) != NodePortDirection::NONE; }

	virtual Dictionary to_dictionary() const = 0;
	
	virtual ~INodePort() = default;
protected:
	INodePort() = default;
};

class NodePort : public INodePort {
	StringName _name;
	StringName _type_name;
	NodePortDirection _direction;
	Variant::Type _variant_type;

protected:
	NodePort(NodePortDirection p_direction, const StringName &p_name, const StringName &p_type_name, Variant::Type p_variant_type) {
		_name = p_name;
		_type_name = p_type_name;
		_direction = p_direction;
		_variant_type = p_variant_type;
	}

	NodePort() = default;

	template<typename TPORT, typename TVALUE, typename... TARGS>
	static TPORT create(NodePortDirection p_direction, const StringName &p_name, TARGS ... p_args) {
		Variant::Type variant_type;
		if constexpr(traits::is_variant_type_v<TVALUE>) {
			variant_type = static_cast<Variant::Type>(GetTypeInfo<TVALUE>::VARIANT_TYPE);
		}
		else {
			variant_type = Variant::NIL;
		}

		const StringName &type_name = get_type_name_static<TVALUE>();

		return TPORT(p_direction, p_name, type_name, variant_type, p_args...);
	}

	template<typename TPORT, typename TVALUE, typename... TARGS>
	static TPORT create_nil(NodePortDirection p_direction, const StringName &p_name, TARGS ... p_args) {
		const StringName &type_name = get_type_name_static<TVALUE>();
		return TPORT(p_direction, p_name, type_name, Variant::NIL, p_args...);
	}

	template<typename TPORT, typename TVALUE, typename ... TARGS>
	static TPORT create_typed(NodePortDirection p_direction, const StringName &p_name, Variant::Type p_variant_type, TARGS ... p_args) {
		const StringName &type_name = get_type_name_static<TVALUE>();

		return TPORT(p_direction, p_name, type_name, p_variant_type, p_args...);
	}

public:
	const StringName &name() const override { return _name; }
	const StringName &value_type_name() const override { return _type_name; }
	NodePortDirection direction() const override { return _direction; }
	Variant::Type variant_type() const override { return _variant_type; }

	Dictionary to_dictionary() const override {
		Dictionary dict = {};

		dict.set("name", _name);
		dict.set("value_type_name", _type_name);
		dict.set("direction", static_cast<uint8_t>(_direction));
		dict.set("port_type_name", port_type_name());
		dict.set("variant_type", _variant_type);

		return dict;
	}

	~NodePort() override = default;
};

#define PORT_TYPE_NAME(type_name)															\
	static const StringName &port_type_static() {											\
		static const StringName n = StringName("BlackboardPort", true);						\
		return n;																			\
	}																						\
																							\
	const StringName &port_type_name() const override { return port_type_static(); }		\

class BlackboardPort final : public NodePort {
	friend class NodePort;
	BlackboardPort( NodePortDirection p_direction, const StringName &p_name, const StringName &p_type_name, Variant::Type p_variant_type, const std::optional<InputNodeDefaultSetter> p_setter) :
		NodePort(p_direction, p_name, p_type_name, p_variant_type), _default_setter(p_setter)
	{
	}

	const std::optional<InputNodeDefaultSetter> _default_setter;
public:
	PORT_TYPE_NAME(BlackboardPort)

	Dictionary to_dictionary() const override {
		Dictionary dict = NodePort::to_dictionary();

		if (_default_setter.has_value() && variant_type() != Variant::NIL) {
			Blackboard bb = Blackboard();
			const InputNodeDefaultSetter func = _default_setter.value();
			func(&bb);
			dict["default_value"] = bb.get_entry<Variant>(name());
		}

		return dict;
	}

	template<typename T>
	static BlackboardPort create(NodePortDirection p_direction, const StringName &p_name, InputNodeDefaultSetter p_setter = nullptr) {
		return NodePort::create<BlackboardPort, T>(p_direction, p_name, p_setter);
	}

	~BlackboardPort() override = default;
};

class VariablePort final : public NodePort {
	friend class NodePort;

	Callable _get;
	Callable _set;

	VariablePort(NodePortDirection p_direction, const StringName &p_name, const StringName &p_type_name, Variant::Type p_variant_type, Callable p_get = {}, Callable p_set = {}) :
		NodePort(p_direction, p_name, p_type_name, p_variant_type), _get(p_get), _set(p_set) {}

public:
	PORT_TYPE_NAME(VariablePort)

	Dictionary to_dictionary() const override {
		Dictionary dict = NodePort::to_dictionary();

		if (likely(has_getter())) {
			dict.set("get", _get);
		}
		if (likely(has_setter())) {
			dict.set("set", _set);
		}

		return dict;
	}

	template<typename T>
	static VariablePort create(NodePortDirection p_direction, const StringName &p_name, Callable p_get = {}, Callable p_set = {}) {
		return NodePort::create<VariablePort, T>(p_direction, p_name, p_get, p_set);
	}

	_FORCE_INLINE_ bool has_getter() const { return _get.is_valid(); }
	_FORCE_INLINE_ bool has_setter() const { return _set.is_valid(); }

	const Callable &get() const { return _get; }
	const Callable &set() const { return _set; }

	~VariablePort() override = default;

};

struct ConnectionPort final : public NodePort {
	PORT_TYPE_NAME(ConnectionPort)

	template<typename T>
	ConnectionPort create(NodePortDirection p_direction, const StringName &p_name) {
		return NodePort::create_nil<ConnectionPort, T>(p_direction, p_name);
	}

	~ConnectionPort() override = default;

private:
	friend class NodePort;
	ConnectionPort(NodePortDirection p_direction, const StringName &p_name, const StringName &p_type_name, Variant::Type p_variant_type) : 
		NodePort(p_direction, p_name, p_type_name, p_variant_type) {}
};

struct FunctionPort final : public NodePort {
	PORT_TYPE_NAME(FunctionPort)

	const Callable func() const { return _callable; }

	Dictionary to_dictionary() const override {
		Dictionary dict = NodePort::to_dictionary();

		dict.set("function", _callable);
	}

	template<typename T>
	FunctionPort create(NodePortDirection p_direction, const StringName &p_name, Callable p_callable) {
		return NodePort::create_typed<FunctionPort, T>(p_direction, p_name, Variant::CALLABLE, p_callable);
	}

	~FunctionPort() override = default;

private:
	const Callable _callable;

	friend class NodePort;
	FunctionPort(NodePortDirection p_direction, const StringName &p_name, const StringName &p_type_name, Variant::Type p_variant_type, Callable p_callable) : 
		NodePort(p_direction, p_name, p_type_name, p_variant_type), _callable(p_callable) {}
};

#undef PORT_TYPE_NAME

template<typename T, typename = void>
struct has_ports : std::false_type {};

template<typename T>
struct has_ports<T, 
	std::enable_if_t<std::is_same_v<const Vector<const INodePort *>&, decltype(T::get_ports())>>>
	: std::true_type {};

#define BEGIN_PORTS()																	\
	inline static Vector<const INodePort *> _ports = {};								\
	inline static void _register_ports() {												\
		if constexpr (has_ports<parent_type>::value) {									\
			const Vector<const INodePort *> &parent_ports = parent_type::get_ports();	\
			_ports.append_array(parent_ports);											\
		}																				\

#define _BLACKBOARD_PORT(direction, port_name, default_setter)												\
		_ports.push_back(memnew(BlackboardPort::create(direction, port_name##_name(), default_setter)));	\

#define BLACKBOARD_INPUT(port_name, default_setter) _BLACKBOARD_PORT(NodePortDirection::INPUT, port_name, default_setter)
#define BLACKBOARD_OUTPUT(port_name) _BLACKBOARD_PORT(NodePortDirection::OUTPUT, port_name, nullptr)
#define BLACKBOARD_IN_OUT(port_name, default_setter) _BLACKBOARD_PORT(NodePortDirection::IN_OUT, port_name, default_setter)



#define END_PORTS()											\
	}														\
															\
	inline static void _unregister_ports() {				\
		for(const INodePort *port : _ports) {				\
			memdelete(port);								\
		}													\
	}														\

	

// struct NodePortInfo {

// 	enum PORT_KIND {
// 		NONE = 0,
// 		INPUT = 1 << 0,
// 		OUTPUT = 1 << 1,
// 		IN_OUT = INPUT | OUTPUT,                                                                                                                                                                                                                                                      
// 	};

// 	StringName name = "";
// 	StringName type_name = "";
// 	Variant::Type variant_type = Variant::NIL;
// 	std::optional<InputNodeDefaultSetter> default_setter = nullptr;
// 	PORT_KIND port_kind = NONE;

// 	_FORCE_INLINE_ bool is_input() const { return (port_kind & INPUT) != 0; }
// 	_FORCE_INLINE_ bool is_output() const { return (port_kind & OUTPUT) != 0; }

// 	constexpr static bool is_true() { return true; }
// 	constexpr static bool is_false() { return false; }

// 	Dictionary to_dictionary(const IPipelineNode *p_node, Blackboard *_blackboard) const {
// 		Dictionary dict = {};

// 		dict["name"] = name;
// 		dict["type_name"] = type_name;
// 		dict["variant_type"] = variant_type;
		
// 		if (default_setter.has_value() && variant_type != Variant::NIL) {
// 			default_setter.value()(p_node, _blackboard);
// 			Variant default_value = _blackboard->get_entry<Variant>(name);
// 			dict["default_value"] = default_value;
// 		}

// 		dict["port_kind"] = port_kind;
// 		dict["is_input"] = is_input();
// 		dict["is_output"] = is_output();

// 		return dict;
// 	}

// 	template<typename T> 
// 	static NodePortInfo create_input(const StringName &p_name, const InputNodeDefaultSetter p_setter) {
// 		return create<T>(p_name, INPUT, p_setter);
// 	}

// 	template<typename T>
// 	static NodePortInfo create_output(const StringName &p_name) {
// 		return create<T>(p_name, OUTPUT);
// 	}

// 	template<typename T>
// 	static NodePortInfo create_in_out(const StringName &p_name, const InputNodeDefaultSetter p_setter) {
// 		return create<T>(p_name, IN_OUT, p_setter);
// 	}

// 	NodePortInfo() {}

// private:

// 	template<typename T>
// 	static NodePortInfo create(const StringName &p_name, PORT_KIND p_kind, std::optional<InputNodeDefaultSetter> p_setter = nullptr) {

// 		Variant::Type variant_type;
// 		if constexpr(traits::is_variant_type_v<T>) {
// 			variant_type = static_cast<Variant::Type>(GetTypeInfo<T>::VARIANT_TYPE);
// 		}
// 		else {
// 			variant_type = Variant::NIL;
// 		}

// 		const StringName &type_name = get_type_name_static<T>();

// 		return NodePortInfo(p_name, type_name, variant_type, p_setter, p_kind);
// 	}

// 	NodePortInfo(const StringName &p_name, const StringName &p_type_name, const Variant::Type p_variant_type, const std::optional<InputNodeDefaultSetter> p_setter, PORT_KIND p_kind) 
// 	: name(p_name), type_name(p_type_name), variant_type(p_variant_type), default_setter(p_setter), port_kind(p_kind) {}
// };

// struct NodeConnectionInfo {
// 	StringName name = "";
// 	StringName type_name = "";

// 	template<typename T>
// 	static NodeConnectionInfo create(const StringName &p_name) {
// 		return NodeConnectionInfo(p_name, get_type_name_static<T>());
// 	}

// 	Dictionary to_dictionary() const {
// 		Dictionary dict = {};
// 		dict["name"] = name;
// 		dict["type_name"] = type_name;
// 		return dict;
// 	}

// 	NodeConnectionInfo() {}

// private:
// 	NodeConnectionInfo(const StringName &p_name, const StringName &p_type_name) 
// 		: name(p_name), type_name(p_type_name) 
// 		{}
// };

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

#define DEFINE_GET_PORTS()										\
    const Vector<NodePortInfo> &get_ports() const override {	\
        return _get_ports();									\
    }															\

class IPipelineGraph;

typedef TypedDictionary<StringName, StringName> PortAliases;

struct IPipelineNode {
	virtual ~IPipelineNode() = default;

	virtual RID get_id() const = 0;

	[[nodiscard]] virtual const StringName &get_type_name() const = 0;

	virtual const Vector<const INodePort *> &get_ports() const = 0;
	// virtual const Vector<NodeConnectionInfo> &get_connections() const = 0;

	// virtual bool set_connection(const StringName &p_name, const IPipelineNode *p_target) = 0;
	// virtual const IPipelineNode *get_connection(const StringName &p_name) const = 0;

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

VARIANT_BITFIELD_CAST(hydrogen::pipelines::NodePortDirection);
// VARIANT_ENUM_CAST(hydrogen::pipelines::NodePortKind);
// VARIANT_BITFIELD_CAST(hydrogen::pipelines::NodePortInfo::PORT_KIND);

#endif //BEHAVIORS_NODE_BASE_HPP
