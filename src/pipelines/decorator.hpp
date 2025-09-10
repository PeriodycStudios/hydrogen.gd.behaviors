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
	const T* _decorated_node = nullptr;

	PipelineNodeDecorator() = default;

public:

	~PipelineNodeDecorator() override {
		_decorated_node = nullptr;
	}

	bool has_child(const IPipelineNode *p_node) const override {
		const T *node = dynamic_cast<const T *>(p_node);
		if (unlikely(node == nullptr)) {
			return false;
		}

		return _decorated_node != nullptr && _decorated_node == node;
	}

	bool remove_child(const IPipelineNode *p_node) override {
		const T *node = dynamic_cast<const T *>(p_node);
		if (unlikely(node == nullptr)) {
			return false;
		}

		if (likely(node == _decorated_node)) {
			_decorated_node = nullptr;
			return true;
		}
		else {
			return false;
		}
	}

	void remove_all_children() override {
		_decorated_node = nullptr;
	}

	[[nodiscard]] const IPipelineNode *get_child() const override { 
		return _decorated_node;
	}
	
	void set_child(const IPipelineNode *p_node) override {
		const T * node = dynamic_cast<const T *>(p_node);
		_decorated_node = node;
	}
};
}

#endif