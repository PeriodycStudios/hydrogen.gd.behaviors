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

#endif