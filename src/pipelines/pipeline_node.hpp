#ifndef PIPELINE_NODE_HPP
#define PIPELINE_NODE_HPP

#include "godot_cpp/variant/string_name.hpp"
#include "node_interfaces.hpp"
#include <cstdint>
#include <godot_cpp/templates/vector.hpp>

namespace hydrogen::pipelines {

#define DECLARE_INPUT_PORTS()                       \
    const static Vector<NodePortInfo> _input_ports; \

#define BEGIN_INPUT_PORTS(type)     \
    const type::_input_ports = {    \

#define DECLARE_OUTPUT_PORTS()                      \
    const static Vector<NodePortInfo> _output_ports;\

#define BEGIN_OUTPUT_PORTS(type)    \
const type::_output_ports = {       \

#define DECLARE_PORT(name, type)            \
    NodePortInfo::create_port<type>(name),  \

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

#define _DEFINE_PORT_FUNCS(node_type, kind)                                 \
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
                                                                            \
void node_type::get_kind_port_infos(Vector<NodePortInfo> &p_infos) const {  \
    const int32_t ports_count = get_kind_port_count();                      \
    p_infos.append_array(_kind_ports);                                      \
}                                                                           \

#define DEFINE_INPUT_PORT_FUNCS(node_type)  \
_DEFINE_PORT_FUNCS(node_type, input)

#define DEFINE_OUTPUT_PORT_FUNCS(node_type) \
_DEFINE_PORT_FUNCS(node_type, output)

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

#endif