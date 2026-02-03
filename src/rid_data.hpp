//
// Created by tkey on 5/6/25.
//

#pragma once

#include <godot_cpp/variant/rid.hpp>

using namespace godot;

class RidData {
	RID self;

	public:
		_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
		_FORCE_INLINE_ RID get_self() const { return self; }
};
