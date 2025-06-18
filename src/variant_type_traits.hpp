
#ifndef VARIANT_TYPE_TRAITS_HPP
#define VARIANT_TYPE_TRAITS_HPP

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <type_traits>

using namespace godot;

namespace hydrogen::traits {

template <typename T, typename = void>
struct variant_has_operator : std::false_type {};

template <typename T>
struct variant_has_operator<T,
		std::void_t<
				decltype(std::declval<Variant>().operator T())>> : std::true_type {};

template <typename T, typename = void>
struct has_variant_operator : std::false_type {};

template <typename T>
struct has_variant_operator<T,
		std::void_t<decltype(std::declval<T>().operator Variant())>> : std::true_type {};

template <typename T, typename = void>
struct can_construct_variant_from : std::false_type {};

template <typename T>
struct can_construct_variant_from<T,
		std::void_t<decltype(Variant(std::declval<T>()))>> : std::true_type {};

template <typename T, typename = void>
struct can_construct_from_variant : std::false_type {};

template <typename T>
struct can_construct_from_variant<T,
		std::void_t<decltype(T(std::declval<Variant>()))>> : std::true_type {};

template <typename T, typename = void>
struct is_ref_type : std::false_type {};

template <typename T>
struct is_ref_type<T,
std::void_t<decltype(std::declval<T>().unref())>> : std::true_type {};

template <typename T, typename = void>
struct is_object_type : std::false_type {};

template <typename T>
struct is_object_type<T,
	std::enable_if_t<std::is_base_of_v<Object, std::remove_pointer_t<T>>, void>> : std::true_type {};



template <typename T, typename = void>
struct is_variant_compatible : std::false_type {};

template <typename T>
struct is_variant_compatible<T,
		std::enable_if_t<
				!std::is_pointer_v<T> && (can_construct_variant_from<T>::value && can_construct_from_variant<T>::value) && (variant_has_operator<T>::value || has_variant_operator<T>::value), void>> : std::true_type {};

template <typename T>
struct is_variant_compatible<T *,
		std::enable_if_t<std::is_base_of<Object, T>::value, void>> : std::true_type {};

template <typename T, typename = void>
struct has_class_name : std::false_type {};

template <typename T>
struct has_class_name<T,
		std::void_t<decltype(std::remove_pointer_t<T>::get_class_name())>> : std::true_type {};

template <typename T, typename = void>
struct has_get_type_info : std::false_type {};

template <typename T>
struct has_get_type_info<T,
	std::void_t<decltype(GetTypeInfo<T>::VARIANT_TYPE)>> : std::true_type {};

} //namespace hydrogen::traits

#endif //VARIANT_TYPE_TRAITS_HPP
