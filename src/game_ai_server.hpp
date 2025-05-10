//
// Created by tkey on 4/2/25.
//

#ifndef GAME_AI_SERVER_HPP
#define GAME_AI_SERVER_HPP

#include <godot_cpp/core/object.hpp>

using namespace godot;

namespace hydrogen {

class GameAIServer final : public Object {
	GDCLASS(GameAIServer, Object);

	static GameAIServer *singleton;

protected:
	static void _bind_methods() {}

public:
	static GameAIServer *get_singleton() { return singleton; };

	GameAIServer() {
		singleton = this;
	};

	~GameAIServer() {
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

class _GameAIServer : public Object {
	GDCLASS(_GameAIServer, Object);

	static _GameAIServer *singleton;

protected:
	static void _bind_methods() {}

public:
	static _GameAIServer *get_singleton() { return singleton; };

	_GameAIServer() {
		singleton = this;
	};

	~_GameAIServer() {
		singleton = nullptr;
	};
};

#endif