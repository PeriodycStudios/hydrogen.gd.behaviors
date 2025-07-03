//
// Created by tkey on 4/2/25.
//

#ifndef BEHAVIOR_SERVER_HPP
#define BEHAVIOR_SERVER_HPP

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/rid_owner.hpp>

#include "blackboard.hpp"

using namespace godot;

namespace hydrogen {

class BehaviorServer final : public Object {
	GDCLASS(BehaviorServer, Object);

	static BehaviorServer *singleton;

	RID_PtrOwner<Blackboard> blackboard_owner;

	Mutex *mutex = nullptr;

protected:
	static void _bind_methods();

public:
	static BehaviorServer *get_singleton();

	BehaviorServer();
	~BehaviorServer() override;

	void lock() const;
	void unlock() const;

	void free_rid(RID p_rid);

	RID blackboard_create();
	RID behavior_tree_create();
	RID state_machine_create();
	RID agent_create();

	Error init();
	void finish();

	// ---- Blackboard ----

	bool blackboard_set_parent(RID p_rid, RID p_parent);
	RID blackboard_get_parent(RID p_rid);
	bool blackboard_is_ancestor(RID p_rid, RID p_candidate);

	template<typename T>
	bool blackboard_try_get(RID p_rid, const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	template<typename T>
	T blackboard_get_entry(RID p_rid, const StringName &p_name, const T& p_default = {}, bool p_check_parents = true);

	template<typename T>
	void blackboard_set_entry(RID p_rid, const StringName &p_name, const T &p_value);

	bool blackboard_erase_entry(RID p_rid, const StringName &p_name);

	bool blackboard_has_entry(RID p_rid, const StringName &p_name, bool p_check_parents = true);

	Dictionary blackboard_export_entries(RID p_rid);

	static Array blackboard_export_type_infos();

	// ---- Blackboard END ----
};


class _BehaviorServer final : public Object {
	GDCLASS(_BehaviorServer, Object);

	friend class BehaviorServer;
	static _BehaviorServer *singleton;

protected:
	static void _bind_methods();

public:
	static _BehaviorServer *get_singleton();

	_BehaviorServer() {
		singleton = this;
	};

	~_BehaviorServer() override {
		singleton = nullptr;
	};

	void free_rid(RID p_rid);

	RID blackboard_create();
	RID behavior_tree_create();
	RID state_machine_create();
	RID agent_create();

	// ---- Blackboard ----

	bool blackboard_set_parent(RID p_rid, RID p_parent);
	RID blackboard_get_parent(RID p_rid);
	bool blackboard_is_ancestor(RID p_rid, RID p_candidate);

	template<typename T>
	bool blackboard_try_get(RID p_rid, const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	template<typename T>
	T blackboard_get_entry(RID p_rid, const StringName &p_name, const T& p_default = {}, bool p_check_parents = true);

	Object* blackboard_get_object(RID p_rid, const StringName &p_name, Object* p_default = nullptr, bool p_check_parents = true) {
		return blackboard_get_entry(p_rid, p_name, p_default, p_check_parents);
	}

	template<typename T>
	void blackboard_set_entry(RID p_rid, const StringName &p_name, const T &p_value);

	void blackboard_set_object(RID p_rid, const StringName &p_name, const Object* p_value) {
		blackboard_set_entry(p_rid, p_name, p_value);
	}

	bool blackboard_erase_entry(RID p_rid, const StringName &p_name);

	bool blackboard_has_entry(RID p_rid, const StringName &p_name, bool p_check_parents = true);

	Dictionary blackboard_export_entries(RID p_rid);

	Array blackboard_export_type_infos();

	// ---- Blackboard END ----

	void run_tests();
};

}


#endif //BEHAVIOR_SERVER_HPP