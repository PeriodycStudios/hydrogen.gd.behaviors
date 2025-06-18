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

// #include "blackboard_storage_type.hpp"
#include "rid_data.hpp"
#include "variant_type_traits.hpp"

#include <functional>

using namespace godot;

namespace hydrogen {

class Blackboard final : public RidData {

	static const StringName default_type_key;

	struct EntryBase;

	typedef std::function<EntryBase*(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase*> &p_entries)> entry_factory;

	template <typename T>
	static const StringName &get_type_key();

	template <typename T>
	static const entry_factory &get_entry_factory();

	struct EntryBase : RidData {

		virtual ~EntryBase() = default;
		virtual Variant as_variant() const {
			return {};
		}

		virtual const StringName &get_type_key() const;
		virtual const Variant::Type get_variant_type() const;
		virtual bool set_from(const Variant &p_value) { return false; }
	};

	template<typename  T>
	struct Entry : EntryBase {
		T value;

		const StringName &get_type_key() const override {
			return Blackboard::get_type_key<T>();
		}

		Entry() = default;
		explicit Entry(const T &p_value) : value(p_value) {}
	};

	template<typename T>
	struct EntryVariant final : Entry<T> {

		Variant as_variant() const override {
			return Variant(this->value);
		}

		bool set_from(const Variant &p_value) override {
			this->value = p_value;
			return true;
		}

		explicit EntryVariant(const T &p_value) : Entry<T>(p_value) {}
		explicit EntryVariant(const Variant &p_value) : Entry<T>(p_value) {}
	};

	struct TypeInfo {
		const StringName type_key = "";
		const entry_factory factory = nullptr;
		const GDExtensionVariantType variant_type_id = GDEXTENSION_VARIANT_TYPE_NIL;
		const GDExtensionClassMethodArgumentMetadata variant_argument_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
		const bool is_registered = false;
		const bool is_variant = false;
		const bool is_gd_object = false;
		const bool is_gd_reference = false;

		TypeInfo(
			const StringName &p_type_key,
			const entry_factory &p_factory,
			const GDExtensionVariantType p_variant_type = GDEXTENSION_VARIANT_TYPE_NIL,
			const GDExtensionClassMethodArgumentMetadata p_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE,
			const bool p_is_variant = false,
			const bool p_is_gd_object = false,
			const bool p_is_gd_reference = false
			) :
		type_key(p_type_key), factory(p_factory),
		is_registered(true), is_variant(p_is_variant),
		is_gd_object(p_is_gd_object), is_gd_reference(p_is_gd_reference)
		{}
	};

	template <typename T>
	struct RegisteredTypeInfo {
		static TypeInfo type_info;
		typedef T registered_type;
	};

	const StringName &name;

	static HashMap<StringName, entry_factory> factories;

	RID_PtrOwner<EntryBase> entries_owner;
	HashMap<StringName, EntryBase*> entries;

	Blackboard *parent;

	template <typename T>
	static EntryBase* create_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries);

	template<typename T>
	static EntryBase* create_variant_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries);

	bool validate_parent(const Blackboard *p_parent) const;
	void free_entry(HashMap<StringName, EntryBase *>::Iterator &iter);

	template<typename T>
	bool find_entry(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, bool p_check_parents) const;

public:

	// TODO: look into constexpr/decltype check to separate types
	template <typename T>
	static void register_type();
	//
	//
	// template<typename T, typename = void>
	// static void register_storage_type() {}
	//
	// template <typename T, typename EnableIf<BlackboardStorageType<T>::value, T>::type>
	// static void register_storage_type();

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
	bool try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents = true) const;

	template <typename T>
	T get_entry(const StringName &p_name, const T &p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	void set_entry(const StringName &p_name, const T &p_value);

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

template <>
inline void Blackboard::register_type<Variant>() {}


} //namespace hydrogen

#endif //BLACKBOARD_HPP
