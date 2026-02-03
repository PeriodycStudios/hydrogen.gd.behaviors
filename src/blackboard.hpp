//
// Created by tkey on 4/2/25.
//

#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/mutex.hpp>

#include <functional>

#include "name_helpers.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"
#include "rid_data.hpp"
#include "variant_type_traits.hpp"

using namespace godot;

namespace hydrogen {

namespace traits {

template <typename T>
struct convertible_info {
	typedef false_type needs_conversion;
	// typedef variant_compatible_type conversion_type;
	// static T convert_to(const U x) { return x; }
	// static U convert_from(const T x) { return x; }
};

template <typename T>
constexpr bool convertible_info_needs_conversion_v = convertible_info<T>::needs_conversion::value;

// use this handy macro to easily define convertible type information:
#define MAKE_VARIANT_CONVERTIBLE_TYPE(type, conv_type, to_func, from_func)	\
template <>																	\
struct hydrogen::traits::convertible_info<type> {							\
	typedef std::true_type needs_conversion;								\
	typedef conv_type conversion_type;										\
	_FORCE_INLINE_ static type convert_to(const conv_type x) {	\
		to_func																\
	}																		\
																			\
_FORCE_INLINE_ static conv_type convert_from(const type x) {		\
		from_func															\
	}																		\
};
}

class Blackboard final : public RidData {

#define ENTRY_NAME_BODY(name)					\
	static const StringName entry_name = #name;	\
	return entry_name;

	struct EntryBase : RidData {

		EntryBase() = default;
		virtual ~EntryBase() = default;
		[[nodiscard]] virtual Variant as_variant() const {
			return {};
		}

		virtual void set_from(const Variant &p_value) {}

		[[nodiscard]] virtual int64_t get_type_key() const = 0;
		[[nodiscard]] virtual Variant::Type get_variant_type() const {
			return Variant::Type::NIL;
		}
		[[nodiscard]] virtual int64_t get_object_class_key() const = 0;

		[[nodiscard]] virtual const StringName &get_type_name() const = 0;

		[[nodiscard]] virtual const StringName &get_entry_type_name() const {
			ENTRY_NAME_BODY(EntryBase)
		}
	};

	template<typename  T>
	struct EntryData : EntryBase {
		typedef T type;
		type value;

		[[nodiscard]] int64_t get_type_key() const override {
			return RegisteredTypeInfo<type>::get_info().type_key;
		}

		[[nodiscard]] int64_t get_object_class_key() const override {
			return RegisteredTypeInfo<type>::get_info().object_class_key;
		}

		EntryData() = default;
		~EntryData() override = default;

		[[nodiscard]] const StringName &get_type_name() const override {
			return RegisteredTypeInfo<type>::get_info().get_name();
		}

		[[nodiscard]] const StringName &get_entry_type_name() const override {
			ENTRY_NAME_BODY(EntryData)
		}
	};

	template<typename T>
	struct EntryVariant final : EntryData<T> {

		[[nodiscard]] Variant as_variant() const override {
			return Variant(this->value);
		}

		void set_from(const Variant &p_value) override {
			this->value = p_value;
		}

		EntryVariant() = default;
		~EntryVariant() override = default;

		[[nodiscard]] Variant::Type get_variant_type() const override {
			return RegisteredTypeInfo<T>::get_info().variant_type;
		}

		[[nodiscard]] const StringName &get_entry_type_name() const override {
			ENTRY_NAME_BODY(EntryVariant)
		}
	};

	template <typename T>
	struct EntryVariantObject final : EntryData<T> {
		typedef traits::resolve_object_ptr_type_t<T> convert_type;
		typedef std::remove_pointer_t<T> without_ptr;

		[[nodiscard]] Variant as_variant() const override {
			convert_type conversion = this->value;
			return Variant(conversion);
		}

		void set_from(const Variant &p_value) override {
			convert_type conversion = p_value;
			this->value = Object::cast_to<without_ptr>(conversion);
		}

		EntryVariantObject() = default;
		~EntryVariantObject() override = default;

		[[nodiscard]] Variant::Type get_variant_type() const override {
			return Variant::OBJECT;
		}

		[[nodiscard]] const StringName &get_entry_type_name() const override {
			ENTRY_NAME_BODY(EntryVariantObject)
		}
	};

	template<typename T, typename U>
	struct EntryVariantConvertable final : EntryData<T> {

		[[nodiscard]] Variant as_variant() const override;

		void set_from(const Variant &p_value) override;

		EntryVariantConvertable() = default;
		~EntryVariantConvertable() override = default;

