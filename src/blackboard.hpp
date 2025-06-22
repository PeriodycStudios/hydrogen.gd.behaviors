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
#include <godot_cpp/core/print_string.hpp>
#include <functional>

#include "rid_data.hpp"
#include "variant_type_traits.hpp"
#include "type_name.hpp"

using namespace godot;

namespace hydrogen {

class Blackboard final : public RidData {

	struct EntryBase : RidData {

		virtual ~EntryBase() = default;
		[[nodiscard]] virtual Variant as_variant() const {
			return {};
		}

		virtual bool set_from(const Variant &p_value) { return false; }

		[[nodiscard]] virtual int64_t get_type_key() const = 0;
		[[nodiscard]] virtual Variant::Type get_variant_type() const {
			return Variant::Type::NIL;
		}
	};

	template<typename  T>
	struct EntryData : EntryBase {
		T value;

		[[nodiscard]] int64_t get_type_key() const override {
			return RegisteredTypeInfo<T>::get_info().type_key;
		}

		EntryData() = default;

		typedef T type;
	};

	template<typename T>
	struct EntryVariant final : EntryData<T> {

		[[nodiscard]] Variant as_variant() const override {
			return Variant(this->value);
		}

		bool set_from(const Variant &p_value) override {
			this->value = p_value;
			return true;
		}

		EntryVariant() = default;

		[[nodiscard]] Variant::Type get_variant_type() const override {
			return RegisteredTypeInfo<T>::get_info().variant_type;
		}
	};

	typedef std::function<EntryBase*(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase*> &p_entries)> entry_factory;

	struct TypeInfo {

		enum struct Flags : uint8_t {
			NONE = 0,
			IS_REGISTERED = 1 << 0,
			IS_VARIANT_TYPE = 1 << 1,
		};

		entry_factory factory = nullptr;
		std::string name; // any attempt to use String or StringName here crashes the plugin on load!
		Variant::Type variant_type = Variant::Type::NIL;
		int64_t type_key = 0;
		Flags flags = Flags::NONE;

		[[nodiscard]] _FORCE_INLINE_ bool has_flag(const Flags p_flags) const {
			return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(p_flags)) != 0;
		}

		_FORCE_INLINE_ void enable_flag(const Flags p_flags) {
			flags = static_cast<Flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(p_flags));
		}

		[[nodiscard]] _FORCE_INLINE_ bool is_registered() const { return has_flag(Flags::IS_REGISTERED); }
		[[nodiscard]] _FORCE_INLINE_ bool is_variant_type() const { return has_flag(Flags::IS_VARIANT_TYPE); }
	};

	template <typename T>
	class RegisteredTypeInfo {
		static TypeInfo type_info;

	public:

		_FORCE_INLINE_ static void init(const TypeInfo &p_info) {
			if (is_registered()) return;

			type_info = p_info;
		}

		_FORCE_INLINE_ static const TypeInfo &get_info() { return type_info; }
		_FORCE_INLINE_ static const TypeInfo *get_info_ptr() { return &type_info; }
		_FORCE_INLINE_ static bool is_registered() { return type_info.is_registered(); }

		typedef T registered_type;
	};

	static HashMap<int64_t, const TypeInfo*> type_infos;
	static std::array<const TypeInfo*, GDEXTENSION_VARIANT_TYPE_VARIANT_MAX> core_variant_type_infos;
	static const TypeInfo *ref_fallback_type_info;
	static bool core_variants_registered;

	RID_PtrOwner<EntryBase> entries_owner;
	HashMap<StringName, EntryBase*> entries;
	StringName name;
	Blackboard *parent;

	template <typename T>
	static EntryBase* create_entry(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase *> &p_entries) {
		T* entry = memnew(T());
		const RID rid = p_owner.make_rid(entry);
		entry->set_self(rid);

		p_entries[p_name] = entry;
		return entry;
	}

	template <typename T>
	static EntryBase* create_data_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries) {
		typedef typename traits::unadorned_type<T>::type type;
		return create_entry<EntryData<type>>(p_name, p_owner, p_entries);
	}

	template<typename T>
	static EntryBase* create_variant_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries) {
		typedef typename traits::unadorned_type<T>::type type;
		return create_entry<EntryVariant<type>>(p_name, p_owner, p_entries);
	}

	template<typename T, typename U>
	static EntryBase* create_variant_converted_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries) {
		typedef typename traits::unadorned_type<T>::type type;
		static_assert(traits::is_variant_type<U>::value, "Convertable type must be a variant type");
		return create_entry<EntryVariantConvertable<type, U>>(p_name, p_owner, p_entries);
	}

	bool validate_parent(const Blackboard *p_parent) const;
	void free_entry(HashMap<StringName, EntryBase *>::Iterator &iter);

	template<typename T>
	bool find_entry(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, bool p_check_parents) const;

