//
// Created by tkey on 7/20/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP


#include "../blackboard.hpp"
#include "../rid_data.hpp"
#include "pipeline_graph.hpp"
#include "pipeline_nodes.hpp"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/core/defs.hpp>
#include <mutex>

namespace hydrogen::pipelines {

using namespace godot;

class Pipeline : public RidData {
	Blackboard *_instance_blackboard;
	Blackboard *_state_blackboard;
	IPipelineGraph *_graph;
	NodeStateMap _node_states {};
	HashMap<StringName, StringName> _aliases {};
	std::mutex *_mutex;

protected:
	[[nodiscard]] _FORCE_INLINE_ std::mutex *mutex() const { return _mutex; }
	_FORCE_INLINE_ NodeStateMap &node_states() { return _node_states; }
	[[nodiscard]] _FORCE_INLINE_ HashMap<StringName, StringName> &aliases() { return _aliases; }

	explicit Pipeline(const Blackboard *p_source_blackboard, IPipelineGraph * p_graph);


	[[nodiscard]] _FORCE_INLINE_ Blackboard *get_state_blackboard() const { return _state_blackboard; }
	[[nodiscard]] _FORCE_INLINE_ const Blackboard *get_readonly_state_blackboard() const { return _state_blackboard; }
	[[nodiscard]] _FORCE_INLINE_ const IPipelineNode *get_pipeline_root() const { return _graph->get_pipeline_root(); }

public:
	virtual ~Pipeline();

	_FORCE_INLINE_ void set_alias(const StringName &p_name, const StringName &p_alias) {
		std::scoped_lock lock(*mutex());
		_aliases[p_name] = p_alias;
	}

	_FORCE_INLINE_ bool erase_alias(const StringName &p_name) {
		std::scoped_lock lock(*mutex());
		return _aliases.erase(p_name);
	}

	[[nodiscard]] const StringName &get_alias(const StringName &p_name) const {
		std::scoped_lock lock(*mutex());
		const auto iter = _aliases.find(p_name);
		if (likely(iter != _aliases.end())) {
			static StringName empty = StringName();
			return empty;
		}
		return iter->value;
	}

	[[nodiscard]] TypedDictionary<StringName, StringName> get_aliases() const;

	virtual void execute() = 0;
	virtual void halt() = 0;

	[[nodiscard]] _FORCE_INLINE_ Blackboard *get_blackboard() const { return _instance_blackboard; }

	[[nodiscard]] _FORCE_INLINE_ String get_error() const {
		return _state_blackboard->get_entry_fast<String>(error_name(), "", false);
	}
};
}

#endif //PIPELINE_HPP