		[[nodiscard]] Variant::Type get_variant_type() const override {
			return RegisteredTypeInfo<U>::get_info().variant_type;
		}

		[[nodiscard]] const StringName &get_entry_type_name() const override {
			ENTRY_NAME_BODY(EntryVariantConvertable)
		}
	};

#undef ENTRY_NAME_BODY

	typedef HashMap<StringName, EntryBase*> EntryMap;
	typedef RID_PtrOwner<EntryBase> EntryOwner;

	typedef std::function<EntryBase*(const StringName &p_name, EntryOwner &p_owner, EntryMap &p_entries, EntryBase* previous)> entry_factory;

	struct TypeInfo {

		static HashMap<int64_t, StringName> type_names;

		enum struct Flags : uint8_t {
			NONE = 0,
			IS_REGISTERED = 1 << 0,
			IS_VARIANT_TYPE = 1 << 1,
			IS_OBJECT_PTR_TYPE = 1 << 2,
			IS_REF_COUNTED = 1 << 3,
			IS_CONVERTIBLE = 1 << 4,

			IS_GD_OBJECT = IS_OBJECT_PTR_TYPE | IS_REF_COUNTED,
		};

		entry_factory create = nullptr;
		Variant::Type variant_type = Variant::Type::NIL;
		int64_t type_key = 0;
		int64_t object_class_key = 0;
		Flags flags = Flags::NONE;

		[[nodiscard]] _FORCE_INLINE_ bool has_flag(const Flags p_flags) const {
			return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(p_flags)) != 0;
		}

		_FORCE_INLINE_ void enable_flag(const Flags p_flags) {
			flags = static_cast<Flags>(static_cast<uint8_t>(flags) | static_cast<uint8_t>(p_flags));
		}

		[[nodiscard]] _FORCE_INLINE_ bool is_registered() const { return has_flag(Flags::IS_REGISTERED); }
		[[nodiscard]] _FORCE_INLINE_ bool is_variant_type() const { return has_flag(Flags::IS_VARIANT_TYPE); }
		[[nodiscard]] _FORCE_INLINE_ bool is_object_ptr_type() const { return has_flag(Flags::IS_OBJECT_PTR_TYPE); }
		[[nodiscard]] _FORCE_INLINE_ bool is_ref_counted() const { return has_flag(Flags::IS_REF_COUNTED); }
		[[nodiscard]] _FORCE_INLINE_ bool is_gd_object() const { return has_flag(Flags::IS_GD_OBJECT); }
		[[nodiscard]] _FORCE_INLINE_ bool is_convertible() const { return has_flag(Flags::IS_CONVERTIBLE); }
		[[nodiscard]] _FORCE_INLINE_ const StringName &get_name() const {
			return type_names[type_key];
		}
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

	typedef HashMap<int64_t, const TypeInfo*> TypeInfoMap;
	typedef std::array<const TypeInfo*, Variant::VARIANT_MAX> FallbackTable;

	static TypeInfoMap type_infos;
	static TypeInfoMap object_class_infos;
	static FallbackTable variant_fallbacks;
	static const TypeInfo *ref_fallback_type_info;
	static bool is_registration_ready;
	static Ref<Mutex> register_mutex;

	EntryOwner entries_owner = {};
	EntryMap entries = {};
	const Blackboard *parent = nullptr;
	Ref<Mutex> mutex = {};

	template <typename T>
	static EntryBase* create_entry(const StringName &p_name, EntryOwner &p_owner, EntryMap &p_entries, EntryBase* previous = nullptr) {
		T* entry = memnew(T());

		const RID rid = previous ? previous->get_self() : p_owner.make_rid(entry);
		entry->set_self(rid);
		p_entries[p_name] = entry;

		if (previous) {
			memdelete(previous);
		}

		return entry;
	}

	bool validate_candidate_parent(const Blackboard *p_candidate) const;
	void free_entry(const EntryMap::Iterator &iter);

	void create_new_variant_entry(const StringName &p_name, const Variant &p_value, Variant::Type variant_type, EntryBase *previous = nullptr);

	template <typename T>
	bool find_entry(const StringName &p_name, EntryMap::ConstIterator &p_out_result, bool p_check_parents) const;

	static void register_core_variant_types();
	static void registration_clear();

	_FORCE_INLINE_ static void registration_lock() {
		ERR_FAIL_COND(register_mutex.is_null());
		register_mutex->lock();
	}

	_FORCE_INLINE_ static void registration_unlock() {
		ERR_FAIL_COND(register_mutex.is_null());
		register_mutex->unlock();
	}

public:

	static void registration_init();
	static void registration_finish();

	template <typename T>
	static void register_type();

