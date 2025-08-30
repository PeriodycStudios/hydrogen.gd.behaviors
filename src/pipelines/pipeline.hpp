//
// Created by tkey on 7/20/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "../blackboard.hpp"
#include "../rid_data.hpp"
#include "pipeline_graph.hpp"
#include "node_interfaces.hpp"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>
#include <mutex>

namespace hydrogen::pipelines {

using namespace godot;

class Pipeline : public RidData {
	Blackboard *_blackboard;
	IPipelineGraph *_graph;
	NodeStateMap _node_states {};
	std::mutex *_mutex;

protected:
	[[nodiscard]] _FORCE_INLINE_ std::mutex *mutex() const { return _mutex; }
	_FORCE_INLINE_ NodeStateMap &node_states() { return _node_states; }

	explicit Pipeline(const Blackboard *p_source_blackboard, IPipelineGraph *p_graph);

	[[nodiscard]] _FORCE_INLINE_ const IPipelineNode *get_pipeline_root() const { return _graph->get_root_node(); }

public:
	virtual ~Pipeline();

	virtual void execute() = 0;
	virtual void halt() = 0;

	[[nodiscard]] _FORCE_INLINE_ Blackboard *get_blackboard() const { return _blackboard; }

	[[nodiscard]] _FORCE_INLINE_ String get_error() const {
		return _blackboard->get_entry_fast<String>(_error_name(), "", false);
	}
};
}

#endif //PIPELINE_HPP
