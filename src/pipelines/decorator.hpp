#ifndef NODE_WRAPPER_HPP
#define NODE_WRAPPER_HPP

#include "godot_cpp/core/memory.hpp"
#include "mutex_helpers.hpp"
#include "node_interfaces.hpp"
#include <godot_cpp/core/defs.hpp>
#include <mutex>
#include <type_traits>

namespace hydrogen::pipelines {

template <typename T, typename = void>
class PipelineNodeDecorator {};

template <typename T>
class PipelineNodeDecorator<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeDecorator {
protected:
	T* _decorated_node = nullptr;
	std::mutex *_mutex = nullptr;

	PipelineNodeDecorator() : _mutex(memnew(std::mutex)) {}

public:

	~PipelineNodeDecorator() override {
		memdelete(_mutex);
		_mutex = nullptr;
		_decorated_node = nullptr;
	}

	[[nodiscard]] IPipelineNode *get_node() const override { 
		LOCK_ONE_V(_mutex, nullptr);
		return _decorated_node;
	}
	
	void set_node(const IPipelineNode *p_node) override {
		LOCK_ONE(_mutex);
		_decorated_node = p_node;
	}
};
}

#endif