//
// Created by tkey on 7/20/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <godot_cpp/templates/hash_map.hpp>

#include "pipeline_nodes.hpp"
#include "rid_data.hpp"

namespace hydrogen {

class Pipeline : public RidData {

	const PipelineNode *_root;
	bool _is_bound = false;

protected:

	Blackboard *_state_blackboard = nullptr;

	typedef HashMap<RID, const PipelineNode*> NodeMap;
	typedef HashMap<RID, IPipelineNodeState *> NodeStateMap;

	NodeMap _nodes = {};
	NodeStateMap _states = {};

	Pipeline(const Blackboard *p_source_blackboard, const PipelineNode * p_root_node);
	virtual ~Pipeline();

public:
	virtual void execute() = 0;

	virtual void bind();
	virtual void unbind();
	_FORCE_INLINE_ bool is_bound() const { return _is_bound; }

	static void register_types();

	_FORCE_INLINE_ void halt() const {
		_state_blackboard->set_entry_fast(pipeline_nodes::halting_name(), true);
	}

	[[nodiscard]] _FORCE_INLINE_ bool is_halting() const {
		return _state_blackboard->get_entry<bool>(pipeline_nodes::halting_name());
	}

	[[nodiscard]] virtual bool is_fully_halted() const = 0;

	_FORCE_INLINE_ void clear_halt() const {
		_state_blackboard->set_entry_fast(pipeline_nodes::halting_name(), false);
	}

	[[nodiscard]] _FORCE_INLINE_ const Blackboard *get_blackboard() const { return _state_blackboard; }
	[[nodiscard]] _FORCE_INLINE_ const PipelineNode *get_root() const { return _root; }

	[[nodiscard]] _FORCE_INLINE_ String get_error() const {
		return _state_blackboard->get_entry_fast<String>(pipeline_nodes::error_name(), "", false);
	}
};
}

#endif //PIPELINE_HPP
