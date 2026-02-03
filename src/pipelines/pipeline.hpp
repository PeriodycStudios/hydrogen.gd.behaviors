//
// Created by tkey on 7/20/25.
//

#pragma once

#include "../blackboard.hpp"
#include "../rid_data.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "pipeline_graph.hpp"
#include "node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>
#include <mutex>
#include <type_traits>

namespace hydrogen::pipelines {

using namespace godot;

template<typename TGRAPH, typename TNODE, typename = void>
class Pipeline {}; 

template<typename TGRAPH, typename TNODE>
class Pipeline<TGRAPH, TNODE, std::enable_if_t<
	std::is_base_of_v<PipelineGraph<TNODE>, TGRAPH> 
	&& std::is_base_of_v<PipelineNode, TNODE>>>
	: public IPipeline, public RidData {
	StringName _plugin_name;
	Blackboard *_blackboard;
	TGRAPH *_graph;
	NodeStateMap _node_states = {};
	std::recursive_mutex *_mutex;
	bool _owns_source_blackboard;

protected:
	[[nodiscard]] _FORCE_INLINE_ std::recursive_mutex *mutex() const { return _mutex; }
	_FORCE_INLINE_ NodeStateMap &node_states() { return _node_states; }

	explicit Pipeline(const StringName &p_key, const Blackboard *p_source_blackboard, TGRAPH *p_graph, bool p_auto_delete_blackboard) 
		: _plugin_name(p_key), _blackboard(memnew(Blackboard)), _graph(p_graph), _mutex(memnew(std::recursive_mutex))  {
		_graph->bind();

		_blackboard->set_parent(p_source_blackboard);
		_blackboard->set_entry_fast(_error_name(), String(""));

		Vector<const IPipelineNode *> collected_nodes = {};
		_graph->get_nodes(collected_nodes);
		LocalVector<Pair<RID, const IPipelineNodeStateful *>> stateful_nodes = {};
		stateful_nodes.reserve(collected_nodes.size());

		for (const IPipelineNode *node : collected_nodes) {
			const IPipelineNodeStateful *stateful_node = dynamic_cast<const IPipelineNodeStateful *>(node);
			if (likely(stateful_node == nullptr)) {
				continue;
			}

			stateful_nodes.push_back(Pair(stateful_node->state_key(), stateful_node));
		}

		_node_states.reserve(stateful_nodes.size());
		for (const auto & kvp : stateful_nodes) {
			_node_states.insert(kvp.first, kvp.second->create_state());
		}
	}

	[[nodiscard]] _FORCE_INLINE_ const TNODE *get_pipeline_root() const { return _graph->get_root_typed(); }

public:
	virtual ~Pipeline() {
		_graph->unbind();

		for (const auto &kvp : _node_states) {
			IPipelineNodeState *state = kvp.value;
			ERR_CONTINUE(state == nullptr);
			memdelete(state);
		}
		_node_states.clear();

		memdelete(_blackboard);
		memdelete(_mutex);

		_blackboard = nullptr;
		_graph = nullptr;
		_mutex = nullptr;
	}

	[[nodiscard]] RID get_id() const override { return get_self(); }
	[[nodiscard]] const StringName &plugin_name() const override { return _plugin_name; }

	[[nodiscard]] bool owns_source_blackboard() const override { return _owns_source_blackboard; }

	[[nodiscard]] Blackboard *get_execution_blackboard() const override { return _blackboard; }
	[[nodiscard]] const Blackboard *get_source_blackboard() const override { return _blackboard->get_parent(); }

	[[nodiscard]] const IPipelineGraph *get_graph() const override { return _graph; }

	[[nodiscard]] const String &get_error() const override {
		return _blackboard->get_entry_fast<String>(_error_name(), "", false);
	}

	void clear_error() const override {
		_blackboard->set_entry(_error_name(), "");
	}
};
}
