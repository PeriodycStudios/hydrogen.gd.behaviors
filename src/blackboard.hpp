//
// Created by tkey on 4/2/25.
//

#ifndef GAME_AI_BLACKBOARD_HPP
#define GAME_AI_BLACKBOARD_HPP

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "blackboard_storage_type.hpp"
#include "rid_data.hpp"

using namespace godot;

namespace hydrogen {

class Blackboard final : public RidData {

	class EntryTableBase : RidData {
	public:
		virtual ~EntryTableBase() = default;
		virtual Variant get_variant(const StringName &p_name) const = 0;
	};

	template <typename V>
	class EntryTable final : EntryTableBase {
		HashMap<StringName, V> entries;

		Variant get_variant(const StringName &p_name) const override;
	};

	RID_PtrOwner<EntryTableBase> entries_owner;
	HashMap<Vector2i, EntryTableBase *> entries;
	HashMap<StringName, EntryTableBase *> name_to_table;
	Blackboard *parent;

	bool validate_parent(Blackboard *p_parent);

public:
	Blackboard();
	explicit Blackboard(Blackboard *p_parent);
	~Blackboard();

	_FORCE_INLINE_ bool set_parent(Blackboard *p_parent) {
		if (validate_parent(p_parent)) {
			parent = p_parent;
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ Blackboard *get_parent() const { return parent; }

	template <typename T>
	typename EnableIf<blackboard_storage_type<T>::value, T>::type get_entry(const StringName &p_name) const;

	template <typename T>
	void set_entry(const StringName &p_name, const typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_value);

	bool erase_entry(const StringName &p_name);

	bool has_entry(const StringName &p_name, bool check_parents = true) const;
};

template <>
Variant Blackboard::get_entry<Variant>(const StringName &p_name) const;

template <>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value);




} //namespace hydrogen

#endif //GAME_AI_BLACKBOARD_H
