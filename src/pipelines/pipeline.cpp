
#include "pipeline.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/string_name.hpp"

#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/pair.hpp>

namespace hydrogen::pipelines {

Pipeline::Pipeline(const Blackboard *p_source_blackboard, IPipelineGraph *p_graph) :
_instance_blackboard(memnew(Blackboard)), _state_blackboard(memnew(Blackboard)), _graph(p_graph), _mutex(memnew(std::mutex)) {
	_instance_blackboard->set_parent(p_source_blackboard);
	_state_blackboard->set_parent(_instance_blackboard);
	_state_blackboard->set_entry_fast(error_name(), String(""));

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

	memdelete(_state_blackboard);
	memdelete(_instance_blackboard);
	memdelete(_mutex);

	_instance_blackboard = nullptr;
	_state_blackboard = nullptr;
	_graph = nullptr;
	_mutex = nullptr;
}

_FORCE_INLINE_ void Pipeline::set_alias(const StringName &p_name, const StringName &p_alias) {
	std::scoped_lock lock(*mutex());
	_aliases[p_name] = p_alias;
}

_FORCE_INLINE_ bool Pipeline::erase_alias(const StringName &p_name) {
	std::scoped_lock lock(*mutex());
	return _aliases.erase(p_name);
}

const StringName &Pipeline::get_alias(const StringName &p_name) const {
	std::scoped_lock lock(*mutex());
	const auto iter = _aliases.find(p_name);
	if (likely(iter != _aliases.end())) {
		static StringName empty = StringName();
		return empty;
	}
	return iter->value;
}

TypedDictionary<StringName, StringName> Pipeline::get_aliases() const {
	TypedDictionary<StringName, StringName> aliases {};
	std::scoped_lock lock(*mutex());
	for (const auto &kvp : _aliases) {
		aliases[kvp.key] = kvp.value;
	}
	return aliases;
}
}