public:

	template<typename T, typename U>
	struct EntryVariantConvertable final : EntryData<T> {

		[[nodiscard]] Variant as_variant() const override;

		bool set_from(const Variant &p_value) override;

		EntryVariantConvertable() = default;

		[[nodiscard]] Variant::Type get_variant_type() const override {
			return RegisteredTypeInfo<U>::get_info().variant_type;
		}
	};

	static void register_core_variant_types();

	template <typename T>
	static void register_type() {
		typedef typename traits::unadorned_type<T>::type type;
		if (RegisteredTypeInfo<type>::is_registered()) {
			return;
		}

		const std::string type_name_str(type_name<type>());
		const String name_str = String(type_name_str.c_str());

		print_line("Registering type with blackboard ", name_str);

		TypeInfo type_info = {};
		type_info.type_key = name_str.hash();
		type_info.name = type_name_str;
		if constexpr (traits::is_variant_type<type>::value) {

			type_info.factory = create_variant_entry<type>;
			type_info.variant_type = static_cast<Variant::Type>(GetTypeInfo<type>::VARIANT_TYPE);
			type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);

			print_line("* Registering variant type: ", type_info.variant_type);
		}
		else {
			type_info.factory = create_data_entry<type>;
		}

		type_info.enable_flag(TypeInfo::Flags::IS_REGISTERED);

		RegisteredTypeInfo<type>::init(type_info);
		type_infos[type_info.type_key] = RegisteredTypeInfo<type>::get_info_ptr();

		print_line("* Type fully registered with key: ", type_info.type_key);
	}

	template <typename T, typename U>
	static void register_convertable_type() {
		static_assert(!std::is_same_v<T, Variant>, "Cannot register Variant as a convertable type.");
		static_assert(!std::is_same_v<U, Variant>, "Cannot register Variant as a conversion type.");
		static_assert(traits::is_variant_type<U>::value, "Conversion type MUST be directly compatible with Variant");

		typedef typename traits::unadorned_type<T>::type convertable_type;
		typedef typename traits::unadorned_type<U>::type conversion_type;
		if (RegisteredTypeInfo<convertable_type>::is_registered()) {
			return;
		}

		const std::string type_name_str(type_name<convertable_type>());
		const auto name_str = String(type_name_str.c_str());

		print_line( "Registering convertable type with Blackboard: ", name_str);

		TypeInfo type_info = {};
		type_info.type_key = name_str.hash();
		type_info.name = type_name_str;
		type_info.factory = create_variant_converted_entry<convertable_type, conversion_type>;
		type_info.variant_type = static_cast<Variant::Type>(GetTypeInfo<conversion_type>::VARIANT_TYPE);
		type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);
		type_info.enable_flag(TypeInfo::Flags::IS_REGISTERED);

		print_line("* Registering with conversion type: ", type_info.variant_type);

		RegisteredTypeInfo<convertable_type>::init(type_info);
		type_infos[type_info.type_key] = RegisteredTypeInfo<convertable_type>::get_info_ptr();


		print_line("* Type fully registered with key: ", type_info.type_key);
	}

	explicit Blackboard(const StringName &p_name) : name(p_name), parent(nullptr) {}
	~Blackboard();

	_FORCE_INLINE_ void set_name(const StringName &p_name) { name = p_name; }
	[[nodiscard]] _FORCE_INLINE_ const StringName &get_name() const { return name; }

	_FORCE_INLINE_ bool set_parent(Blackboard *p_parent) {
		if (validate_parent(p_parent)) {
			parent = p_parent;
			return true;
		}
		return false;
	}

	[[nodiscard]] _FORCE_INLINE_ Blackboard *get_parent() const { return parent; }

	[[nodiscard]] Blackboard *find_parent(const StringName &p_name) const;
	[[nodiscard]] Blackboard *find_parent(const RID &p_rid) const;

	template <typename T>
	bool try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents = true) const;

	template <typename T>
	[[nodiscard]] T get_entry(const StringName &p_name, const T &p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	void set_entry(const StringName &p_name, const T &p_value);

	bool erase_entry(const StringName &p_name);

	[[nodiscard]] bool has_entry(const StringName &p_name, bool p_check_parents = true) const;
};


template <>
inline bool Blackboard::find_entry<Variant>(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, bool p_check_parents) const {
	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		p_out_result = iter;
		return true;
	}

	if (likely(p_check_parents)) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			iter = current->entries.find(p_name);
			if (iter != current->entries.end()) {
				p_out_result = iter;
				return true;
			}

			current = current->parent;
		}
	}

	return false;
}

template <typename T>
bool Blackboard::find_entry(const StringName &p_name, HashMap<StringName, EntryBase*>::ConstIterator &p_out_result, const bool p_check_parents) const {
	typedef typename traits::unadorned_type<T>::type type;
	const TypeInfo &type_info = RegisteredTypeInfo<type>::get_info();
	if (unlikely(!type_info.is_registered())) {
		return false;
	}

	const auto type_key = type_info.type_key;

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		p_out_result = iter;
		return true;
	}

	if (likely(p_check_parents)) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			iter = current->entries.find(p_name);
			if (iter != current->entries.end()) {
				const auto entry_type_key = iter->value->get_type_key();

				if (entry_type_key != type_key) {
					return false;
				}

				p_out_result = iter;
				return true;
			}

			current = current->parent;
		}
	}

	return false;
}