	explicit Blackboard();
	~Blackboard();

	_FORCE_INLINE_ bool is_empty() const { return entries.is_empty(); }
	_FORCE_INLINE_ uint32_t size() const { return entries.size(); }

	bool set_parent(const Blackboard *p_parent);
	[[nodiscard]] const Blackboard *get_parent() const;
	[[nodiscard]] bool is_ancestor(const Blackboard *p_candidate) const;

	_FORCE_INLINE_ void lock() const {
		ERR_FAIL_COND(mutex.is_null());
		mutex->lock();
	}

	_FORCE_INLINE_ void unlock() const {
		ERR_FAIL_COND(mutex.is_null());
		mutex->unlock();
	}

	template <typename T>
	bool try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents = true) const;

	template <typename T>
	[[nodiscard]] const T &get_entry_fast(const StringName &p_name, const T &p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	[[nodiscard]] T get_entry(const StringName &p_name, T p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	void set_entry_fast(const StringName &p_name, const T &p_value);

	template <typename T>
	void set_entry(const StringName &p_name, T p_value);

	bool import_entries(const TypedDictionary<StringName, Variant> &p_data);

	bool erase_entry(const StringName &p_name);

	[[nodiscard]] bool has_entry(const StringName &p_name, bool p_check_parents = true) const;

	[[nodiscard]] Dictionary export_entries(bool p_include_parents = true) const;

	[[nodiscard]] static Dictionary export_type_infos();
};

template <typename T>
void Blackboard::register_type() {

	registration_lock();

	typedef traits::resolved_type_t<T> type;
	static_assert(!std::is_same_v<type, Variant>, "Cannot register Variant type itself.");
	if (RegisteredTypeInfo<type>::is_registered()) {
		registration_unlock();
		return;
	}

	typedef std::remove_pointer_t<type> without_ptr;
	if constexpr (traits::is_ref_counted_v<without_ptr>) {
		register_type<Ref<without_ptr>>();
		registration_unlock();
		return;
	}

	if constexpr (!std::is_pointer_v<type> && traits::is_gd_object_type_v<type>) {
		register_type<type*>();
		registration_unlock();
		return;
	}

	TypeInfo type_info = {};
	const StringName name = get_type_name_static<type>();
	type_info.type_key = name.hash();
	TypeInfo::type_names[type_info.type_key] = name;

	constexpr bool is_const_obj_type = std::is_const_v<without_ptr>;

	if constexpr (traits::convertible_info_needs_conversion_v<type>) {
		typedef typename traits::convertible_info<type>::conversion_type conversion_type;
		
		static_assert(!std::is_same_v<type, Variant>, "Cannot register Variant as a convertable type.");
		static_assert(!std::is_same_v<conversion_type, Variant>, "Cannot register Variant as a conversion type.");
		static_assert(traits::is_variant_type<conversion_type>::value, "Conversion type MUST be directly compatible with Variant");

		type_info.create = create_entry<EntryVariantConvertable<type, conversion_type>>;
		type_info.variant_type = static_cast<Variant::Type>(GetTypeInfo<conversion_type>::VARIANT_TYPE);
		type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);
		type_info.enable_flag(TypeInfo::Flags::IS_CONVERTIBLE);
	}
	else if constexpr (traits::is_variant_type_v<type>) {
		type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);
		type_info.variant_type = static_cast<Variant::Type>(GetTypeInfo<type>::VARIANT_TYPE);

		if constexpr (traits::is_exactly_gd_object_v<without_ptr>) {
			if constexpr (!is_const_obj_type) {
				type_info.object_class_key = Object::get_class_static().hash();
			}
			type_info.enable_flag(TypeInfo::Flags::IS_OBJECT_PTR_TYPE);
			type_info.create = create_entry<EntryVariant<type>>;
		}
		else if constexpr (traits::is_gd_object_type_v<without_ptr>) {
			if constexpr (!is_const_obj_type) {
				type_info.object_class_key = without_ptr::get_class_static().hash();
			}
			type_info.enable_flag(TypeInfo::Flags::IS_OBJECT_PTR_TYPE);
			type_info.create = create_entry<EntryVariantObject<type>>;
		}
		else if constexpr (traits::is_gd_ref_helper_v<type>) {
			typedef traits::extract_ref_counted_type_t<type> ref_counted_type;
			if constexpr (!is_const_obj_type && !std::is_const_v<ref_counted_type>) {
				type_info.object_class_key = ref_counted_type::get_class_static().hash();
			}
			type_info.enable_flag(TypeInfo::Flags::IS_REF_COUNTED);
			type_info.create = create_entry<EntryVariant<type>>;
		}
		else {
			type_info.create = create_entry<EntryVariant<type>>;
		}
	}
	else {
		type_info.type_key = name.hash();
		type_info.create = create_entry<EntryData<type>>;
	}

