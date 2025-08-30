
#include "pipeline.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "pipelines/node_interfaces.hpp"

#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/pair.hpp>

namespace hydrogen::pipelines {

Pipeline::Pipeline(const Blackboard *p_source_blackboard, IPipelineGraph *p_graph)
	: _blackboard(memnew(Blackboard)), _graph(p_graph), _mutex(memnew(std::mutex)) {
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

Pipeline::~Pipeline() {
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
}
