//
// Created by tkey on 7/31/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP
#include "pipeline_nodes.hpp"
#include <type_traits>

#include <atomic>

namespace hydrogen {
namespace pipelines {

class Pipeline;

class IPipelineGraph {





protected:
	IPipelineGraph() = default;

	// TODO: Port mappings/connections
	// TODO: sub-graph management

public:



	virtual ~IPipelineGraph() = default;

	[[nodiscard]] virtual PipelineNode *get_pipeline_root() const = 0;
	virtual Vector<IPipelineGraph *> get_pipeline_sub_graphs() const = 0;
};

template<typename T, typename = void>
class PipelineGraph {};

template<typename T>
class PipelineGraph<T, std::enable_if_t<std::is_base_of_v<PipelineNode, T>>> : public RidData, public IPipelineGraph {

	friend class Pipeline;

	std::recursive_mutex _mutex = {};
	std::atomic_uint32_t _bind_count = {0};
	T *_root;

	virtual void _bind() {
		++_bind_count;

		if (unlikely(_bind_count == 1)) {
			_root->_bind();
		}
	}

	virtual void _unbind() {

		if (unlikely(_bind_count == 1)) {
			_root->_unbind();
		}

		--_bind_count;
	}

protected:
	explicit PipelineGraph(T *root) : _root{root} {}
public:
	~PipelineGraph() override = default;

	[[nodiscard]] PipelineNode *get_pipeline_root() const override { return _root; }
	[[nodiscard]] _FORCE_INLINE_ T *get_root() const { return _root; }
	virtual Vector<T*> get_sub_graphs() const = 0;

	_FORCE_INLINE_ bool is_bound() const { return _bind_count > 0; }
	void lock() {
		ERR_FAIL_COND(is_bound());

		_mutex.lock();
	}

	void unlock() {
		ERR_FAIL_COND(is_bound());

		_mutex.unlock();
	}
};

} // pipelines
} // hydrogen

#endif //PIPELINE_GRAPH_HPP