template <>
inline bool Blackboard::try_get_entry<Variant>(const StringName &p_name, Variant &p_out_result, bool p_check_parents) const {

	HashMap<StringName, EntryBase *>::ConstIterator iter;
	if (unlikely(!find_entry<Variant>(p_name, iter, p_check_parents))) {
		return false;
	}

	const EntryBase *result = iter->value;
	p_out_result = result->as_variant();
	return true;
}

template <typename T>
bool Blackboard::try_get_entry(const StringName &p_name, T &p_out_result, const bool p_check_parents) const {
	HashMap<StringName, EntryBase *>::ConstIterator iter;
	if (unlikely(!find_entry<T>(p_name, iter, p_check_parents))) {
		return false;
	}

	typedef typename traits::unadorned_type<T>::type type;
	auto *result = dynamic_cast<EntryData<type> *>(iter->value);
	p_out_result = result->value;
	return true;
}

template <>
inline Variant Blackboard::get_entry(const StringName &p_name, const Variant &p_default, bool p_check_parents ) const {
	Variant result = p_default;
	try_get_entry<Variant>(p_name, result, p_check_parents);
	return result;
}

template <typename T>
T Blackboard::get_entry(const StringName &p_name, const T &p_default, const bool p_check_parents) const {
	T result = p_default;
	try_get_entry<T>(p_name, result, p_check_parents);
	return result;
}

template <>
inline void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value) {

	const auto variant_type =p_value.get_type();

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		EntryBase *existing_entry = iter->value;
		if (likely(existing_entry->get_variant_type() == variant_type)) {
			existing_entry->set_from(p_value);
			return;
		}

		free_entry(iter);
	}


	// NIL will delete an existing entry or do nothing if one doesn't exist.
	if (unlikely(variant_type == Variant::Type::NIL)) {
		return;
	}

	const TypeInfo *type_info;
	if (unlikely(variant_type == Variant::Type::OBJECT)) {
		const Ref<RefCounted> ref = p_value;
		type_info = ref.is_null() ?
			core_variant_type_infos[Variant::Type::OBJECT] :
			ref_fallback_type_info;
	}
	else {
		type_info = core_variant_type_infos[variant_type];
	}

	EntryBase *entry = type_info->factory(p_name, entries_owner, entries);
	entry->set_from(p_value);
}

template <typename T>
void Blackboard::set_entry(const StringName &p_name, const T &p_value)  {

	typedef typename traits::unadorned_type<T>::type type;
	if (unlikely(!RegisteredTypeInfo<type>::is_registered())) {
		register_type<type>();
	}

	const TypeInfo &type_info = RegisteredTypeInfo<type>::get_info();
	const int64_t type_key = type_info.type_key;

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		EntryData<type>* existing_entry = dynamic_cast<EntryData<type>*>(iter->value);
		if (likely(existing_entry->get_type_key() == type_key)) {
			existing_entry->value = p_value;
			return;
		}
		free_entry(iter);
	}

	EntryData<type> *new_entry = dynamic_cast<EntryData<type>*>(type_info.factory(p_name, entries_owner, entries));
	new_entry->value = p_value;
}


template <>
_FORCE_INLINE_ void Blackboard::register_type<Variant>() {}

template <typename T>
Blackboard::TypeInfo Blackboard::RegisteredTypeInfo<T>::type_info = TypeInfo();

template <>
inline Variant Blackboard::EntryVariantConvertable<char16_t, int64_t>::as_variant() const {
	int64_t conversion = this->value;
	return {conversion};
}

template <>
inline bool Blackboard::EntryVariantConvertable<char16_t, int64_t>::set_from(const Variant &p_value) {
	const int64_t conversion = p_value;
	this->value = static_cast<char16_t>(conversion);
	return true;
}

template <>
inline Variant Blackboard::EntryVariantConvertable<char32_t, int64_t>::as_variant() const {
	int64_t conversion = this->value;
	return {conversion};
}

template <>
inline bool Blackboard::EntryVariantConvertable<char32_t, int64_t>::set_from(const Variant &p_value) {
	const int64_t conversion = p_value;
	this->value = static_cast<char32_t>(conversion);
	return true;
}

template <>
inline Variant Blackboard::EntryVariantConvertable<ObjectID, int64_t>::as_variant() const {
	const int64_t conversion = this->value;
	return {conversion};
}

template <>
inline bool Blackboard::EntryVariantConvertable<ObjectID, int64_t>::set_from(const Variant &p_value) {
	const int64_t conversion = p_value;
	this->value = conversion;
	return true;
}


template <typename T, typename U>
Variant Blackboard::EntryVariantConvertable<T, U>::as_variant() const {
	U conversion = this->value;
	return Variant(conversion);
}

template <typename T, typename U>
bool Blackboard::EntryVariantConvertable<T, U>::set_from(const Variant &p_value) {
	U conversion = p_value;
	this->value = conversion;
	return true;
}


} //namespace hydrogen

#endif //BLACKBOARD_HPP
