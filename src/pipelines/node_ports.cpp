
#include "godot_cpp/core/binder_common.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "node_interfaces.hpp"

namespace hydrogen::pipelines {

Dictionary NodePortInfo::to_dictionary(const IPipelineNode *p_node, Blackboard *_blackboard) const {
    Dictionary dict = {};

    dict["name"] = name;
    dict["type_name"] = type_name;
    dict["variant_type"] = variant_type;
    
    if (default_setter.has_value() && variant_type != Variant::NIL) {
        default_setter.value()(p_node, _blackboard);
        Variant default_value = _blackboard->get_entry<Variant>(name);
        dict["default_value"] = default_value;
    }

    dict["port_kind"] = direction;
    dict["is_input"] = is_input();
    dict["is_output"] = is_output();
    dict["is_graph_port"] = is_graph_port;

    return dict;
}

void NodePortInfo::from_dictionary(Dictionary p_dict) {
    ERR_FAIL_COND(p_dict.is_empty());

    name = p_dict["name"];
    type_name = p_dict["type_name"];
    variant_type = VariantCaster<Variant::Type>::cast(p_dict["variant_type"]);

    Variant default_value = p_dict["default_value"];
    if (likely(default_value != Variant())) {
        StringName port_name = name;
        default_setter = [default_value, port_name](const IPipelineNode *p_node, Blackboard *p_blackboard) {
            const StringName &alias = p_node->get_input_alias(port_name);
            p_blackboard->set_entry<Variant>(alias, default_value);
        };
    }

    direction = VariantCaster<PORT_DIRECTION>::cast(p_dict["direction"]);
    is_graph_port = p_dict["is_graph_port"];
}

}