
#include "pipeline.hpp"

#include <godot_cpp/templates/local_vector.hpp>

namespace hydrogen::pipelines {

void IPipelineNodeStateful::cleanup_state(IPipelineNodeState *p_state) const { memdelete(p_state); }

Pipeline::Pipeline(const Blackboard *p_source_blackboard, const PipelineNode *p_root_node) : _state_blackboard(memnew(Blackboard)), _root(p_root_node)  {
	_root = p_root_node;
	_state_blackboard->set_parent(p_source_blackboard);
	_state_blackboard->set_entry_fast(error_name(), String(""));
	_state_blackboard->set_entry_fast(halting_name(), false);
	_state_blackboard->set_entry_fast(node_states_name(), &_states);
	_state_blackboard->set_entry_fast(pipeline_name(), this);
}

Pipeline::~Pipeline() {
	memdelete(_state_blackboard);
}

bool Pipeline::set_pipeline_root(const PipelineNode *p_node) {
	if (_is_bound) {
		WARN_PRINT("Cannot set pipeline root while pipeline is bound.");
		return false;
	}

	_root = p_node;
	return true;
}

void Pipeline::bind() {
	if (unlikely(_root == nullptr || _is_bound)) {
		return;
	}

	LocalVector<PipelineNode *> collected_nodes = {};
	LocalVector<const IPipelineNodeStateful *> stateful_nodes = {};

	std::function<void(PipelineNode *)> collect = [&collected_nodes, &collect](PipelineNode *p_node) {
		collected_nodes.push_back(p_node);

		const IPipelineNodeContainer *container_node = dynamic_cast<const IPipelineNodeContainer *>(p_node);
		if (unlikely(container_node != nullptr)) {
			const uint32_t nodes_count = container_node->get_node_count();
			for (uint32_t i = 0; i < nodes_count; i++) {
				PipelineNode *node = container_node->get_node(i);
				collect(node);
			}
		}

		const IPipelineNodeWrapper *wrapper_node = dynamic_cast<const IPipelineNodeWrapper *>(container_node);
		if (unlikely(wrapper_node != nullptr)) {
			PipelineNode *wrapped_node = wrapper_node->get_pipeline_node();
			if (likely(wrapped_node != nullptr)) {
				collect(wrapped_node);
			}
		}
	};

	PipelineNode *root = const_cast<PipelineNode *>(_root);
	collect(root);

	_nodes.reserve(collected_nodes.size());
	for (const auto & node : collected_nodes) {
		node->_bind();
		const RID node_rid = node->get_self();
		_nodes.insert(node_rid, node, false);
		const IPipelineNodeStateful *stateful_node = dynamic_cast<const IPipelineNodeStateful *>(node);
		if (likely(stateful_node == nullptr)) {
			continue;
		}

		stateful_nodes.push_back(stateful_node);
	}

	_states.reserve(stateful_nodes.size());
	for (const auto & node : stateful_nodes) {
		_states.insert(node->state_key(), node->create_state());
	}
}

void Pipeline::unbind() {
	if (unlikely(_root == nullptr || !_is_bound)) {
		return;
	}

	for (const auto &kvp : _states) {
		const IPipelineNodeStateful *stateful_node = dynamic_cast<const IPipelineNodeStateful *>(_nodes[kvp.key]);
		IPipelineNodeState *state = kvp.value;

		ERR_CONTINUE(stateful_node == nullptr || state == nullptr);

		stateful_node->cleanup_state(state);
	}
	_states.clear();

	for (const auto &kvp : _nodes) {
		PipelineNode *node = const_cast<PipelineNode *>(kvp.value);
		node->_unbind();
	}
	_nodes.clear();
}

void Pipeline::register_types() {
	Blackboard::register_type<NodeStateMap*>();
	Blackboard::register_type<Pipeline*>();
}

}
