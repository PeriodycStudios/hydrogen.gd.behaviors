//
// Created by tkey on 4/2/25.
//

#ifndef BLACKBOARD_HPP
#define BLACKBOARD_HPP

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "hydrogen_rid.hpp"

using namespace godot;

class HydrogenBlackboardEntryTableBase : HydrogenRid {};

template<typename V>
class HydrogenBlackboardEntryTable : HydrogenBlackboardEntryTableBase {
	HashMap<StringName, V> entries;
};

class HydrogenBlackboard : public HydrogenRid {

	RID_PtrOwner<HydrogenBlackboardEntryTableBase> entries_owner;
	HashMap<Variant::Type, HydrogenBlackboardEntryTableBase *> entries;
	HashMap<StringName, HydrogenBlackboardEntryTableBase *> name_to_table;

  public:
	HydrogenBlackboard() = default;
	~HydrogenBlackboard() = default;

	bool clear_entry(const StringName& p_name);
	_FORCE_INLINE_ bool has_entry(const StringName& p_name) const {
		return name_to_table.has(p_name);
	}
};

#endif //BLACKBOARD_H