	type_info.enable_flag(TypeInfo::Flags::IS_REGISTERED);
	RegisteredTypeInfo<T>::init(type_info);
	type_infos[type_info.type_key] = RegisteredTypeInfo<type>::get_info_ptr();
	if (type_info.object_class_key != 0) {
		object_class_infos[type_info.object_class_key] = RegisteredTypeInfo<type>::get_info_ptr();
	}

	registration_unlock();
}

template <>
inline bool Blackboard::find_entry<Variant>(const StringName &p_name, EntryMap::ConstIterator &p_out_result, const bool p_check_parents) const {
	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		p_out_result = iter;
		return true;
	}

	if (likely(p_check_parents)) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			current->lock();

			iter = current->entries.find(p_name);
			if (iter != current->entries.end()) {
				p_out_result = iter;
				current->unlock();
				return true;
			}

			const Blackboard *next = current->parent;
			current->unlock();
			current = next;
		}
	}

	return false;
}

template <>
inline bool Blackboard::find_entry<const Variant>(const StringName &p_name, EntryMap::ConstIterator &p_out_result, const bool p_check_parents) const {
	return find_entry<Variant>(p_name, p_out_result, p_check_parents);
}

template <>
inline bool Blackboard::try_get_entry<Variant>(const StringName &p_name, Variant &p_out_result, const bool p_check_parents) const {
	lock();

	EntryMap::ConstIterator iter;
	if (unlikely(!find_entry<Variant>(p_name, iter, p_check_parents))) {
		unlock();
		return false;
	}

	const EntryBase *entry = iter->value;
	p_out_result = entry->as_variant();

	unlock();
	return true;
}

template <>
inline const Variant &Blackboard::get_entry_fast<Variant>(const StringName &p_name, const Variant &p_default, const bool p_check_parents) const {
	lock();

	thread_local Variant result;
	result = p_default;
	try_get_entry<Variant>(p_name, result, p_check_parents);

	unlock();
	return result;
}

inline void Blackboard::create_new_variant_entry(const StringName &p_name, const Variant &p_value, const Variant::Type variant_type, EntryBase* previous) {
	registration_lock();

	const TypeInfo *type_info;
	if (unlikely(variant_type == Variant::Type::OBJECT)) {
		const Object *obj = p_value;
		const String class_name = obj->get_class();
		const int64_t object_class_key = class_name.hash();

		const auto type_info_iter = object_class_infos.find(object_class_key);
		if (likely(type_info_iter == object_class_infos.end())) {
			const Ref<RefCounted> ref = p_value;
			type_info = ref.is_null() ? variant_fallbacks[Variant::Type::OBJECT] : ref_fallback_type_info;
		} else {
			type_info = type_info_iter->value;
		}
	} else {
		type_info = variant_fallbacks[variant_type];
	}

	registration_unlock();

	EntryBase *entry = type_info->create(p_name, entries_owner, entries, previous);
	entry->set_from(p_value);
}

template <>
inline void Blackboard::set_entry_fast<Variant>(const StringName &p_name, const Variant& p_value) {
	lock();

	const Variant::Type variant_type = p_value.get_type();
	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		if (unlikely(p_value.get_type() == Variant::NIL)) {
			free_entry(iter);
			unlock();
			return;
		}

		EntryBase *existing_entry = iter->value;

		if (unlikely(existing_entry->get_variant_type() == Variant::OBJECT)) {
			const int64_t existing_type_key = existing_entry->get_object_class_key();
			const Object *incoming_obj = p_value;
			const String incoming_class_name = incoming_obj->get_class();
			const int64_t incoming_class_key = incoming_class_name.hash();

			if (likely(existing_type_key == incoming_class_key)) {
				existing_entry->set_from(p_value);
				unlock();
				return;
			}
			// else regenerate entry with fallback
		}
		else if (likely(existing_entry->get_variant_type() == variant_type)) {
			existing_entry->set_from(p_value);
			unlock();
			return;
		}

		create_new_variant_entry(p_name, p_value, variant_type, existing_entry);

		unlock();
		return;
	}

	if (unlikely(variant_type == Variant::Type::NIL)) {
		unlock();
		return;
	}

	create_new_variant_entry(p_name, p_value, variant_type);
	unlock();
}

template <>
inline void Blackboard::set_entry<Variant>(const StringName &p_name, Variant p_value) {
	set_entry_fast(p_name, p_value);
}

