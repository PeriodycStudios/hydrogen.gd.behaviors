
#ifndef VARIANT_TYPE_TRAITS_HPP
#define VARIANT_TYPE_TRAITS_HPP


#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

#include <type_traits>

namespace hydrogen::traits {
using namespace godot;
using namespace std;

template <typename T, typename = void>
struct is_ref_counted : false_type {};

template <typename T>
struct is_ref_counted<T, enable_if_t<is_base_of_v<RefCounted, T>, void>> : true_type {};

template <typename T>
constexpr bool is_ref_counted_v = is_ref_counted<T>::value;

template <typename T, typename = void>
struct is_gd_ref_helper : false_type {};

template <typename T>
struct is_gd_ref_helper<T,
	enable_if_t<
		conjunction_v<
			is_void<void_t<decltype(declval<T>().unref())>>,
			is_void<void_t<decltype(declval<T>().is_valid())>>,
			is_void<void_t<decltype(declval<T>().is_null())>>,
			is_void<void_t<decltype(declval<T>().ptr())>>
		>
	, void>> : true_type {};

template <typename T>
constexpr bool is_gd_ref_helper_v = is_gd_ref_helper<T>::value;

template <typename T, typename = void>
struct extract_ref_counted_type { typedef T type; };

template <typename T>
struct extract_ref_counted_type<T, enable_if_t<is_gd_ref_helper<T>::value, void>> {
	typedef std::remove_pointer_t<decltype(declval<T>().ptr())> type;
};

template <typename T>
using extract_ref_counted_type_t = typename extract_ref_counted_type<T>::type;

template <typename T>
struct resolved_type {
	typedef remove_const_t<remove_reference_t<T>> type;
};

template <typename T>
using resolved_type_t = typename resolved_type<T>::type;

template <typename T, typename = void>
struct is_gd_object_type : false_type {};

template <typename T>
struct is_gd_object_type<T,
	enable_if_t<
		is_base_of_v<Object, resolved_type_t<T>> &&
			!is_base_of_v<RefCounted, resolved_type_t<T>>
>> : true_type {};

template <typename T>
constexpr bool is_gd_object_type_v = is_gd_object_type<T>::value;

template <typename T, typename = void>
struct is_variant_type : false_type {};

template <typename T>
struct is_variant_type<T,
	void_t<decltype(GetTypeInfo<T>::VARIANT_TYPE)>> : true_type {};

template <typename T>
constexpr bool is_variant_type_v = is_variant_type<T>::value;

template <typename T>
struct is_exactly_gd_object : false_type {};

template <>
struct is_exactly_gd_object<Object> : true_type {};

template <>
struct is_exactly_gd_object<const Object> : true_type {};

template <>
struct is_exactly_gd_object<Object *> : true_type {};

template <>
struct is_exactly_gd_object<const Object *> : true_type {};

template <typename T>
constexpr bool is_exactly_gd_object_v = is_exactly_gd_object<T>::value;

template <typename T, typename = void>
struct resolve_object_ptr_type {
	typedef Object * type;
};

template <typename T>
struct resolve_object_ptr_type<T,
	std::enable_if_t<
		is_pointer_v<T> &&
		is_base_of_v<Object, remove_pointer_t<T>> &&
		is_const_v<remove_pointer_t<T>>
>> : std::true_type {

	typedef const Object * type;
};

template <typename T>
using resolve_object_ptr_type_t = typename resolve_object_ptr_type<T>::type;



} //namespace hydrogen::traits

#endif //VARIANT_TYPE_TRAITS_HPP
