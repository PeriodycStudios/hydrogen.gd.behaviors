//
// Created by tkey on 4/2/25.
//

#ifndef BLACKBOARD_HPP
#define BLACKBOARD_HPP

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

	const StringName &name;

	struct EntryBase : RidData {

		virtual ~EntryBase() = default;
		virtual Variant as_variant() const = 0;

		virtual const StringName &get_type_key() const;
		virtual const Variant::Type get_variant_type() const;
		virtual void set_from(const Variant &p_value);
	};

	template<typename T>
	struct Entry final : EntryBase {
		T value;
		Variant as_variant() const override {
			return Variant(value);
		}

		 const StringName &get_type_key() const override {
			return BlackboardStorageType<T>::get_type_key();
		}

		void set_from(const Variant &p_value) override {
			value = p_value;
		}

		explicit Entry(const T &p_value) : value(p_value) {}
		explicit Entry(const Variant &p_value) : value(p_value) {}
	};

	typedef std::function<EntryBase*(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase*> &p_entries)> entry_factory;

	static HashMap<StringName, entry_factory> factories;

	RID_PtrOwner<EntryBase> entries_owner;
	HashMap<StringName, EntryBase*> entries;

	Blackboard *parent;

	template<typename T>
	static EntryBase* create_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries);

	bool validate_parent(const Blackboard *p_parent) const;
	void free_entry(HashMap<StringName, EntryBase *>::Iterator &iter);

	template<typename T>
	bool find_entry(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, bool p_check_parents) const;

public:

	template<typename T, typename = void>
	static void register_storage_type() {}

	template <typename T, typename EnableIf<BlackboardStorageType<T>::value, T>::type>
	static void register_storage_type();

	explicit Blackboard(const StringName &p_name) : name(p_name), parent(nullptr) {}
	~Blackboard() = default;

	_FORCE_INLINE_ const StringName &get_name() const { return name; }

	bool set_parent(Blackboard *p_parent) {
		if (validate_parent(p_parent)) {
			parent = p_parent;
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ Blackboard *get_parent() const { return parent; }

	Blackboard *get_parent(const StringName &p_name) const;
	Blackboard *get_parent(const RID &p_rid) const;

	template <typename T>
	bool try_get_entry(const StringName &p_name, typename EnableIf<BlackboardStorageType<T>::value, T>::type &p_out_result, bool p_check_parents = true) const;

	template <typename T>
	T get_entry(const StringName &p_name, const typename EnableIf<BlackboardStorageType<T>::value, T>::type &p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	void set_entry(const StringName &p_name, const typename EnableIf<BlackboardStorageType<T>::value, T>::type &p_value);

	bool erase_entry(const StringName &p_name);

	bool has_entry(const StringName &p_name, bool p_check_parents = true) const;
};

template <>
bool Blackboard::find_entry<Variant>(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, bool p_check_parents) const;

template <>
bool Blackboard::try_get_entry<Variant>(const StringName &p_name, Variant &p_out_result, bool p_check_parents) const;

template <>
Variant Blackboard::get_entry(const StringName &p_name, const Variant &p_default, bool p_check_parents) const;

template <>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value);


} //namespace hydrogen

#endif //BLACKBOARD_H
