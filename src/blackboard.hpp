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
#if BLACKBOARD_VERBOSE
#include <godot_cpp/core/print_string.hpp>
#endif

#include <functional>

#include "rid_data.hpp"
#include "variant_type_traits.hpp"
#include "type_name.hpp"

using namespace godot;

namespace hydrogen {

namespace traits {

template <typename T>
struct convertible_info {
	typedef false_type needs_conversion;
	// typedef variant_compatible_type conversion_type;
	// static constexpr T convert_to(const U x) { return x; }
	// static constexpr U convert_from(const T x) { return x; }
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

		[[nodiscard]] virtual String get_type_name() const = 0;

		[[nodiscard]] virtual String get_entry_type_name() const {
			return "EntryBase";
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

		[[nodiscard]] String get_type_name() const override {
			const std::string raw_name = RegisteredTypeInfo<type>::get_info().raw_name;
			return {raw_name.c_str()};
		}

		[[nodiscard]] String get_entry_type_name() const override {
			return "EntryData";
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

		[[nodiscard]] String get_entry_type_name() const override {
			return "EntryVariant";
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

		[[nodiscard]] String get_entry_type_name() const override {
			return "EntryVariantObject";
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

		[[nodiscard]] String get_entry_type_name() const override {
			return "EntryVariantConvertable";
		}
	};

	typedef HashMap<StringName, EntryBase*> EntryMap;
	typedef RID_PtrOwner<EntryBase> EntryOwner;

	typedef std::function<EntryBase*(const StringName &p_name, EntryOwner &p_owner, EntryMap &p_entries)> entry_factory;

	struct TypeInfo {

		enum struct Flags : uint8_t {
			NONE = 0,
			IS_REGISTERED = 1 << 0,
			IS_VARIANT_TYPE = 1 << 1,
			IS_OBJECT_PTR_TYPE = 1 << 2,
			IS_REF_COUNTED = 1 << 3,
			IS_CONVERTIBLE = 1 << 4,

			IS_GD_OBJECT = IS_OBJECT_PTR_TYPE | IS_REF_COUNTED,
		};

		entry_factory factory = nullptr;
		std::string raw_name; // any attempt to use String or StringName here crashes the plugin on load!
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
	static bool core_variants_registered;

	EntryOwner entries_owner;
	EntryMap entries;
	Blackboard *parent;

	template <typename T>
	static EntryBase* create_entry(const StringName &p_name, EntryOwner &p_owner, EntryMap &p_entries) {
		T* entry = memnew(T());
		const RID rid = p_owner.make_rid(entry);
		entry->set_self(rid);

		p_entries[p_name] = entry;
		return entry;
	}

	bool validate_parent(const Blackboard *p_parent) const;
	void free_entry(EntryMap::Iterator &iter);

	template<typename T>
	bool find_entry(const StringName &p_name, EntryMap::ConstIterator &p_out_result, bool p_check_parents) const;

public:

	static void register_core_variant_types();

	template <typename T>
	static void register_type();

	explicit Blackboard() : parent(nullptr) {}
	~Blackboard();

	_FORCE_INLINE_ bool set_parent(Blackboard *p_parent) {
		if (validate_parent(p_parent)) {
			parent = p_parent;
			return true;
		}
		return false;
	}

	[[nodiscard]] _FORCE_INLINE_ Blackboard *get_parent() const { return parent; }
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

template <typename T>
void Blackboard::register_type() {

	typedef traits::resolved_type_t<T> type;
	static_assert(!std::is_same_v<type, Variant>, "Cannot register Variant type itself.");
	if (RegisteredTypeInfo<type>::is_registered()) {
		return;
	}

	typedef std::remove_pointer_t<type> without_ptr;
	if constexpr (traits::is_ref_counted_v<without_ptr>) {
		register_type<Ref<without_ptr>>();
		return;
	}

	if constexpr (!std::is_pointer_v<type> && traits::is_gd_object_type_v<type>) {
		register_type<type*>();
		return;
	}

	TypeInfo type_info = {};
	type_info.raw_name = std::string(type_name<type>());
	const auto name = String(type_info.raw_name.c_str());
	type_info.type_key = name.hash();

	if constexpr (traits::convertible_info_needs_conversion_v<type>) {
		typedef typename traits::convertible_info<type>::conversion_type conversion_type;
		
		static_assert(!std::is_same_v<type, Variant>, "Cannot register Variant as a convertable type.");
		static_assert(!std::is_same_v<conversion_type, Variant>, "Cannot register Variant as a conversion type.");
		static_assert(traits::is_variant_type<conversion_type>::value, "Conversion type MUST be directly compatible with Variant");

		type_info.factory = create_entry<EntryVariantConvertable<type, conversion_type>>;
		type_info.variant_type = static_cast<Variant::Type>(GetTypeInfo<conversion_type>::VARIANT_TYPE);
		type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);
		type_info.enable_flag(TypeInfo::Flags::IS_CONVERTIBLE);
	}
	else if constexpr (traits::is_variant_type_v<type>) {
		type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);
		type_info.variant_type = static_cast<Variant::Type>(GetTypeInfo<type>::VARIANT_TYPE);

		if constexpr (traits::is_exactly_gd_object_v<without_ptr>) {
			type_info.object_class_key = Object::get_class_static().hash();
			type_info.enable_flag(TypeInfo::Flags::IS_OBJECT_PTR_TYPE);
			type_info.factory = create_entry<EntryVariant<type>>;
		}
		else if constexpr (traits::is_gd_object_type_v<without_ptr>) {
			type_info.object_class_key = without_ptr::get_class_static().hash();
			type_info.enable_flag(TypeInfo::Flags::IS_OBJECT_PTR_TYPE);
			type_info.factory = create_entry<EntryVariantObject<type>>;
		}
		else if constexpr (traits::is_gd_ref_helper_v<type>) {
			typedef traits::extract_ref_counted_type_t<type> ref_counted_type;
			type_info.object_class_key = ref_counted_type::get_class_static().hash();
			type_info.enable_flag(TypeInfo::Flags::IS_REF_COUNTED);
			type_info.factory = create_entry<EntryVariant<type>>;
		}
		else {
			type_info.factory = create_entry<EntryVariant<type>>;
		}
	}
	else {
		type_info.type_key = name.hash();
		type_info.factory = create_entry<EntryData<type>>;
	}

	type_info.enable_flag(TypeInfo::Flags::IS_REGISTERED);
	RegisteredTypeInfo<T>::init(type_info);
	type_infos[type_info.type_key] = RegisteredTypeInfo<type>::get_info_ptr();
	if (type_info.object_class_key != 0) {
		object_class_infos[type_info.object_class_key] = RegisteredTypeInfo<type>::get_info_ptr();
	}

#if BLACKBOARD_VERBOSE
	print_line("Type [", name, "] fully registered with key: ", type_info.type_key);
	if (type_info.variant_type != Variant::NIL) {
		const String variant_name = Variant::get_type_name(type_info.variant_type);
		print_line("- Type associated with: ", variant_name);
		if (type_info.is_convertible()) {
			print_line("- Is Convertible");
		}
		if (type_info.is_gd_object()) {
			if (type_info.is_object_ptr_type()) {
				print_line("- Is Object pointer type");
			}
			if (type_info.is_ref_counted()) {
				print_line("- Is Ref-Counted");
			}
		}
	}
	print_line("");
#endif
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

template <>
inline bool Blackboard::find_entry<const Variant>(const StringName &p_name, EntryMap::ConstIterator &p_out_result, const bool p_check_parents) const {
	return find_entry<Variant>(p_name, p_out_result, p_check_parents);
}

template <typename T>
bool Blackboard::find_entry(const StringName &p_name, EntryMap::ConstIterator &p_out_result, const bool p_check_parents) const {
	typedef traits::resolved_type_t<T> type;
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
inline bool Blackboard::try_get_entry<Variant>(const StringName &p_name, Variant &p_out_result, const bool p_check_parents) const {
	EntryMap::ConstIterator iter;
	if (unlikely(!find_entry<Variant>(p_name, iter, p_check_parents))) {
		return false;
	}

	const EntryBase *entry = iter->value;
	p_out_result = entry->as_variant();
	return true;
}

template <typename T>
bool Blackboard::try_get_entry(const StringName &p_name, T &p_out_result, const bool p_check_parents) const {
	static_assert(!std::is_const_v<T>, "Can't use const types with try_get_entry.");
	if constexpr (!std::is_const_v<T>) {
		EntryMap::ConstIterator iter;
		if (unlikely(!find_entry<T>(p_name, iter, p_check_parents))) {
			return false;
		}

		typedef traits::resolved_type_t<T> type;
		auto *entry = dynamic_cast<EntryData<type> *>(iter->value);
		p_out_result = entry->value;
		return true;
	}
	else {
		return false;
	}
}

template <>
inline Variant Blackboard::get_entry<Variant>(const StringName &p_name, const Variant &p_default, const bool p_check_parents ) const {
	Variant result = p_default;
	try_get_entry<Variant>(p_name, result, p_check_parents);
	return result;
}

template <>
inline const Variant Blackboard::get_entry<const Variant>(const StringName &p_name, const Variant &p_default, const bool p_check_parents) const {
	return get_entry<Variant>(p_name, p_default, p_check_parents);
}

template <typename T>
T Blackboard::get_entry(const StringName &p_name, const T &p_default, const bool p_check_parents) const {
	typedef traits::resolved_type_t<T> type;
	type result = p_default;
	try_get_entry<type>(p_name, result, p_check_parents);
	return result;
}

template <>
inline void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value) {
	const auto variant_type = p_value.get_type();

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		// print_line("[Variant] Found entry.");
		EntryBase *existing_entry = iter->value;

		if (unlikely(p_value.get_type() == Variant::NIL)) {
			// print_line("[Variant] Type is nil, deleting entry.");
			free(existing_entry);
			return;
		}

		if (unlikely(existing_entry->get_variant_type() == Variant::OBJECT)) {
			// print_line("[Variant] type is object, attempting resolve");
			const int64_t existing_type_key = existing_entry->get_object_class_key();
			// print_line("Existing Object class key: ", existing_type_key);
			const Object *incoming_obj = p_value;
			const String incoming_class_name = incoming_obj->get_class();
			const int64_t incoming_class_key = incoming_class_name.hash();

			// print_line("Incoming: ", incoming_class_name, " ", incoming_class_key);

			if (likely(existing_type_key == incoming_class_key)) {
				// print_line("[Variant] Same type, setting.");
				existing_entry->set_from(p_value);
				return;
			}
			// else regenerate entry with fallback
		}
		else if (likely(existing_entry->get_variant_type() == variant_type)) {
			// print_line("[Variant] Type is not object, simple set.");
			existing_entry->set_from(p_value);
			return;
		}

		// print_line("[Variant] Type mismatch, regenerating entry.");
		free_entry(iter);
	}

	// NIL will delete an existing entry or do nothing if one doesn't exist.
	if (unlikely(variant_type == Variant::Type::NIL)) {
		return;
	}

	const TypeInfo *type_info;
	if (unlikely(variant_type == Variant::Type::OBJECT)) {
		// print_line("[Variant] Type is object, attempting resolve");
		const Object * obj = p_value;
		const String class_name = obj->get_class();
		const int64_t object_class_key = class_name.hash();

		// print_line("Object class key: ", class_name, " ", object_class_key);

		const auto type_info_iter = object_class_infos.find(object_class_key);
		if (likely(type_info_iter == object_class_infos.end())) {
			// print_line("Couldn't find object class key. Resorting to fallback.");
			const Ref<RefCounted> ref = p_value;

			// print_line("Is ref? ", !ref.is_null());
			type_info = ref.is_null() ?
			variant_fallbacks[Variant::Type::OBJECT] :
			ref_fallback_type_info;
		}
		else {
			// print_line("Found object type info");
			type_info = type_info_iter->value;
		}
	}
	else {
		// print_line("[Variant] Type is not object, simple set using fallback.");
		type_info = variant_fallbacks[variant_type];
	}

	EntryBase *entry = type_info->factory(p_name, entries_owner, entries);
	// print_line("Found ", entry->get_entry_type_name(), "<", entry->get_type_name(), "> ", entry->get_object_class_key(), " ", entry->get_type_key(), " ", entry->get_variant_type());
	entry->set_from(p_value);

}

template <>
inline void Blackboard::set_entry<const Variant>(const StringName& p_name, const Variant &p_value) {
	set_entry<Variant>(p_name, p_value);
}

template <typename T>
void Blackboard::set_entry(const StringName &p_name, const T &p_value)  {
	typedef traits::resolved_type_t<T> type;
	if (unlikely(!RegisteredTypeInfo<type>::is_registered())) {
		register_type<T>();
	}

	typedef traits::resolved_type_t<T> type;
	const TypeInfo &type_info = RegisteredTypeInfo<type>::get_info();

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		auto* existing_entry = dynamic_cast<EntryData<type> *>(iter->value);
		if (likely(existing_entry->get_type_key() == type_info.type_key)) {
			existing_entry->value = p_value;
			return;
		}
		free_entry(iter);
	}

	auto *new_entry = dynamic_cast<EntryData<type> *>(type_info.factory(p_name, entries_owner, entries));
	new_entry->value = p_value;
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

#endif //BLACKBOARD_HPP
