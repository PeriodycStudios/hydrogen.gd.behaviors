//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "blackboard.hpp"
#include "pipeline_nodes.hpp"
#include "rid_data.hpp"

#include <cstdint>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string_name.hpp>

#define MAKE_BLACKBOARD_ENTRY_NAME(entry_name)			\
	static const StringName &entry_name##_name() {		\
		const static StringName name(#entry_name, true);\
		return name;									\
	}

namespace hydrogen::pipelines {

using namespace godot;

MAKE_BLACKBOARD_ENTRY_NAME(error)

class IPipelineNode;

struct NodePortInfo {
	const StringName name;
	const StringName type_name;

private:
	friend class IPipelineNode;

	NodePortInfo(const StringName &p_name, const StringName &p_type_name) : name(p_name), type_name(p_type_name) {}
};

class IPipelineNode : public RidData {
protected:
	IPipelineNode() = default;

public:
	virtual ~IPipelineNode() = default;

	virtual int32_t get_input_port_count() const = 0;
	virtual int32_t get_output_port_count() const = 0;

	virtual StringName get_input_port_type_name(int32_t p_port) const = 0;
	virtual StringName get_output_port_type_name(int32_t p_port) const = 0;

	virtual StringName get_input_port_name(int32_t p_port) const = 0;
	virtual StringName get_output_port_name(int32_t p_port) const = 0;

	virtual void get_input_port_info(Vector<NodePortInfo> &p_infos) const = 0;
	virtual void get_output_port_info(Vector<NodePortInfo> &p_infos) const = 0;
};

struct IPipelineNodeState;

struct IPipelineNodeStateful {
	virtual ~IPipelineNodeStateful() = default;

	[[nodiscard]] virtual RID state_key() const = 0;

	[[nodiscard]] virtual IPipelineNodeState *create_state() const = 0;

protected:
	IPipelineNodeStateful() = default;
};

struct IPipelineNodeState {
	virtual ~IPipelineNodeState() = default;

protected:
	IPipelineNodeState() = default;
};

typedef HashMap<RID, IPipelineNodeState *> NodeStateMap;

class IPipelineNodeContainer : public IPipelineNode {
protected:
	IPipelineNodeContainer() = default;
public:
	~IPipelineNodeContainer() override = default;
	virtual int64_t get_node_count() const = 0;
	virtual IPipelineNode *get_node(int64_t p_index) const = 0;
};

class IPipelineNodeWrapper : public IPipelineNode {
protected:
	IPipelineNodeWrapper() = default;
public:
	~IPipelineNodeWrapper() override = default;
	virtual IPipelineNode *get_pipeline_node() const = 0;
};

template <typename T, typename = void>
class ChildNodeWrapper {};

template <typename T>
class ChildNodeWrapper<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeWrapper {
	T* _wrapped_node = nullptr;

protected:
	T* get_child() { return _wrapped_node; }
	ChildNodeWrapper() = default;

public:

	[[nodiscard]] IPipelineNode *get_pipeline_node() const override { return _wrapped_node; }
	[[nodiscard]] _FORCE_INLINE_ T *get_child() const { return _wrapped_node; }
	_FORCE_INLINE_ void set_child(T *p_node) { _wrapped_node = p_node; }

	~ChildNodeWrapper() override = default;
};

template <typename T, typename = void>
class ChildNodeContainer {};

template <typename T>
class ChildNodeContainer<T, std::enable_if_t<std::is_base_of_v<IPipelineNode, T>>> : public IPipelineNodeContainer {
	Vector<T*> _children = {};

protected:

	const Vector<T*> &get_children() const { return _children; }

	ChildNodeContainer() = default;

public:
	~ChildNodeContainer() override { _children.clear(); }

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
	_FORCE_INLINE_ bool is_empty() const { return _children.is_empty(); }

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
			const ChildNodeContainer<T> *container = dynamic_cast<const ChildNodeContainer<T>*>(candidate);
			if (unlikely(container != nullptr && container->is_descendent(p_node))) {
				return true;
			}

			const ChildNodeWrapper<T> wrapper = dynamic_cast<const ChildNodeWrapper<T>*>(candidate);
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
			const ChildNodeContainer *container = dynamic_cast<const ChildNodeContainer*>(candidate);
			if (unlikely(container != nullptr)) {
				T* descendent_candidate = container->get_descendant(p_rid);
				if (unlikely(descendent_candidate != nullptr)) {
					return descendent_candidate;
				}
			}

			const ChildNodeWrapper<T> wrapper = dynamic_cast<const ChildNodeWrapper<T>*>(candidate);
			if (unlikely(wrapper != nullptr && wrapper.get_node() != nullptr && wrapper.get_node()->get_rid() == p_rid)) {
				return wrapper.get_node();
			}
		}

		return nullptr;
	}
};


} // hydrogen

#endif //BEHAVIORS_NODE_BASE_HPP
