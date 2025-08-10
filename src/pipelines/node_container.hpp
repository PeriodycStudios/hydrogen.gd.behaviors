#ifndef NODE_CONTAINER_HPP
#define NODE_CONTAINER_HPP

#include "node_interfaces.hpp"
#include "node_wrapper.hpp"

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/core/defs.hpp>
#include <type_traits>

namespace hydrogen::pipelines {

using namespace godot;

template <typename T, typename = void>
class PipelineNodeContainer {};

template <typename T>
class PipelineNodeContainer<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeContainer {
	Vector<T*> _children = {};

protected:

	const Vector<T*> &get_children() const { return _children; }

	PipelineNodeContainer() = default;

public:
	~PipelineNodeContainer() override { _children.clear(); }

	_FORCE_INLINE_ void add_child(T *p_node) { _children.push_back(p_node); }
	_FORCE_INLINE_ bool remove_child(T *p_node) {
		auto iter = _children.find(p_node);
		if (likely(iter != _children.end())) {
			return _children.erase(iter);
		}
		return false;
	}
	_FORCE_INLINE_ bool remove_child_at(int64_t p_index) { return _children.remove_at(p_index); }
	_FORCE_INLINE_ void clear() { _children.clear(); }
	bool is_empty() const override { return _children.is_empty(); }

	IPipelineNode *get_node(int64_t p_index) const override { return _children[p_index]; }
	_FORCE_INLINE_ T *get_child(int64_t p_index) const { return _children.get(p_index); }
	_FORCE_INLINE_ void set_child(int64_t p_index, const T *&p_elem) { _children.set(p_index, p_elem); }

	int64_t get_node_count() const override { return _children.size(); }
	_FORCE_INLINE_ int64_t get_children_size() const { return _children.size(); }
	_FORCE_INLINE_ void resize(uint64_t p_size) { _children.resize(p_size); }
	_FORCE_INLINE_ void resize_zeroed(int64_t p_size) { _children.resize_zeroed(p_size); }


	_FORCE_INLINE_ void swap_children(int64_t p_first_index, int64_t p_second_index) {
		T *p = _children.ptrw();
		SWAP(p[p_first_index], p[p_second_index]);
	}

	_FORCE_INLINE_ Error insert_child(int64_t p_pos, T* p_val) { return _children.insert(p_pos, p_val); }
	_FORCE_INLINE_ void append_children(const Vector<T*> &p_to_append) { _children.append(p_to_append); }


	[[nodiscard]] bool is_descendent(T *p_node) const {
		for (auto it = _children.begin(); it != _children.end(); ++it) {
			const T *candidate = *it;
			if (unlikely(p_node == candidate)) {
				return true;
			}
			const PipelineNodeContainer<T> *container = dynamic_cast<const PipelineNodeContainer<T>*>(candidate);
			if (unlikely(container != nullptr && container->is_descendent(p_node))) {
				return true;
			}

			const PipelineNodeWrapper<T> wrapper = dynamic_cast<const PipelineNodeWrapper<T>*>(candidate);
			if (unlikely(wrapper != nullptr && wrapper.get_node() == p_node)) {
				return true;
			}
		}

		return false;
	}
	[[nodiscard]] _FORCE_INLINE_ bool is_child(T *p_node) const { return _children.find(p_node) != _children.end(); }

	T *get_descendant(RID p_rid) {
		ERR_FAIL_COND_V(p_rid == RID(), nullptr);

		for (auto it = _children.begin(); it != _children.end(); ++it) {
			const IPipelineNode *candidate = dynamic_cast<const IPipelineNode*>(*it);
			if (unlikely(candidate->get_self() == p_rid)) {
				return candidate;
			}
			const PipelineNodeContainer *container = dynamic_cast<const PipelineNodeContainer*>(candidate);
			if (unlikely(container != nullptr)) {
				T* descendent_candidate = container->get_descendant(p_rid);
				if (unlikely(descendent_candidate != nullptr)) {
					return descendent_candidate;
				}
			}

			const PipelineNodeWrapper<T> wrapper = dynamic_cast<const PipelineNodeWrapper<T>*>(candidate);
			if (unlikely(wrapper != nullptr && wrapper.get_node() != nullptr && wrapper.get_node()->get_rid() == p_rid)) {
				return wrapper.get_node();
			}
		}

		return nullptr;
	}
};

}


#endif
