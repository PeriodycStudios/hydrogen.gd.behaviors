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

	void run_tests();
};

}


#endif //BEHAVIOR_SERVER_HPP