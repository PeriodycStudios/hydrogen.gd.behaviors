//
// Created by tkey on 5/6/25.
//

#ifndef GAME_AI_RID_H
#define GAME_AI_RID_H

#include <godot_cpp/variant/rid.hpp>

using namespace godot;

class GameAIRid {
	RID self;

	public:
		_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
		_FORCE_INLINE_ RID get_self() const { return self; }
};

#endif //GAME_AI_RID_H
