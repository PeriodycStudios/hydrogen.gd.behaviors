//
// Created by tkey on 5/16/25.
//

#ifndef BLACKBOARD_STORAGE_TYPE_HPP
#define BLACKBOARD_STORAGE_TYPE_HPP

#include <type_traits>
#include <godot_cpp/core/type_info.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace hydrogen {
//
// template <typename T, typename = void>
// struct BlackboardStorageType : std::false_type {};
//
// #define DECLARE_GET_TYPE_KEY(data_type)						\
// static const StringName &get_type_key() {					\
// 															\
// 	static const StringName key = StringName(#data_type);	\
// 	return key;												\
// }
//
// #define DECLARE_GET_GDCLASS_TYPE_KEY(data_type)								\
// static const StringName &get_type_key() {									\
// 																			\
// 	static const StringName & class_name = data_type::get_class_static();	\
// 	return class_name;														\
// }
//
// #define DECLARE_BLACKBOARD_DATA_TYPE(data_type)				\
// template<>													\
// struct BlackboardStorageType<data_type> : std::true_type {	\
// 	DECLARE_GET_TYPE_KEY(data_type)							\
// };															\
//
// #define DECLARE_BLACKBOARD_OBJECT_TYPE(obj_type)									\
// template<>																			\
// struct BlackboardStorageType<														\
// 	obj_type*, typename EnableIf<TypeInherits<Object, obj_type>::value>::type		\
// > : std::true_type {																\
// 	DECLARE_GET_GDCLASS_TYPE_KEY(obj_type)											\
// };																					\
// 																					\
// template<>																			\
// struct BlackboardStorageType<														\
// 	const obj_type*, typename EnableIf<TypeInherits<Object, obj_type>::value>::type	\
// > : std::true_type {																\
// 	DECLARE_GET_GDCLASS_TYPE_KEY(obj_type)											\
// };
//
// #define DECLARE_BLACKBOARD_REFERENCE_TYPE(ref_type)												\
// template<>																						\
// struct BlackboardStorageType<																	\
// 	Ref<ref_type>, typename EnableIf<TypeInherits<RefCounted, ref_type>::value>::type			\
// > : std::true_type {																			\
// 	DECLARE_GET_GDCLASS_TYPE_KEY(ref_type)														\
// };																								\
// 																								\
// template<>																						\
// struct BlackboardStorageType<																	\
// 	const Ref<ref_type> &, typename EnableIf<TypeInherits<RefCounted, ref_type>::value>::type	\
// > : std::true_type {																			\
// 	DECLARE_GET_GDCLASS_TYPE_KEY(ref_type)														\
// };
//
// DECLARE_BLACKBOARD_DATA_TYPE(bool)
// DECLARE_BLACKBOARD_DATA_TYPE(uint8_t)
// DECLARE_BLACKBOARD_DATA_TYPE(int8_t)
// DECLARE_BLACKBOARD_DATA_TYPE(uint16_t)
// DECLARE_BLACKBOARD_DATA_TYPE(int16_t)
// DECLARE_BLACKBOARD_DATA_TYPE(uint32_t)
// DECLARE_BLACKBOARD_DATA_TYPE(int32_t)
// DECLARE_BLACKBOARD_DATA_TYPE(uint64_t)
// DECLARE_BLACKBOARD_DATA_TYPE(int64_t)
//
// DECLARE_BLACKBOARD_DATA_TYPE(char16_t)
// DECLARE_BLACKBOARD_DATA_TYPE(char32_t)
//
// DECLARE_BLACKBOARD_DATA_TYPE(float)
// DECLARE_BLACKBOARD_DATA_TYPE(double)
//
// DECLARE_BLACKBOARD_DATA_TYPE(String)
// DECLARE_BLACKBOARD_DATA_TYPE(Vector2)
// DECLARE_BLACKBOARD_DATA_TYPE(Vector2i)
// DECLARE_BLACKBOARD_DATA_TYPE(Rect2)
// DECLARE_BLACKBOARD_DATA_TYPE(Rect2i)
// DECLARE_BLACKBOARD_DATA_TYPE(Vector3)
// DECLARE_BLACKBOARD_DATA_TYPE(Vector3i)
// DECLARE_BLACKBOARD_DATA_TYPE(Transform2D)
// DECLARE_BLACKBOARD_DATA_TYPE(Vector4)
// DECLARE_BLACKBOARD_DATA_TYPE(Vector4i)
// DECLARE_BLACKBOARD_DATA_TYPE(Plane)
// DECLARE_BLACKBOARD_DATA_TYPE(Quaternion)
// DECLARE_BLACKBOARD_DATA_TYPE(AABB)
// DECLARE_BLACKBOARD_DATA_TYPE(Basis)
// DECLARE_BLACKBOARD_DATA_TYPE(Transform3D)
// DECLARE_BLACKBOARD_DATA_TYPE(Projection)
// DECLARE_BLACKBOARD_DATA_TYPE(Color)
// DECLARE_BLACKBOARD_DATA_TYPE(StringName)
// DECLARE_BLACKBOARD_DATA_TYPE(NodePath)
// DECLARE_BLACKBOARD_DATA_TYPE(RID)
// DECLARE_BLACKBOARD_DATA_TYPE(ObjectID)
// DECLARE_BLACKBOARD_DATA_TYPE(Callable)
// DECLARE_BLACKBOARD_DATA_TYPE(Signal)
// DECLARE_BLACKBOARD_DATA_TYPE(Dictionary)
// DECLARE_BLACKBOARD_DATA_TYPE(Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedByteArray)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedInt32Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedInt64Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedFloat32Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedFloat64Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedStringArray)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedVector2Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedVector3Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedVector4Array)
// DECLARE_BLACKBOARD_DATA_TYPE(PackedColorArray)
//
// DECLARE_BLACKBOARD_DATA_TYPE(Variant)
//
// DECLARE_BLACKBOARD_OBJECT_TYPE(Object)
// DECLARE_BLACKBOARD_REFERENCE_TYPE(RefCounted)
//
// #define REGISTER_BLACKBOARD_DATA_TYPE(data_type)				\
// 	hydrogen::Blackboard::register_storage_type<data_type>();
//
// #define REGISTER_BLACKBOARD_OBJECT_TYPE(obj_type)					\
// 	hydrogen::Blackboard::register_storage_type<obj_type*>();		\
// 	hydrogen::Blackboard::register_storage_type<const obj_type*>();	\
//
// #define REGISTER_BLACKBOARD_REFERENCE_TYPE(ref_type)						\
// 	hydrogen::Blackboard::register_storage_type<Ref<ref_type>>();			\
// 	hydrogen::Blackboard::register_storage_type<const Ref<ref_type> &>();	\

}

#endif //BLACKBOARD_STORAGE_TYPE_HPP
