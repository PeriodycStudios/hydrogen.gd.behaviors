#ifndef NODE_CONTAINER_HPP
#define NODE_CONTAINER_HPP

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "node_interfaces.hpp"
#include "decorator.hpp"

#include <cstdint>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/core/defs.hpp>
#include <type_traits>

namespace hydrogen::pipelines {

using namespace godot;

template <typename T, typename = void>
class PipelineNodeComposite {};

#define TRY_CONVERT_CHILD()								\
	const T* node = dynamic_cast<const T *>(p_node);	\
	ERR_FAIL_COND(node == nullptr);						\

#define TRY_CONVERT_CHILD_RESULT(result)				\
	const T *node = dynamic_cast<const T *>(p_node);	\
	ERR_FAIL_COND_V(node == nullptr, result);			\
	

template <typename T>
class PipelineNodeComposite<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeComposite {
	Vector<const T*> _children = {};

protected:
	PipelineNodeComposite() = default;

public:
	~PipelineNodeComposite() override { _children.clear(); }

	void add_child_node(const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD()
		add_child(node);
	}

	_FORCE_INLINE_ void add_child(const T *p_node) { _children.push_back(p_node); }

	bool remove_child_node(const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD_RESULT(false)
		return remove_child(node);
	}

	_FORCE_INLINE_ bool remove_child(T *p_node) {
		auto iter = _children.find(p_node);
		if (likely(iter != _children.end())) {
			return _children.erase(iter);
		}
		return false;
	}

	bool remove_child_node_at(int64_t p_index) override {
		return remove_child_at(p_index);
	}

	_FORCE_INLINE_ bool remove_child_at(int64_t p_index) { return _children.remove_at(p_index); }


	void clear() override { _children.clear(); }
	bool is_empty() const override { return _children.is_empty(); }

	const IPipelineNode *get_child_node(int64_t p_index) const override { return get_child(p_index); }
	void set_child_node(int64_t p_index, const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD()
		set_child(p_index, node);
	}

	_FORCE_INLINE_ T *get_child(int64_t p_index) const { return _children.get(p_index); }
	_FORCE_INLINE_ void set_child(int64_t p_index, const T *&p_elem) { _children.set(p_index, p_elem); }

	int64_t get_node_count() const override { return _children.size(); }
	void resize(uint64_t p_size) override { _children.resize(p_size); }
	void resize_zeroed(uint64_t p_size) override { _children.resize_zeroed(p_size); }

	void swap_child_nodes(uint64_t p_first_index, uint64_t p_second_index) override {
		T *p = _children.ptrw();
		SWAP(p[p_first_index], p[p_second_index]);
	}

	Error insert_child_node(int64_t p_pos, const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD_RESULT(Error::ERR_INVALID_PARAMETER)
		return insert_child(p_pos, p_node);
	}

	_FORCE_INLINE_ Error insert_child(int64_t p_pos, const T* p_val) { return _children.insert(p_pos, p_val); }

	void append_child_nodes(const Vector<const IPipelineNode *> &p_nodes) override {
		Vector<const T *> nodes = {};
		uint64_t actual_size = 0;
		for(int64_t i = 0; i < p_nodes.size(); ++i) {
			const T *node = dynamic_cast<const T *>(p_nodes[i]);
			ERR_CONTINUE(node == nullptr);
			nodes.push_back(node);
		}

		append_children(nodes);
	}

	_FORCE_INLINE_ void append_children(const Vector<const T*> &p_to_append) { _children.append(p_to_append); }

	[[nodiscard]] bool is_descendent_node(const IPipelineNode * p_node) override {
		TRY_CONVERT_CHILD_RESULT(false)
		return is_descendent(node);
	}

	[[nodiscard]] bool is_descendent(const T *p_node) const {
		for (auto it = _children.begin(); it != _children.end(); ++it) {
			const T *candidate = *it;
			if (unlikely(p_node == candidate)) {
				return true;
			}
			const PipelineNodeComposite<T> *container = dynamic_cast<const PipelineNodeComposite<T>*>(candidate);
			if (unlikely(container != nullptr && container->is_descendent(p_node))) {
				return true;
			}

			const PipelineNodeDecorator<T> wrapper = dynamic_cast<const PipelineNodeDecorator<T>*>(candidate);
			if (unlikely(wrapper != nullptr && wrapper.get_node() == p_node)) {
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool is_child_node(const IPipelineNode *p_node) override {
		TRY_CONVERT_CHILD_RESULT(false)
		return is_child(node);
	}

	[[nodiscard]] _FORCE_INLINE_ bool is_child(const T *p_node) const { return _children.find(p_node) != _children.end(); }

	T *get_descendant(RID p_rid) {
		ERR_FAIL_COND_V(p_rid == RID(), nullptr);

		for (auto it = _children.begin(); it != _children.end(); ++it) {
			const IPipelineNode *candidate = dynamic_cast<const IPipelineNode*>(*it);
			if (unlikely(candidate->get_self() == p_rid)) {
				return candidate;
			}
			const PipelineNodeComposite *container = dynamic_cast<const PipelineNodeComposite*>(candidate);
			if (unlikely(container != nullptr)) {
				T* descendent_candidate = container->get_descendant(p_rid);
				if (unlikely(descendent_candidate != nullptr)) {
					return descendent_candidate;
				}
			}

			const PipelineNodeDecorator<T> wrapper = dynamic_cast<const PipelineNodeDecorator<T>*>(candidate);
			if (unlikely(wrapper != nullptr && wrapper.get_node() != nullptr && wrapper.get_node()->get_rid() == p_rid)) {
				return wrapper.get_node();
			}
		}

		return nullptr;
	}
};

#undef TRY_CONVERT_CHILD
#undef TRY_CONVERT_CHILD_RESULT

}


#endif
