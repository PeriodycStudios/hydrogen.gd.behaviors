#ifndef NODE_CONTAINER_HPP
#define NODE_CONTAINER_HPP

#include "godot_cpp/core/memory.hpp"
#include "node_interfaces.hpp"
#include "../mutex_helpers.hpp"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/core/defs.hpp>

#include <cstdint>
#include <mutex>
#include <type_traits>

namespace hydrogen::pipelines {

using namespace godot;

template <typename T, typename = void>
class PipelineNodeComposite {};

#define TRY_CONVERT_CHILD()								\
	const T* node = dynamic_cast<const T *>(p_node);	\
	ERR_FAIL_COND(node == nullptr)						\

#define TRY_CONVERT_CHILD_V(fail_result)				\
	const T *node = dynamic_cast<const T *>(p_node);	\
	ERR_FAIL_COND_V(node == nullptr, fail_result)		\
	
template <typename T>
class PipelineNodeComposite<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeComposite {
protected:

	Vector<const T*> _children = {};
	std::mutex *_mutex = nullptr;

	PipelineNodeComposite() : _mutex(memnew(std::mutex)) {}

public:
	~PipelineNodeComposite() override { 
		memdelete(_mutex);
		_mutex = nullptr;
		_children.clear();
	}

	void add_child_node(const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD();
		add_child(node);
	}

	_FORCE_INLINE_ void add_child(const T *p_node) { 
		LOCK_ONE(_mutex);
		if (unlikely(_children.has(p_node))) {
			return;	
		}
		_children.push_back(p_node); 
	}

	bool remove_child_node(const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD_V(false);
		return remove_child(node);
	}

	_FORCE_INLINE_ bool remove_child(T *p_node) {
		LOCK_ONE_V(_mutex, false);
		auto iter = _children.find(p_node);
		if (likely(iter != _children.end())) {
			return _children.erase(iter);
		}
		return false;
	}

	bool remove_child_node_at(int64_t p_index) override { return remove_child_at(p_index); }

	_FORCE_INLINE_ bool remove_child_at(int64_t p_index) {
		LOCK_ONE_V(_mutex, false);
		return _children.remove_at(p_index);
	}

	void clear() override { 
		LOCK_ONE(_mutex);
		_children.clear();
	}
	bool is_empty() const override { 
		LOCK_ONE_V(_mutex, false);
		return _children.is_empty(); 
	}

	const IPipelineNode *get_child_node(int64_t p_index) const override { return get_child(p_index); }
	void set_child_node(int64_t p_index, const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD();
		set_child(p_index, node);
	}

	_FORCE_INLINE_ T *get_child(int64_t p_index) const {
		LOCK_ONE_V(_mutex, nullptr);
		return _children.get(p_index);
	}
	_FORCE_INLINE_ void set_child(int64_t p_index, const T *&p_elem) {
		LOCK_ONE(_mutex);
		_children.set(p_index, p_elem);
	}

	int64_t get_node_count() const override {
		LOCK_ONE_V(_mutex, 0);
		return _children.size();
	}

	void resize(uint64_t p_size) override {
		LOCK_ONE(_mutex);
		_children.resize(p_size);
	}
	void resize_zeroed(uint64_t p_size) override {
		LOCK_ONE(_mutex);
		_children.resize_zeroed(p_size);
	}

	void swap_child_nodes(uint64_t p_first_index, uint64_t p_second_index) override {
		LOCK_ONE(_mutex);
		const uint64_t child_count = _children.size();
		ERR_FAIL_INDEX(p_first_index, child_count);
		ERR_FAIL_INDEX(p_second_index, child_count);

		T *p = _children.ptrw();
		SWAP(p[p_first_index], p[p_second_index]);
	}

	Error insert_child_node(int64_t p_pos, const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD_V(Error::ERR_INVALID_PARAMETER);
		return insert_child(p_pos, p_node);
	}

	_FORCE_INLINE_ Error insert_child(int64_t p_pos, const T* p_val) {
		LOCK_ONE_V(_mutex, Error::ERR_INVALID_PARAMETER);
		return _children.insert(p_pos, p_val);
	}

	void append_child_nodes(const Vector<const IPipelineNode *> &p_nodes) override {
		LOCK_ONE(_mutex);

		Vector<const T *> nodes = {};
		for(const IPipelineNode *node : p_nodes) {
			const T *typed_node = dynamic_cast<const T *>(node);
			ERR_CONTINUE(typed_node == nullptr);
			_children.append(typed_node);
		}
	}

	_FORCE_INLINE_ void append_children(const Vector<const T*> &p_to_append) {
		LOCK_ONE(_mutex);
		_children.append_array(p_to_append);
	}
};

#undef TRY_CONVERT_CHILD
#undef TRY_CONVERT_CHILD_V

}


#endif