template <typename T>
bool Blackboard::find_entry(const StringName &p_name, EntryMap::ConstIterator &p_out_result, const bool p_check_parents) const {
	registration_lock();

	typedef traits::resolved_type_t<T> type;
	const TypeInfo &type_info = RegisteredTypeInfo<type>::get_info();
	if (unlikely(!type_info.is_registered())) {
		registration_unlock();
		return false;
	}

	const auto type_key = type_info.type_key;
	registration_unlock();

	lock();

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		if (likely(iter->value->get_type_key() == type_key)) {
			p_out_result = iter;
			unlock();
			return true;
		}
		else {
			unlock();
			return false;
		}
	}

	if (likely(p_check_parents)) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			current->lock();
			iter = current->entries.find(p_name);
			if (iter != current->entries.end()) {
				const auto entry_type_key = iter->value->get_type_key();

				if (entry_type_key != type_key) {
					current->unlock();
					unlock();
					return false;
				}

				p_out_result = iter;
				current->unlock();
				unlock();
				return true;
			}

			const Blackboard *next = current->parent;
			current->unlock();
			current = next;
		}
	}

	unlock();
	return false;
}

template <typename T>
bool Blackboard::try_get_entry(const StringName &p_name, T &p_out_result, const bool p_check_parents) const {
	static_assert(!std::is_const_v<T>, "Can't use const types with try_get_entry.");
	if constexpr (!std::is_const_v<T>) {
		lock();

		EntryMap::ConstIterator iter;
		if (unlikely(!find_entry<T>(p_name, iter, p_check_parents))) {
			unlock();
			return false;
		}

		typedef traits::resolved_type_t<T> type;
		auto *entry = dynamic_cast<EntryData<type> *>(iter->value);
		p_out_result = entry->value;

		unlock();
		return true;
	}
	else {
		return false;
	}
}

template <typename T>
const T &Blackboard::get_entry_fast(const StringName &p_name, const T &p_default, const bool p_check_parents) const {
	lock();

	EntryMap::ConstIterator iter;
	if (unlikely(!find_entry<T>(p_name, iter, p_check_parents))) {
		unlock();
		return p_default;
	}

	const EntryData<T> *entry = dynamic_cast<const EntryData<T> *>(iter->value);
	const T &result = entry->value;
	unlock();
	return result;
}

template <typename T>
T Blackboard::get_entry(const StringName &p_name, T p_default, const bool p_check_parents) const {
	return get_entry_fast(p_name, p_default, p_check_parents);
}

template <typename T>
void Blackboard::set_entry_fast(const StringName &p_name, const T &p_value)  {

	registration_lock();
	if (unlikely(!RegisteredTypeInfo<T>::is_registered())) {
		register_type<T>();
	}

	const TypeInfo &type_info = RegisteredTypeInfo<T>::get_info();
	registration_unlock();

	lock();

	auto iter = entries.find(p_name);
	EntryBase *existing_entry = nullptr;
	if (likely(iter != entries.end())) {
		existing_entry = iter->value;
		if (likely(existing_entry->get_type_key() == type_info.type_key)) {
			EntryData<T> *entry = dynamic_cast<EntryData<T> *>(existing_entry);
			entry->value = p_value;
			return;
		}
	}

	EntryData<T> *new_entry = dynamic_cast<EntryData<T> *>(type_info.create(p_name, entries_owner, entries, existing_entry));
	new_entry->value = p_value;

	unlock();
}

template <typename T>
void Blackboard::set_entry(const StringName &p_name, T p_value) {
	set_entry_fast(p_name, p_value);
}

template <typename T>
Blackboard::TypeInfo Blackboard::RegisteredTypeInfo<T>::type_info = TypeInfo();

template <typename T, typename U>
Variant Blackboard::EntryVariantConvertable<T, U>::as_variant() const {
	U conversion = traits::convertible_info<T>::convert_from(this->value);
	return Variant(conversion);
}

template <typename T, typename U>
void Blackboard::EntryVariantConvertable<T, U>::set_from(const Variant &p_value) {
	this->value = traits::convertible_info<T>::convert_to(p_value);
}

MAKE_VARIANT_CONVERTIBLE_TYPE(char16_t, int64_t, return static_cast<char16_t>(x);, return x;)
MAKE_VARIANT_CONVERTIBLE_TYPE(char32_t, int64_t, return static_cast<char32_t>(x);, return x;)
MAKE_VARIANT_CONVERTIBLE_TYPE(ObjectID, uint64_t, return ObjectID(x);, return x.operator uint64_t();)

} //namespace hydrogen
