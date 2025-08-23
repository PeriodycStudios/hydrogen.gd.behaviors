#ifndef PIPELINE_NODE_HPP
#define PIPELINE_NODE_HPP

#include "godot_cpp/templates/vector.hpp"
#include "node_interfaces.hpp"
#include "rid_data.hpp"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace hydrogen::pipelines {

class PipelineNode : public RidData, public IPipelineNode {
protected:
    PipelineNode() = default; 

public:
    ~PipelineNode() override = default;

    RID get_id() const override { return get_self(); }

    const Vector<NodePortInfo> &get_port_infos() const override {
        static const Vector<NodePortInfo> empty = {};
        return empty; 
    }

    // [[nodiscard]] int32_t get_input_port_count() const override {
    //     return 0;
    // }

    // [[nodiscard]] int32_t get_output_port_count() const override {
    //     return 0;
    // }

    // [[nodiscard]] StringName get_input_port_type_name(int32_t p_port) const override {
    //     return StringName();
    // }

    // [[nodiscard]] StringName get_output_port_type_name(int32_t p_port) const override {
    //     return StringName();
    // }

    // [[nodiscard]] StringName get_input_port_name(int32_t p_port) const override {
    //     return StringName();
    // }

    // [[nodiscard]] StringName get_output_port_name(int32_t p_port) const override {
    //     return StringName();
    // }

    // void get_input_port_infos(Vector<NodePortInfo> &p_infos) const override {}
    // void get_output_port_infos(Vector<NodePortInfo> &p_infos) const override {}

    bool supports_children() const override { return false; }
    bool has_children() const override { return false; }

    void get_children(Vector<const IPipelineNode *> &p_nodes) const override {}
    void get_descendants(Vector<const IPipelineNode *> &p_nodes) const override {}
};

}

#define DECLARE_PIPELINE_NODE(type_name)        \
	DEFINE_NAME_STATIC(type_name);              \
                                                \
public:										    \
	static StringName node_name() {			    \
		return type_name##_name();			    \
	}										    \
                                                \
	StringName get_type_name() const override { \
		return type_name##_name();			    \
	}										    \
                                                \
private:                                        \

#define DECLARE_GET_PORTS()                                         \
    const Vector<NodePortInfo> &get_port_infos() const override {   \
        return get_ports();                                         \
    }                                                               \



// #define DECLARE_INPUT_PORTS()                       \
// const static Vector<NodePortInfo> _input_ports	    \

// #define DECLARE_OUTPUT_PORTS()                      \
// const static Vector<NodePortInfo> _output_ports	    \

// #define _DECLARE_PORT_NAME(port_name)                   \
//     static const StringName &port_name##_name() {       \
//         const static StringName name(#port_name, true); \
//         return name;                                    \
//     }                                                   \

// #define _DECLARE_PORT_SETTER(port_name, port_type)                                      \
//     static void port_name##_set(Blackboard * p_blackboard, const port_type &p_value) {  \
//         p_blackboard->set_entry_fast<port_type>(get_port_##port_name(), p_value);       \
//     }                                                                                   \

// #define _DECLARE_PORT_GETTER(port_name, port_type, default_value)                           \
//     static const port_type &port_name##_get(Blackboard *p_blackboard) {                     \
//         return p_blackboard->get_entry_fast<port_type>(port_name##_name(), default_value);  \
//     }                                                                                       \
//                                                                                             \

// #define DECLARE_INPUT_PORT(port_name, port_type, default_value) \
//     _DECLARE_PORT_NAME(port_name);                              \
//     _DECLARE_PORT_GETTER(port_name, port_type, default_value);  \

// #define DECLARE_OUTPUT_PORT(port_name, port_type)   \
//     _DECLARE_PORT_NAME(port_name);                  \
//     _DECLARE_PORT_SETTER(port_name, port_type);     \

// #define DECLARE_INPUT_OUTPUT_PORT(port_name, port_type, default_value)  \
//     _DECLARE_PORT_NAME(port_name);                                      \
//     _DECLARE_PORT_GETTER(port_name, port_type, default_value);          \
//     _DECLARE_PORT_SETTER(port_name, port_type);                         \

// #define DECLARE_INPUT_PORT_FUNCS()                                                      \
//     [[nodiscard]] int32_t get_input_port_count() const override;                        \
//     [[nodiscard]] StringName get_input_port_type_name(int32_t p_port) const override;   \
//     [[nodiscard]] StringName get_input_port_name(int32_t p_port) const override;        \
//     void get_input_port_infos(Vector<NodePortInfo> &p_infos) const override             \

// #define DECLARE_OUTPUT_PORT_FUNCS()                                                     \
//     [[nodiscard]] int32_t get_output_port_count() const override;                       \
//     [[nodiscard]] StringName get_output_port_type_name(int32_t p_port) const override;  \
//     [[nodiscard]] StringName get_output_port_name(int32_t p_port) const override;       \
//     void get_output_port_infos(Vector<NodePortInfo> &p_infos) const override            \

// #define BEGIN_DEFINE_INPUT_PORTS(node_type)             \
// const Vector<NodePortInfo> node_type::_input_ports = {  \

// #define BEGIN_DEFINE_OUTPUT_PORTS(node_type)            \
// const Vector<NodePortInfo> node_type::_output_ports = { \

// #define DEFINE_PORT_INFO(port_name, port_type)          \
//     NodePortInfo::create_port<port_type>(#port_name),   \

// #define END_DEFINE_PORTS()  \
// };                          \

// #define _DEFINE_PORT_FUNCS(node_type, kind)                             \
// int32_t node_type::get_kind_port_count() const {                        \
//     return _kind_ports.size();                                          \
// }                                                                       \
//                                                                         \
// StringName node_type::get_kind_port_type_name(int32_t p_port) const {   \
//     ERR_FAIL_INDEX_V(p_port, _kind_ports.size(), StringName());         \
//     return _kind_ports[p_port].type_name;                               \
// }                                                                       \
//                                                                         \
// StringName node_type::get_kind_port_name(int32_t p_port) const {        \
//     ERR_FAIL_INDEX_V(p_port, _kind_ports.size(), StringName());         \
//     return _kind_ports[p_port].name;                                    \
// }                                                                       \

// #define DEFINE_INPUT_PORT_FUNCS(node_type) _DEFINE_PORT_FUNCS(node_type, input);

// #define DEFINE_OUTPUT_PORT_FUNCS(node_type) _DEFINE_PORT_FUNCS(node_type, output);

#endif