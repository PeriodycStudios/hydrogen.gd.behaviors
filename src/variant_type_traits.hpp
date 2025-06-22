
#ifndef VARIANT_TYPE_TRAITS_HPP
#define VARIANT_TYPE_TRAITS_HPP

#include <type_traits>

using namespace godot;

namespace hydrogen::traits {

using namespace std;

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
