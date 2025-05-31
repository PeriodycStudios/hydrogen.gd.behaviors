//
// Created by tkey on 5/16/25.
//

#ifndef IS_BLACKBOARD_STORAGE_TYPE_HPP
#define IS_BLACKBOARD_STORAGE_TYPE_HPP

#include <type_traits>
#include <godot_cpp/core/type_info.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace hydrogen {

template <typename T, typename = void>
struct blackboard_storage_type : std::false_type {};

#define GET_TYPE_KEY(type)								\
static const StringName &get_type_key() {				\
														\
	static const StringName key = StringName(#type);	\
	return key;											\
}

#define GET_CLASS_TYPE_KEY(type)										\
static const StringName &get_type_key() {								\
																		\
	static const StringName & class_name = type::get_class_static();	\
	return class_name;													\
}

#define DECLARE_BLACKBOARD_STORAGE_TYPE(type)			\
template<>												\
struct blackboard_storage_type<type> : std::true_type {	\
														\
	GET_TYPE_KEY(type)									\
};

DECLARE_BLACKBOARD_STORAGE_TYPE(bool)
DECLARE_BLACKBOARD_STORAGE_TYPE(uint8_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(int8_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(uint16_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(int16_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(uint32_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(int32_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(uint64_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(int64_t)

DECLARE_BLACKBOARD_STORAGE_TYPE(char16_t)
DECLARE_BLACKBOARD_STORAGE_TYPE(char32_t)

DECLARE_BLACKBOARD_STORAGE_TYPE(float)
DECLARE_BLACKBOARD_STORAGE_TYPE(double)

DECLARE_BLACKBOARD_STORAGE_TYPE(String)
DECLARE_BLACKBOARD_STORAGE_TYPE(Vector2)
DECLARE_BLACKBOARD_STORAGE_TYPE(Vector2i)
DECLARE_BLACKBOARD_STORAGE_TYPE(Rect2)
DECLARE_BLACKBOARD_STORAGE_TYPE(Rect2i)
DECLARE_BLACKBOARD_STORAGE_TYPE(Vector3)
DECLARE_BLACKBOARD_STORAGE_TYPE(Vector3i)
DECLARE_BLACKBOARD_STORAGE_TYPE(Transform2D)
DECLARE_BLACKBOARD_STORAGE_TYPE(Vector4)
DECLARE_BLACKBOARD_STORAGE_TYPE(Vector4i)
DECLARE_BLACKBOARD_STORAGE_TYPE(Plane)
DECLARE_BLACKBOARD_STORAGE_TYPE(Quaternion)
DECLARE_BLACKBOARD_STORAGE_TYPE(AABB)
DECLARE_BLACKBOARD_STORAGE_TYPE(Basis)
DECLARE_BLACKBOARD_STORAGE_TYPE(Transform3D)
DECLARE_BLACKBOARD_STORAGE_TYPE(Projection)
DECLARE_BLACKBOARD_STORAGE_TYPE(Color)
DECLARE_BLACKBOARD_STORAGE_TYPE(StringName)
DECLARE_BLACKBOARD_STORAGE_TYPE(NodePath)
DECLARE_BLACKBOARD_STORAGE_TYPE(RID)
DECLARE_BLACKBOARD_STORAGE_TYPE(ObjectID)
DECLARE_BLACKBOARD_STORAGE_TYPE(Callable)
DECLARE_BLACKBOARD_STORAGE_TYPE(Signal)
DECLARE_BLACKBOARD_STORAGE_TYPE(Dictionary)
DECLARE_BLACKBOARD_STORAGE_TYPE(Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedByteArray)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedInt32Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedInt64Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedFloat32Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedFloat64Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedStringArray)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedVector2Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedVector3Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedVector4Array)
DECLARE_BLACKBOARD_STORAGE_TYPE(PackedColorArray)

DECLARE_BLACKBOARD_STORAGE_TYPE(Variant)

template<typename T>
struct blackboard_storage_type<
	T*, typename EnableIf<TypeInherits<Object, T>::value>::type
> : std::true_type {
	GET_CLASS_TYPE_KEY(T)
};

template<typename T>
struct blackboard_storage_type<
	const T*, typename EnableIf<TypeInherits<Object, T>::value>::type
> : std::true_type {
	GET_CLASS_TYPE_KEY(T)
};

template<typename T>
struct blackboard_storage_type<
	Ref<T>, typename EnableIf<TypeInherits<RefCounted, T>::value>::type
> : std::true_type {
	GET_CLASS_TYPE_KEY(T)
};

// TODO: Setup interface to allow for user-defined types and storing
// alternate backings for certain entry kinds (IE const char* -> backed as String)

#undef GET_TYPE_KEY
#undef DECLARE_BLACKBOARD_STORAGE_TYPE

}

#endif //IS_BLACKBOARD_STORAGE_TYPE_HPP
