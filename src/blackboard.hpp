//
// Created by tkey on 4/2/25.
//

#ifndef BLACKBOARD_HPP
#define BLACKBOARD_HPP

#include "hydrogen_rid.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace Hydrogen {

class BlackboardEntryTableBase : HydrogenRid {
public:
	virtual ~BlackboardEntryTableBase() {}
	virtual Variant get_variant(const StringName &p_name) const = 0;
};

template <typename V>
class BlackboardEntryTable final : BlackboardEntryTableBase {
	HashMap<StringName, V> entries;

	virtual Variant get_variant(const StringName &p_name) const;
};

template <typename T, typename = void>
struct is_variant_type : std::false_type {};

template<typename T>
struct is_variant_type<T, std::void_t<decltype(GetTypeInfo<T>::get_class_info)>> : std::true_type {};

class Blackboard final : public HydrogenRid {
	RID_PtrOwner<BlackboardEntryTableBase> entries_owner;
	HashMap<Vector2i, BlackboardEntryTableBase *> entries;
	HashMap<StringName, BlackboardEntryTableBase *> name_to_table;
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
	typename EnableIf<is_variant_type<T>::value, T>::type get_entry(const StringName &p_name) const;

	template <typename T, EnableIf<is_variant_type<T>::value, T> = true>
	void set_entry(const StringName &p_name, const T &p_value);

	Variant get_entry(const StringName &p_name) const;

	void set_entry(const StringName &p_name, const Variant &p_value);

	bool erase_entry(const StringName &p_name);

	bool has_entry(const StringName &p_name, bool check_parents = true) const;
};



} //namespace Hydrogen

#endif //BLACKBOARD_H
