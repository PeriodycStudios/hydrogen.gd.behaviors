//
// Created by tkey on 5/6/25.
//

#ifndef HYDROGENRID_H
#define HYDROGENRID_H

#include <godot_cpp/variant/rid.hpp>

using namespace godot;

class HydrogenRid {
	RID self;

	public:
		_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
		_FORCE_INLINE_ RID get_self() const { return self; }
};

#endif //HYDROGENRID_H
