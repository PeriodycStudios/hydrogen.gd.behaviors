#ifndef NODE_WRAPPER_HPP
#define NODE_WRAPPER_HPP

#include "node_interfaces.hpp"
#include <godot_cpp/core/defs.hpp>
#include <type_traits>

namespace hydrogen::pipelines {

template <typename T, typename = void>
class PipelineNodeDecorator {};

template <typename T>
class PipelineNodeDecorator<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeDecorator {
	T* _decorated_node = nullptr;

protected:
	T* get_child() { return _decorated_node; }
	PipelineNodeDecorator() = default;

public:

	[[nodiscard]] IPipelineNode *get_pipeline_node() const override { return _decorated_node; }
	[[nodiscard]] _FORCE_INLINE_ T *get_child() const { return _decorated_node; }
	_FORCE_INLINE_ void set_child(T *p_node) { _decorated_node = p_node; }

	~PipelineNodeDecorator() override = default;
};
}

#endif