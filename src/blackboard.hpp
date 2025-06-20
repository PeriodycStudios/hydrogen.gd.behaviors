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

#include "rid_data.hpp"

#include <functional>

using namespace godot;

namespace hydrogen {

class Blackboard final : public RidData {

	struct EntryBase : RidData {

		virtual ~EntryBase() = default;
		virtual Variant as_variant() const {
			return {};
		}
		virtual bool set_from(const Variant &p_value) { return false; }

		virtual const StringName &get_type_key() const;
		virtual GDExtensionVariantType get_variant_type() const {
			return GDEXTENSION_VARIANT_TYPE_NIL;
		}
	};

	template<typename  T>
	struct EntryData : EntryBase {
		T value;

		const StringName &get_type_key() const override {
			return RegisteredTypeInfo<T>::get_info().type_key;
		}

		EntryData() = default;
		explicit EntryData(const T &p_value) : value(p_value) {}
	};

	template<typename T>
	struct EntryVariant final : EntryData<T> {

		Variant as_variant() const override;

		bool set_from(const Variant &p_value) override;

		EntryVariant() = default;
		explicit EntryVariant(const T &p_value) : EntryData<T>(p_value) {}
		explicit EntryVariant(const Variant &p_value) : EntryData<T>(p_value) {}

		GDExtensionVariantType get_variant_type() const override {
			return RegisteredTypeInfo<T>::get_info().variant_type;
		}
	};

	typedef std::function<EntryBase*(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase*> &p_entries)> entry_factory;

	struct TypeInfo {

		enum struct Flags : uint8_t {
			NONE = 0,
			IS_REGISTERED = 1 << 0,
			IS_VARIANT_TYPE = 1 << 1,
			IS_GD_OBJECT = 1 << 2,
			IS_GD_REF = 1 << 3,
		};

		PropertyInfo variant_prop_info = {};
		StringName type_key = "";
		entry_factory factory = nullptr;
		GDExtensionVariantType variant_type = GDEXTENSION_VARIANT_TYPE_NIL;
		GDExtensionClassMethodArgumentMetadata variant_argument_metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
		Flags flags = Flags::NONE;

		_FORCE_INLINE_ bool has_flag(const Flags p_flags) const {
			return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(p_flags)) != 0;
		}

		_FORCE_INLINE_ void enable_flag(const Flags p_flags) {
			flags = static_cast<Flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(p_flags));
		}

		_FORCE_INLINE_ bool is_registered() const { return has_flag(Flags::IS_REGISTERED); }
		_FORCE_INLINE_ bool is_variant_type() const { return has_flag(Flags::IS_VARIANT_TYPE); }
		_FORCE_INLINE_ bool is_gd_object() const { return has_flag(Flags::IS_GD_OBJECT); }
		_FORCE_INLINE_ bool is_gd_reference() const { return has_flag(Flags::IS_GD_REF); }
	};

	template <typename T>
	class RegisteredTypeInfo {
		friend class Blackboard;
		static TypeInfo type_info;

	public:

		_FORCE_INLINE_ static void set_info(const TypeInfo &p_info) {
			if (is_registered()) return;

			type_info = p_info;
		}

		_FORCE_INLINE_ static const TypeInfo &get_info() { return type_info; }
		_FORCE_INLINE_ static const TypeInfo *get_info_ptr() { return &type_info; }
		_FORCE_INLINE_ static bool is_registered() { return type_info.is_registered(); }

		typedef T registered_type;
	};

	static HashMap<StringName, const TypeInfo*> type_infos;
	static std::array<const TypeInfo*, GDEXTENSION_VARIANT_TYPE_VARIANT_MAX> core_variant_type_infos;
	static const TypeInfo *ref_fallback_type_info;
	static bool core_variants_registered;

	const StringName &name;
	RID_PtrOwner<EntryBase> entries_owner;
	HashMap<StringName, EntryBase*> entries;

	Blackboard *parent;

	template <typename T>
	static EntryBase* create_data_entry(const StringName &p_name,
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

	static void register_core_variant_types();

	template <typename T>
	static void register_type();

	explicit Blackboard(const StringName &p_name) : name(p_name), parent(nullptr) {}
	~Blackboard() = default;

	_FORCE_INLINE_ const StringName &get_name() const { return name; }

	_FORCE_INLINE_ bool set_parent(Blackboard *p_parent) {
		if (validate_parent(p_parent)) {
			parent = p_parent;
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ Blackboard *get_parent() const { return parent; }

	Blackboard *find_parent(const StringName &p_name) const;
	Blackboard *find_parent(const RID &p_rid) const;

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
_FORCE_INLINE_ void Blackboard::register_type<Variant>() {}

template <typename T>
Blackboard::TypeInfo Blackboard::RegisteredTypeInfo<T>::type_info = TypeInfo();

template <>
Variant Blackboard::EntryVariant<char16_t>::as_variant() const;

template <>
bool Blackboard::EntryVariant<char16_t>::set_from(const Variant &p_value);

template <>
Variant Blackboard::EntryVariant<char32_t>::as_variant() const;

template <>
bool Blackboard::EntryVariant<char32_t>::set_from(const Variant &p_value);


} //namespace hydrogen

#endif //BLACKBOARD_HPP
