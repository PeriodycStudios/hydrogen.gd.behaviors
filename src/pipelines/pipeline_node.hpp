#ifndef PIPELINE_NODE_HPP
#define PIPELINE_NODE_HPP

#include "node_interfaces.hpp"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/local_vector.hpp>

#include <cstdint>

namespace hydrogen::pipelines {

class PipelineNode : public IPipelineNode {
protected:
    PipelineNode() = default;

public:
    ~PipelineNode() override = default;

    [[nodiscard]] int32_t get_input_port_count() const override {
        return 0;
    }

    [[nodiscard]] int32_t get_output_port_count() const override {
        return 0;
    }

    [[nodiscard]] StringName get_input_port_type_name(int32_t p_port) const override {
        return StringName();
    }

    [[nodiscard]] StringName get_output_port_type_name(int32_t p_port) const override {
        return StringName();
    }

    [[nodiscard]] StringName get_input_port_name(int32_t p_port) const override {
        return StringName();
    }

    [[nodiscard]] StringName get_output_port_name(int32_t p_port) const override {
        return StringName();
    }

    void get_input_port_infos(Vector<NodePortInfo> &p_infos) const override {}
    void get_output_port_infos(Vector<NodePortInfo> &p_infos) const override {}
};

}

#define DECLARE_INPUT_PORTS()                       \
const static Vector<NodePortInfo> _input_ports	    \

#define DECLARE_OUTPUT_PORTS()                      \
const static Vector<NodePortInfo> _output_ports	    \

#define DECLARE_PIPELINE_NODE(type_name, graph_node_base)	                        \
	DEFINE_NAME_STATIC(type_name);					                                \
    typedef graph_node_base node_type;                                              \
                                                                                    \
public:												                                \
	static StringName node_name() {					                                \
		return type_name##_name();					                                \
	}												                                \
                                                                                    \
	StringName get_type_name() const override {		                                \
		return type_name##_name();					                                \
	}												                                \
private:											                                \

#define BEGIN_DEFINE_INPUT_PORTS(node_type)     \
	node_type::_input_ports = {                 \

#define BEGIN_DEFINE_OUTPUT_PORTS(node_type)    \
    node_type::_output_ports = {                \

#define DECLARE_PORT(port_name, port_type, default_value)                               \
    const StringName &get_port_##port_name() {                                          \
        const static StringName name(#port_name, true);                                 \
        return name;                                                                    \
    }                                                                                   \
                                                                                        \
    static void port_name##_setter(Blackboard * p_blackboard) {                         \
        const static port_type d_value = default_value;                                 \
        p_blackboard->set_entry_fast<port_type>(get_port_##port_name(), default_value); \
    }                                                                                   \

#define DEFINE_PORT(port_name, port_type)                                   \
    NodePortInfo::create_port<port_type>(port_name, port_name##_setter),    \

#define END_PORTS() \
};                  \

#define DECLARE_INPUT_PORT_FUNCS()                                                      \
    [[nodiscard]] int32_t get_input_port_count() const override;                        \
    [[nodiscard]] StringName get_input_port_type_name(int32_t p_port) const override;   \
    [[nodiscard]] StringName get_input_port_name(int32_t p_port) const override;        \
    void get_input_port_infos(Vector<NodePortInfo> &p_infos) const override;            \

#define DECLARE_OUTPUT_PORT_FUNCS()                                                     \
    [[nodiscard]] int32_t get_output_port_count() const override;                       \
    [[nodiscard]] StringName get_output_port_type_name(int32_t p_port) const override;  \
    [[nodiscard]] StringName get_output_port_name(int32_t p_port) const override;       \
    void get_output_port_infos(Vector<NodePortInfo> &p_infos) const override;           \

#define DEFINE_PORT_FUNCS(node_type, kind)                                  \
int32_t node_type::get_kind_port_count() const {                            \
    return _kind_ports.size();                                              \
}                                                                           \
                                                                            \
StringName node_type::get_kind_port_type_name(int32_t p_port) const {       \
    ERR_FAIL_INDEX_V(p_port, _kind_ports.size(), StringName());             \
    return _kind_ports[p_port].type_name;                                   \
}                                                                           \
                                                                            \
StringName node_type::get_kind_port_name(int32_t p_port) const {            \
    ERR_FAIL_INDEX_V(p_port, _kind_ports.size(), StringName());             \
    return _kind_ports[p_port].name;                                        \
}                                                                           \

#endif