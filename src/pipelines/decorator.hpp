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
protected:
	T* _decorated_node = nullptr;

	PipelineNodeDecorator() = default;

public:

	~PipelineNodeDecorator() override {
		_decorated_node = nullptr;
	}

	[[nodiscard]] IPipelineNode *get_child() const override { 
		return _decorated_node;
	}
	
	void set_child(const IPipelineNode *p_node) override {
		_decorated_node = p_node;
	}
};
}

#endif