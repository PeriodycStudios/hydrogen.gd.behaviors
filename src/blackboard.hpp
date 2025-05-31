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

#include <functional>

using namespace godot;

namespace hydrogen {

class Blackboard final : public RidData {

	struct EntryBase : RidData {

		virtual ~EntryBase() = default;
		virtual Variant as_variant() const = 0;

		virtual Vector2i get_type_key() const;
		virtual void set_from(const Variant &p_value);
	};

	template<typename T, typename = void>
	struct Entry : EntryBase {};

	template<typename T, typename EnableIf<blackboard_storage_type<T>::value>::type>
	struct Entry final : EntryBase {
		T value;
		Variant as_variant() const override {
			return Variant(value);
		}

		Vector2i get_type_key() const override {
			return blackboard_storage_type<T>::get_type_key();
		}

		void set_from(const Variant &p_value) override {
			value = p_value;
		}

		explicit Entry(const typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_value) :
				value(value) {}
		explicit Entry(const Variant &p_value) : value(p_value) {}
	};

	typedef std::function<EntryBase*(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<String, EntryBase*> &p_entries)> entry_factory;

	static HashMap<StringName, entry_factory> factories;

	RID_PtrOwner<EntryBase> entries_owner;
	HashMap<StringName, EntryBase*> entries;

	Blackboard *parent;


	template<typename T>
	static EntryBase* create_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries);

	template<typename T>
	static void register_create();

	bool validate_parent(const Blackboard *p_parent) const;
	void free_entry(const HashMap<StringName, EntryBase *>::Iterator &iter);

public:

	static void init_create_functions();

	Blackboard();
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
	bool get_entry(const StringName &p_name, typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_out_result,  bool p_check_parents = true) const;

	template <typename T>
	void set_entry(const StringName &p_name, const typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_value);

	bool erase_entry(const StringName &p_name);

	bool has_entry(const StringName &p_name, bool p_check_parents = true) const;
};

template <>
bool Blackboard::get_entry<Variant>(const StringName &p_name, Variant &p_out_result, bool p_check_parents) const;

template <>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value);


} //namespace hydrogen

#endif //GAME_AI_BLACKBOARD_H
