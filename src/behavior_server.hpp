//
// Created by tkey on 4/2/25.
//

#ifndef GAME_AI_SERVER_HPP
#define GAME_AI_SERVER_HPP

#include <godot_cpp/core/object.hpp>

using namespace godot;

namespace hydrogen {

class BehaviorServer : public Object {
	GDCLASS(BehaviorServer, Object);

	static BehaviorServer *singleton;

protected:
	static void _bind_methods() {}

public:
	static BehaviorServer *get_singleton() { return singleton; };

	BehaviorServer() {
		singleton = this;
	};

	~BehaviorServer() override {
		singleton = nullptr;
	};

	void free(RID p_object) {};

	void set_active(bool p_active) {};

	RID blackboard_create() { return {}; };
	RID behavior_tree_create() { return {}; };
	RID state_machine_create() { return {}; };
	RID agent_create() { return {}; };

	void init() {};
	void finish() {};
};

}

class _BehaviorServer : public Object {
	GDCLASS(_BehaviorServer, Object);

	static _BehaviorServer *singleton;

protected:
	static void _bind_methods() {}

public:
	static _BehaviorServer *get_singleton() { return singleton; };

	_BehaviorServer() {
		singleton = this;
	};

	~_BehaviorServer() override {
		singleton = nullptr;
	};
};

#endif