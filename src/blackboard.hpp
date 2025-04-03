//
// Created by tkey on 4/2/25.
//

#ifndef BLACKBOARD_HPP
#define BLACKBOARD_HPP

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

struct HydrogenBlackboardEntry {
  RID self;

  public:
	_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }
};

class HydrogenBlackboard {
  RID self;

  public:
	_FORCE_INLINE_ void set_self(const RID &p_self) { self = p_self; }
	_FORCE_INLINE_ RID get_self() const { return self; }
};

// class HydrogenBlackboardPort : public Object {
//   GDCLASS(HydrogenBlackboardPort, Object);
// };
//
// class HydrogenBlackboard : public Resource {
//   GDCLASS(HydrogenBlackboard, Resource);
//
// };


#endif //BLACKBOARD_H
