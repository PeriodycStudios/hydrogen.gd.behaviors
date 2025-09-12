#ifndef PIPELINE_NODE_HPP
#define PIPELINE_NODE_HPP

#include "blackboard.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "node_interfaces.hpp"
#include "rid_data.hpp"

#include <cstdint>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace hydrogen::pipelines {

class PipelineNode : public RidData, public IPipelineNode {
    ABSTRACT_PIPELINE_NODE(PipelineNode, IPipelineNode);

    typedef HashMap<StringName, StringName> AliasMap;

    String _name = "";
    AliasMap _input_aliases = {};
    AliasMap _output_aliases = {};

    static void _setup_aliases(AliasMap &p_internal_aliases, const PortAliases &p_aliases) {
        p_internal_aliases.clear();
        p_internal_aliases.reserve(p_aliases.size());
        TypedArray<StringName> keys = p_aliases.keys();
        for (int32_t idx = 0; idx < keys.size(); ++idx) {
            const StringName key = keys[idx];
            p_internal_aliases.insert(key, p_aliases[key]);
        }
    }

    static PortAliases _get_aliases(const AliasMap &p_internal_aliases) {
        PortAliases aliases = {};
        for (const auto &kvp : p_internal_aliases) {
            aliases.set(kvp.key, kvp.value);
        }

        return aliases;
    }

    static const StringName &_get_alias(const AliasMap &p_aliases, const StringName &p_port_name) {
        const auto iter = p_aliases.find(p_port_name);
        if (likely(iter != p_aliases.end())) {
            return iter->value;
        }
        return p_port_name;
    }

protected:
    PipelineNode() = default;

    static const Vector<NodePortInfo> &_get_ports() {
        static const Vector<NodePortInfo> empty = {};
        return empty;
    }

    static const Vector<NodeConnectionInfo> &_get_connections() {
        static const Vector<NodeConnectionInfo> empty = {};
        return empty;
    }

    template<typename T>
    _FORCE_INLINE_ T _get_port(const Blackboard *p_blackboard, const StringName &p_port_name, T p_default, bool p_check_parents = true) const {
        const StringName &port_name = _get_alias(_input_aliases, p_port_name);
        return p_blackboard->get_entry<T>(port_name, p_default, p_check_parents);
    }

    template<typename T>
    _FORCE_INLINE_ const T &_get_port_fast(const Blackboard *p_blackboard, const StringName &p_port_name, const T &p_default, bool p_check_parents = true) const {
        const StringName &port_name = _get_alias(_input_aliases, p_port_name, p_default, p_check_parents);
        return p_blackboard->get_entry_fast<T>(port_name);
    }

    template<typename T>
    _FORCE_INLINE_ void _set_port(Blackboard *p_blackboard, const StringName &p_port_name, T p_value) const {
        const StringName &port_name = _get_alias(_output_aliases, p_port_name);
        p_blackboard->set_entry<T>(port_name, p_value);
    }

    template<typename T>
    _FORCE_INLINE_ void _set_port_fast(Blackboard *p_blackboard, const StringName &p_port_name, const T &p_value) const {
        const StringName &port_name = _get_alias(_output_aliases, p_port_name);
        p_blackboard->set_entry_fast<T>(port_name, p_value);
    }

public:
    ~PipelineNode() override = default;

    RID get_id() const override { return get_self(); }

    DEFINE_GET_PORTS();
    DEFINE_GET_CONNECTIONS();

    bool set_connection(const StringName &p_name, const IPipelineNode *p_node) override {
        return false;
    }

    const IPipelineNode *get_connection(const StringName &p_name) const override {
        return nullptr;
    }

    void set_name(const String &p_name) override { _name = p_name; }
    
    const String &get_name() const override { return _name; }

    void set_input_aliases(const PortAliases &p_aliases) override {
        _setup_aliases(_input_aliases, p_aliases);
    }

    PortAliases get_input_aliases() const override {
        return _get_aliases(_input_aliases);
    }

    void set_output_aliases(const PortAliases &p_aliases) override {
        _setup_aliases(_output_aliases, p_aliases);
    }

    PortAliases get_output_aliases() const override {
        return _get_aliases(_output_aliases);
    }

    const StringName &get_input_alias(const StringName &p_port) const override {
        return _get_alias(_input_aliases, p_port);
    }

    const StringName &get_output_alias(const StringName &p_port) const override {
        return _get_alias(_output_aliases, p_port);
    }

    bool supports_children() const override { return false; }
    bool has_children() const override { return false; }

    void get_children(Vector<const IPipelineNode *> &p_nodes) const override {}
    void get_descendants(Vector<const IPipelineNode *> &p_nodes) const override {}

    bool has_child(const IPipelineNode *p_node) const override { return false; }
    bool has_descendant(const IPipelineNode *p_node) const override { return false; }
};

}

#define GET_PORT_LOCAL(port_name) _get_port<port_name##_type>(p_context.blackboard(), port_name##_name(), k_default_##port_name, false)
#define GET_PORT_LOCAL_FAST(port_name) _get_port_fast<port_name##_type>(p_context.blackboard(), port_name##_name(), k_default_##port_name, false)

#define GET_PORT(port_name) _get_port<port_name##_type>(p_context.blackboard(), port_name##_name(), k_default_##port_name)
#define GET_PORT_FAST(port_name) _get_port_fast<port_name##_type>(p_context.blackboard(), port_name##_name(), k_default_##port_name)

#define SET_PORT(port_name, value) _set_port<port_name##_type>(p_context.blackboard(), port_name##_name(), value)
#define SET_PORT_FAST(port_name, value) _set_port_fast<port_name##_type>(p_context.blackboard(), port_name##_name(), value)

#define GET_STATE(state_type)                                           \
    state_type * state = p_context.get_state<state_type>(state_key());  \
    ERR_FAIL_NULL(state)                                                \

#define GET_STATE_V(state_type, fail_value)                             \
    state_type *state = p_context.get_state<state_type>(state_key());   \
    ERR_FAIL_NULL_V(state, fail_value)                                  \

#endif