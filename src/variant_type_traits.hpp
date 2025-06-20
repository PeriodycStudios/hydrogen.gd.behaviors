
#ifndef VARIANT_TYPE_TRAITS_HPP
#define VARIANT_TYPE_TRAITS_HPP

#include "variant_type_traits.hpp"

#include <type_traits>

using namespace godot;

namespace hydrogen::traits {

using namespace std;

template <typename T, typename = void>
struct is_ref_counted : false_type {};

template <typename T>
struct is_ref_counted<T, enable_if_t<is_base_of_v<RefCounted, T>, void>> : true_type {};

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

template <typename T, typename = void>
struct remove_ref_helper { typedef T type; };

template <typename T>
struct remove_ref_helper<T, enable_if_t<is_gd_ref_helper<T>::value, void>> {
	typedef decltype(declval<T>().ptr()) type;
};

template <typename T, typename = void>
struct is_gd_object_type : false_type {};

template <typename T>
struct is_gd_object_type<T,
	enable_if_t<is_base_of_v<Object, remove_pointer_t<T>>, void>> : true_type {};

template <typename T, typename = void>
struct has_class_static : false_type {};

template <typename T>
struct has_class_static<T,
		void_t<decltype(remove_pointer_t<T>::get_class_static())>> : true_type {};

template <typename T, typename = void>
struct is_variant_type : false_type {};

template <typename T>
struct is_variant_type<T,
	void_t<decltype(GetTypeInfo<T>::VARIANT_TYPE)>> : true_type {};

template <typename T>
struct unadorned_type {
	typedef remove_cv_t<remove_reference_t<T>> type;
};


} //namespace hydrogen::traits

#endif //VARIANT_TYPE_TRAITS_HPP
