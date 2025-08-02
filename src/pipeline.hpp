//
// Created by tkey on 7/20/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <godot_cpp/templates/hash_map.hpp>

#include "pipeline_nodes.hpp"
#include "rid_data.hpp"
#include "pipeline_graph.hpp"

namespace hydrogen::pipelines {

class Pipeline : public RidData {

	Blackboard *_state_blackboard = nullptr;
	IPipelineGraph *_graph = nullptr;

	const PipelineNode *_root = nullptr;
	bool _is_bound = false;

protected:

	typedef HashMap<RID, const PipelineNode*> NodeMap;
	typedef HashMap<RID, IPipelineNodeState *> NodeStateMap;

	NodeMap _nodes = {};
	NodeStateMap _states = {};

	explicit Pipeline(const Blackboard *p_source_blackboard, const PipelineNode * p_root_node = nullptr);
	virtual ~Pipeline();

	// _FORCE_INLINE_ bool set_pipeline_graph(IPipelineGraph *p_graph) {
	// 	if (_graph != nullptr) {
	// 		_graph->
	// 	}
	// }

	bool set_pipeline_root(const PipelineNode *p_node);

	Blackboard *get_blackboard() { return _state_blackboard; }

public:

	virtual void execute() = 0;

	virtual void bind();
	virtual void unbind();
	_FORCE_INLINE_ bool is_bound() const { return _is_bound; }

	static void register_types();

	const Blackboard *get_blackboard() const { return _state_blackboard; }
	[[nodiscard]] _FORCE_INLINE_ const PipelineNode *get_pipeline_root() const { return _root; }

	_FORCE_INLINE_ void halt() const {
		_state_blackboard->set_entry_fast(halting_name(), true);
	}

	[[nodiscard]] _FORCE_INLINE_ bool is_halting() const {
		return _state_blackboard->get_entry<bool>(halting_name());
	}

	[[nodiscard]] virtual bool is_fully_halted() const = 0;

	_FORCE_INLINE_ void clear_halt() const {
		_state_blackboard->set_entry_fast(halting_name(), false);
	}

	[[nodiscard]] _FORCE_INLINE_ String get_error() const {
		return _state_blackboard->get_entry_fast<String>(error_name(), "", false);
	}
};
}

#endif //PIPELINE_HPP
