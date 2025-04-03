//
// Created by tkey on 4/2/25.
//

#ifndef BEHAVIOR_AGENCY_HPP
#define BEHAVIOR_AGENCY_HPP

#include <godot_cpp/core/object.hpp>

using namespace godot;

class HydrogenBehaviorServer : public Object {
    GDCLASS(HydrogenBehaviorServer, Object);

    static HydrogenBehaviorServer *singleton;

protected:
    static void _bind_methods() {}

public:
    static HydrogenBehaviorServer *get_singleton() { return singleton; };

    HydrogenBehaviorServer(){
		singleton = this;
    };

    ~HydrogenBehaviorServer() {
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

class _HydrogenBehaviorServer : public Object {
	GDCLASS(_HydrogenBehaviorServer, Object);
};

#endif