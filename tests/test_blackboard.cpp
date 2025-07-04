#define DOCTEST_CONFIG_IMPLEMENT_WITH_DLL
#include <doctest.h>

#include "blackboard.hpp"
#include "godot_cpp/classes/json.hpp"
#include "godot_cpp/classes/node3d.hpp"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace hydrogen::test {
using namespace godot;

template<typename T>
void test_simple_get_set(Blackboard *blackboard, const StringName &p_name, T p_value) {
	blackboard->set_entry(p_name, p_value);
	CHECK(blackboard->has_entry(p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(blackboard->try_get_entry<T>(p_name, result));
		CHECK(result == p_value);
	}

	CHECK(blackboard->get_entry<T>(p_name) == p_value);

	if constexpr (
		!traits::is_exactly_gd_object_v<T> &&
		traits::is_variant_type_v<T> &&
		std::is_pointer_v<T> &&
		traits::is_gd_object_type_v<std::remove_pointer_t<T>>
		) {

		blackboard->erase_entry(p_name);
		CHECK_FALSE(blackboard->has_entry(p_name));

		typedef traits::resolve_object_ptr_type_t<T> obj_ptr_t;
		typedef std::remove_pointer_t<obj_ptr_t> obj_without_ptr;
		typedef std::remove_pointer_t<T> without_ptr;

		obj_ptr_t obj = Object::cast_to<obj_without_ptr>(p_value);
		Variant v1 = obj;
		blackboard->set_entry(p_name, v1);
		CHECK(blackboard->has_entry(p_name));

		Variant v2 = blackboard->get_entry<Variant>(p_name);
		obj_ptr_t obj2 = v2;
		T result = Object::cast_to<without_ptr>(obj);
		CHECK(result == p_value);
	}
	else if constexpr (traits::is_variant_type_v<T>) {
		blackboard->erase_entry(p_name);
		CHECK_FALSE(blackboard->has_entry(p_name));

		Variant v1 = p_value;
		blackboard->set_entry(p_name, v1);
		CHECK(blackboard->has_entry(p_name));

		Variant v2 = blackboard->get_entry<Variant>(p_name);
		T result = v2;
		CHECK(result == p_value);
	}
	else {
		blackboard->erase_entry(p_name);
		CHECK_FALSE(blackboard->has_entry(p_name));
	}

	blackboard->set_entry(p_name, p_value);
	CHECK(blackboard->has_entry(p_name));
	blackboard->set_entry(p_name, Variant());
	CHECK(!blackboard->has_entry(p_name));
}

template<typename T, typename U>
void test_convertable_get_set(Blackboard *blackboard, const StringName &p_name, T p_value) {
	static_assert(traits::is_variant_type_v<U>, "U must have a supported Variant conversion.");

	blackboard->set_entry(p_name, p_value);
	CHECK(blackboard->has_entry(p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(blackboard->try_get_entry<T>(p_name, result));
		CHECK(result == p_value);
	}

	CHECK(blackboard->get_entry<T>(p_name) == p_value);
	blackboard->erase_entry(p_name);
	CHECK_FALSE(blackboard->has_entry(p_name));

	U conversion = p_value;
	Variant v1 = conversion;
	blackboard->set_entry(p_name, v1);

	CHECK(blackboard->has_entry(p_name));

	Variant v2 = blackboard->get_entry<Variant>(p_name);
	U conversion_result = v2;
	T result = conversion_result;
	CHECK(result == p_value);

	blackboard->erase_entry(p_name);
	CHECK_FALSE(blackboard->has_entry(p_name));
}

template <typename T, typename U>
void test_overwrite_get_set(Blackboard *blackboard, const StringName &p_name, const T &p_value_1, const T &p_value_2, const U &p_value_3) {

	blackboard->set_entry(p_name, p_value_1);
	CHECK(blackboard->has_entry(p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(blackboard->try_get_entry<T>(p_name, result));
		CHECK(result == p_value_1);
	}

	CHECK(blackboard->get_entry<T>(p_name) == p_value_1);
	blackboard->set_entry(p_name, p_value_2);
	CHECK(blackboard->has_entry(p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(blackboard->try_get_entry<T>(p_name, result));
		CHECK(result == p_value_2);
	}

	CHECK(blackboard->get_entry<T>(p_name) == p_value_2);

	blackboard->set_entry(p_name, p_value_3);
	CHECK(blackboard->has_entry(p_name));

	if constexpr (!std::is_const_v<U>) {
		U result;
		CHECK(blackboard->try_get_entry<U>(p_name, result));
		CHECK(result == p_value_3);
	}

	CHECK(blackboard->get_entry<U>(p_name) == p_value_3);

	Variant v1 = Variant();

	blackboard->set_entry(p_name, v1);
	CHECK(!blackboard->has_entry(p_name));
}

template <typename T>
void test_default_get(Blackboard *blackboard, const StringName &p_name, const T &p_value) {
	CHECK(blackboard->get_entry<T>(p_name, p_value) == p_value);

	if constexpr (std::is_default_constructible_v<T>) {
		const T default_result = {};
		CHECK(blackboard->get_entry<T>(p_name) == default_result);
	}

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK_FALSE(blackboard->try_get_entry(p_name, result));
	}

	CHECK_FALSE(blackboard->has_entry(p_name));
}

template <typename T>
void test_blackboard_get_ignore_parents(Blackboard *blackboard, const StringName &p_name, const T &p_value) {
	CHECK_FALSE(blackboard->has_entry(p_name, false));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK_FALSE(blackboard->try_get_entry<T>(p_name, result, false));
	}

	CHECK(blackboard->get_entry<T>(p_name, p_value, false) == p_value);
}

#define TEST_SIMPLE_GET_SET(type, value) test_simple_get_set<type>(blackboard, #type, value);

#define TEST_CONVERTABLE_GET_SET(type, convert_type, value) test_convertable_get_set<type, convert_type>(blackboard, #type, value);

#define TEST_OVERWRITE_GET_SET(type, value1, value2, value3) test_overwrite_get_set(blackboard, #type, value1, value2, value3);

#define TEST_DEFAULT_GET(type, value) test_default_get<type>(blackboard, #type, value);

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Simple Get and Set") {

	Blackboard* blackboard = memnew(Blackboard);

	TEST_SIMPLE_GET_SET(bool, true)
	TEST_SIMPLE_GET_SET(const bool, false)
	TEST_SIMPLE_GET_SET(int8_t, std::numeric_limits<int8_t>::min())
	TEST_SIMPLE_GET_SET(const int8_t, std::numeric_limits<int8_t>::max())
	TEST_SIMPLE_GET_SET(uint8_t, std::numeric_limits<uint8_t>::max())
	TEST_SIMPLE_GET_SET(int16_t, std::numeric_limits<int16_t>::min())
	TEST_SIMPLE_GET_SET(uint16_t, std::numeric_limits<uint16_t>::max())
	TEST_SIMPLE_GET_SET(int32_t, std::numeric_limits<int32_t>::min())
	TEST_SIMPLE_GET_SET(uint32_t, std::numeric_limits<uint32_t>::max())
	TEST_SIMPLE_GET_SET(int64_t, std::numeric_limits<int64_t>::min())
	TEST_SIMPLE_GET_SET(uint64_t, std::numeric_limits<uint64_t>::max())

	TEST_CONVERTABLE_GET_SET(char16_t, int64_t, std::numeric_limits<char>::min())
	TEST_CONVERTABLE_GET_SET(char32_t, int64_t, std::numeric_limits<char>::min())

	TEST_SIMPLE_GET_SET(float, std::numeric_limits<float>::max())
	TEST_SIMPLE_GET_SET(double, std::numeric_limits<double>::max())

	TEST_SIMPLE_GET_SET(String, "Behold, a godot::String!")
	TEST_SIMPLE_GET_SET(Vector2, Vector2(1.0f, -1.0f))
	TEST_SIMPLE_GET_SET(Vector2i, Vector2i(1, -1))
	TEST_SIMPLE_GET_SET(Rect2, Rect2(1.0f, 2.0f, 3.0f, 4.0f))
	TEST_SIMPLE_GET_SET(Rect2i, Rect2i(1, 2, 3, 4))
	TEST_SIMPLE_GET_SET(Vector3, Vector3(1.0f, -1.0f, -1.0f))
	TEST_SIMPLE_GET_SET(Vector3i, Vector3i(1, 2, 3))
	TEST_SIMPLE_GET_SET(Transform2D, Transform2D(Math_PI, Vector2(1.0f, 2.0f)))
	TEST_SIMPLE_GET_SET(Vector4, Vector4(1.0f, -1.0f, -1.0f, 2.0f))
	TEST_SIMPLE_GET_SET(Vector4i, Vector4i(1, 2, 3, 4))
	TEST_SIMPLE_GET_SET(Plane, Plane(Vector3(0.0f, 1.0f, 0.0), Vector3(-1.0f, 2.0f, -3.0)))
	TEST_SIMPLE_GET_SET(AABB, AABB(Vector3(2.0f, 1.0f, 5.0), Vector3(1.0f, 2.0f, 3.0)))
	TEST_SIMPLE_GET_SET(Basis, Basis())
	TEST_SIMPLE_GET_SET(Quaternion, Quaternion::from_euler(Vector3(0.0f, Math_PI / 2.0f, 0.0f)))
	TEST_SIMPLE_GET_SET(Transform3D, Transform3D(Basis(), Vector3(1.0f, 2.0f, 3.0f)))
	TEST_SIMPLE_GET_SET(Projection, Projection::create_depth_correction(false))
	TEST_SIMPLE_GET_SET(Color, Color::named("RED"))
	TEST_SIMPLE_GET_SET(StringName, Object::get_class_static())
	TEST_SIMPLE_GET_SET(NodePath, NodePath("../Sibling:color:r"))

	Ref<JSON> json = Ref(memnew(JSON));
	json->parse("{\"alpha\": 1}");
	RID rid = json->get_rid();

	Node *node = memnew(Node);
	Object *obj = node;
	const Object *obj_const = obj;

	TEST_SIMPLE_GET_SET(RID, rid)
	TEST_SIMPLE_GET_SET(Object*, obj)
	TEST_SIMPLE_GET_SET(const Object*, obj_const)
	Callable callable = Callable();
	TEST_SIMPLE_GET_SET(Callable, callable)

	Signal signal = Signal();

	TEST_SIMPLE_GET_SET(Signal, signal)

	Dictionary dict = Dictionary();
	dict["A"] = 42.0;
	dict["B"] = 37;
	dict["C"] = callable;
	dict["S"] = signal;

	TEST_SIMPLE_GET_SET(Dictionary, dict)

	Array array = Array();
	array.push_back(dict["A"]);
	array.push_back(dict["B"]);
	array.push_back(dict["C"]);
	array.push_back(dict["S"]);

	TEST_SIMPLE_GET_SET(Array, array)

	PackedByteArray pbarray = PackedByteArray();
	pbarray.push_back(42);
	pbarray.push_back(37);
	pbarray.push_back(255);
	TEST_SIMPLE_GET_SET(PackedByteArray, pbarray)

	PackedInt32Array pi32array = PackedInt32Array();
	pi32array.push_back(42);
	pi32array.push_back(37);
	pi32array.push_back(255);
	pi32array.push_back(-1000);
	TEST_SIMPLE_GET_SET(PackedInt32Array, pi32array)

	PackedInt64Array pi64array = PackedInt64Array();
	pi64array.push_back(42);
	pi64array.push_back(37);
	pi64array.push_back(-1000);
	pi64array.push_back(5500055);
	TEST_SIMPLE_GET_SET(PackedInt64Array, pi64array)

	PackedFloat32Array pf32array = PackedFloat32Array();
	pf32array.push_back(42.0f);
	pf32array.push_back(37.0f);
	pf32array.push_back(-1000.0f);
	pf32array.push_back(Math_PI);
	TEST_SIMPLE_GET_SET(PackedFloat32Array, pf32array)

	PackedFloat64Array pf64array = PackedFloat64Array();
	pf64array.push_back(42.0);
	pf64array.push_back(37.0);
	pf64array.push_back(-1000.0);
	pf64array.push_back(Math_PI);
	pf64array.push_back(-5500055.0);
	TEST_SIMPLE_GET_SET(PackedFloat64Array, pf64array)

	PackedStringArray psarray = PackedStringArray();
	psarray.push_back("Hello");
	psarray.push_back("World");
	psarray.push_back("Alice");
	psarray.push_back("Bob");
	TEST_SIMPLE_GET_SET(PackedStringArray, psarray)

	PackedVector2Array pv2array = PackedVector2Array();
	pv2array.push_back(Vector2(1.0f, 2.0f));
	pv2array.push_back(Vector2(3.0f, 4.0f));
	pv2array.push_back(Vector2(5.0f, 6.0f));
	TEST_SIMPLE_GET_SET(PackedVector2Array, pv2array)

	PackedVector3Array pv3array = PackedVector3Array();
	pv3array.push_back(Vector3(1.0f, 2.0f, 3.0f));
	pv3array.push_back(Vector3(4.0f, 5.0f, 6.0f));
	pv3array.push_back(Vector3(7.0f, 8.0f, 9.0f));
	TEST_SIMPLE_GET_SET(PackedVector3Array, pv3array)

	PackedVector4Array pv4array = PackedVector4Array();
	pv4array.push_back(Vector4(1.0f, 2.0f, 3.0f, 4.0f));
	pv4array.push_back(Vector4(5.0f, 6.0f, 7.0f, 8.0f));
	pv4array.push_back(Vector4(9.0f, 10.0f, 11.0f, 12.0f));
	pv4array.push_back(Vector4(12.0f, 13.0f, 14.0f, 15.0f));
	TEST_SIMPLE_GET_SET(PackedVector4Array, pv4array)

	PackedColorArray pcol_array = PackedColorArray();
	pcol_array.push_back(Color::named("RED"));
	pcol_array.push_back(Color::named("GREEN"));
	pcol_array.push_back(Color::named("BLUE"));
	pcol_array.push_back(Color::named("CHARTREUSE"));
	pcol_array.push_back(Color::named("MINT_CREAM"));
	TEST_SIMPLE_GET_SET(PackedColorArray, pcol_array)

	ObjectID obj_id = ObjectID(node->get_instance_id());
	TEST_SIMPLE_GET_SET(ObjectID, obj_id)

	Ref<RefCounted> ref_obj = json;
	TEST_SIMPLE_GET_SET(Ref<RefCounted>, ref_obj)

	Blackboard::register_type<Ref<JSON>>();
	TEST_SIMPLE_GET_SET(Ref<JSON>, json)
	Blackboard::register_type<Node*>();
	TEST_SIMPLE_GET_SET(Node*, node)

	const Node* const_node = node;
	TEST_SIMPLE_GET_SET(const Node*, const_node)

	memdelete(blackboard);
	memdelete(node);
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Set Overwrite") {

	Blackboard* blackboard = memnew(Blackboard);
	REQUIRE(blackboard != nullptr);

	uint8_t a = 128;
	TEST_OVERWRITE_GET_SET(bool, true, false, a)

	int8_t b = -127;
	TEST_OVERWRITE_GET_SET(uint8_t, 42, 128, b)

	Dictionary dict = Dictionary();
	dict["A"] = 42.0;
	dict["B"] = 37;

	Dictionary dict2 = Dictionary();
	dict2[42] = 1.0 / 100.0;
	dict['a'] = Variant(5000);

	Array array = Array();
	array.push_back(dict["A"]);
	array.push_back(dict["B"]);
	array.push_back(dict["C"]);
	array.push_back(dict["S"]);

	TEST_OVERWRITE_GET_SET(Dictionary, dict, dict2, array)

	Node * node = memnew(Node);
	Node * node2 = memnew(Node);
	const Node3D * node_3d = memnew(Node3D);

	TEST_OVERWRITE_GET_SET(Node3D, node, node2, node_3d)

	Ref<JSON> json = {};
	json.instantiate();

	Ref json2 = memnew(JSON);

	TEST_OVERWRITE_GET_SET(Ref<JSON>, json, json2, node_3d)

	memdelete(blackboard);
	memdelete(node_3d);
	memdelete(node2);
	memdelete(node);
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Get default values") {

	Blackboard * blackboard = memnew(Blackboard);
	REQUIRE(blackboard != nullptr);

	TEST_DEFAULT_GET(bool, true)
	TEST_DEFAULT_GET(int8_t, std::numeric_limits<int8_t>::min())
	TEST_DEFAULT_GET(const int8_t, std::numeric_limits<int8_t>::max())
	TEST_DEFAULT_GET(uint8_t, std::numeric_limits<uint8_t>::max())
	TEST_DEFAULT_GET(int16_t, std::numeric_limits<int16_t>::min())
	TEST_DEFAULT_GET(uint16_t, std::numeric_limits<uint16_t>::max())
	TEST_DEFAULT_GET(int32_t, std::numeric_limits<int32_t>::min())
	TEST_DEFAULT_GET(uint32_t, std::numeric_limits<uint32_t>::max())
	TEST_DEFAULT_GET(int64_t, std::numeric_limits<int64_t>::min())
	TEST_DEFAULT_GET(uint64_t, std::numeric_limits<uint64_t>::max())

	TEST_DEFAULT_GET(char16_t, std::numeric_limits<char16_t>::min())
	TEST_DEFAULT_GET(char32_t, std::numeric_limits<char32_t>::min())

	TEST_DEFAULT_GET(float, std::numeric_limits<float>::max())
	TEST_DEFAULT_GET(double, std::numeric_limits<double>::max())

	TEST_DEFAULT_GET(String, "Behold, a godot::String!")
	TEST_DEFAULT_GET(Vector2, Vector2(1.0f, -1.0f))
	TEST_DEFAULT_GET(Vector2i, Vector2i(1, -1))
	TEST_DEFAULT_GET(Rect2, Rect2(1.0f, 2.0f, 3.0f, 4.0f))
	TEST_DEFAULT_GET(Rect2i, Rect2i(1, 2, 3, 4))
	TEST_DEFAULT_GET(Vector3, Vector3(1.0f, -1.0f, -1.0f))
	TEST_DEFAULT_GET(Vector3i, Vector3i(1, 2, 3))
	TEST_DEFAULT_GET(Transform2D, Transform2D(Math_PI, Vector2(1.0f, 2.0f)))
	TEST_DEFAULT_GET(Vector4, Vector4(1.0f, -1.0f, -1.0f, 2.0f))
	TEST_DEFAULT_GET(Vector4i, Vector4i(1, 2, 3, 4))
	TEST_DEFAULT_GET(Plane, Plane(Vector3(0.0f, 1.0f, 0.0), Vector3(-1.0f, 2.0f, -3.0)))
	TEST_DEFAULT_GET(AABB, AABB(Vector3(2.0f, 1.0f, 5.0), Vector3(1.0f, 2.0f, 3.0)))
	TEST_DEFAULT_GET(Basis, Basis())
	TEST_DEFAULT_GET(Quaternion, Quaternion::from_euler(Vector3(0.0f, Math_PI / 2.0f, 0.0f)))
	TEST_DEFAULT_GET(Transform3D, Transform3D(Basis(), Vector3(1.0f, 2.0f, 3.0f)))
	TEST_DEFAULT_GET(Projection, Projection::create_depth_correction(false))
	TEST_DEFAULT_GET(Color, Color::named("RED"))
	TEST_DEFAULT_GET(StringName, Object::get_class_static())
	TEST_DEFAULT_GET(NodePath, NodePath("../Sibling:color:r"))

	Ref<JSON> json = Ref(memnew(JSON));
	json->parse("{\"alpha\": 1}");
	RID rid = json->get_rid();

	Node *node = memnew(Node);
	Object *obj = node;
	const Object *obj_const = obj;

	TEST_DEFAULT_GET(RID, rid)
	TEST_DEFAULT_GET(Object*, obj)
	TEST_DEFAULT_GET(const Object*, obj_const)
	Callable callable = Callable();
	TEST_DEFAULT_GET(Callable, callable)

	Signal signal = Signal();

	TEST_DEFAULT_GET(Signal, signal)

	Dictionary dict = Dictionary();
	dict["A"] = 42.0;
	dict["B"] = 37;
	dict["C"] = callable;
	dict["S"] = signal;

	TEST_DEFAULT_GET(Dictionary, dict)

	Array array = Array();
	array.push_back(dict["A"]);
	array.push_back(dict["B"]);
	array.push_back(dict["C"]);
	array.push_back(dict["S"]);

	TEST_DEFAULT_GET(Array, array)

	PackedByteArray pbarray = PackedByteArray();
	pbarray.push_back(42);
	pbarray.push_back(37);
	pbarray.push_back(255);
	TEST_DEFAULT_GET(PackedByteArray, pbarray)

	PackedInt32Array pi32array = PackedInt32Array();
	pi32array.push_back(42);
	pi32array.push_back(37);
	pi32array.push_back(255);
	pi32array.push_back(-1000);
	TEST_DEFAULT_GET(PackedInt32Array, pi32array)

	PackedInt64Array pi64array = PackedInt64Array();
	pi64array.push_back(42);
	pi64array.push_back(37);
	pi64array.push_back(-1000);
	pi64array.push_back(5500055);
	TEST_DEFAULT_GET(PackedInt64Array, pi64array)

	PackedFloat32Array pf32array = PackedFloat32Array();
	pf32array.push_back(42.0f);
	pf32array.push_back(37.0f);
	pf32array.push_back(-1000.0f);
	pf32array.push_back(Math_PI);
	TEST_DEFAULT_GET(PackedFloat32Array, pf32array)

	PackedFloat64Array pf64array = PackedFloat64Array();
	pf64array.push_back(42.0);
	pf64array.push_back(37.0);
	pf64array.push_back(-1000.0);
	pf64array.push_back(Math_PI);
	pf64array.push_back(-5500055.0);
	TEST_DEFAULT_GET(PackedFloat64Array, pf64array)

	PackedStringArray psarray = PackedStringArray();
	psarray.push_back("Hello");
	psarray.push_back("World");
	psarray.push_back("Alice");
	psarray.push_back("Bob");
	TEST_DEFAULT_GET(PackedStringArray, psarray)

	PackedVector2Array pv2array = PackedVector2Array();
	pv2array.push_back(Vector2(1.0f, 2.0f));
	pv2array.push_back(Vector2(3.0f, 4.0f));
	pv2array.push_back(Vector2(5.0f, 6.0f));
	TEST_DEFAULT_GET(PackedVector2Array, pv2array)

	PackedVector3Array pv3array = PackedVector3Array();
	pv3array.push_back(Vector3(1.0f, 2.0f, 3.0f));
	pv3array.push_back(Vector3(4.0f, 5.0f, 6.0f));
	pv3array.push_back(Vector3(7.0f, 8.0f, 9.0f));
	TEST_DEFAULT_GET(PackedVector3Array, pv3array)

	PackedVector4Array pv4array = PackedVector4Array();
	pv4array.push_back(Vector4(1.0f, 2.0f, 3.0f, 4.0f));
	pv4array.push_back(Vector4(5.0f, 6.0f, 7.0f, 8.0f));
	pv4array.push_back(Vector4(9.0f, 10.0f, 11.0f, 12.0f));
	pv4array.push_back(Vector4(12.0f, 13.0f, 14.0f, 15.0f));
	TEST_DEFAULT_GET(PackedVector4Array, pv4array)

	PackedColorArray pcol_array = PackedColorArray();
	pcol_array.push_back(Color::named("RED"));
	pcol_array.push_back(Color::named("GREEN"));
	pcol_array.push_back(Color::named("BLUE"));
	pcol_array.push_back(Color::named("CHARTREUSE"));
	pcol_array.push_back(Color::named("MINT_CREAM"));
	TEST_DEFAULT_GET(PackedColorArray, pcol_array)

	ObjectID obj_id = ObjectID(node->get_instance_id());
	TEST_DEFAULT_GET(ObjectID, obj_id)

	Ref<RefCounted> ref_obj = json;
	TEST_DEFAULT_GET(Ref<RefCounted>, ref_obj)

	Blackboard::register_type<Ref<JSON>>();
	TEST_DEFAULT_GET(Ref<JSON>, json)
	Blackboard::register_type<Node*>();
	TEST_DEFAULT_GET(Node*, node)

	const Node* const_node = node;
	TEST_DEFAULT_GET(const Node*, const_node)


	memdelete(blackboard);
	memdelete(node);
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Parents") {

	Blackboard* blackboard = memnew(Blackboard);
	REQUIRE(blackboard != nullptr);

	blackboard->set_entry("First", 255ul);

	Blackboard* blackboard2 = memnew(Blackboard);
	REQUIRE(blackboard2 != nullptr);
	blackboard2->set_entry("Second", String("42"));

	Blackboard* blackboard3 = memnew(Blackboard);
	REQUIRE(blackboard3 != nullptr);
	Array array = Array();
	array.push_back(255ul);
	array.push_back(42ul);
	blackboard3->set_entry("Third", array);

	Blackboard* blackboard4 = memnew(Blackboard);
	REQUIRE(blackboard4 != nullptr);

	Ref<JSON> json = {};
	json.instantiate();
	blackboard4->set_entry("Fourth", json);

	CHECK(blackboard->set_parent(blackboard2));
	CHECK(blackboard2->set_parent(blackboard3));
	CHECK(blackboard3->set_parent(blackboard4));
	CHECK_FALSE(blackboard4->set_parent(blackboard));

	CHECK(blackboard->get_parent() == blackboard2);
	CHECK(blackboard2->get_parent() == blackboard3);
	CHECK(blackboard3->get_parent() == blackboard4);
	CHECK(blackboard4->get_parent() == nullptr);

	CHECK(blackboard->get_entry<String>("Second") == String("42"));
	CHECK(blackboard->get_entry<Array>("Third") == array);
	CHECK(blackboard->get_entry<Variant>("Fourth") == json);

	test_blackboard_get_ignore_parents(blackboard, "Second", String("FooBarBazQux"));
	test_blackboard_get_ignore_parents(blackboard, "Third", 512);
	test_blackboard_get_ignore_parents(blackboard, "Fourth", 42.0);

	memdelete(blackboard);
	memdelete(blackboard2);
	memdelete(blackboard3);
	memdelete(blackboard4);
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Export Entries") {
	Blackboard* blackboard = memnew(Blackboard);
	REQUIRE(blackboard != nullptr);

	Dictionary entries = blackboard->export_entries();
	CHECK(entries.size() == 0);

	Node *node = memnew(Node);
	Ref json = memnew(JSON);

	blackboard->set_entry("First", 255ul);
	blackboard->set_entry("Second", String("42"));
	blackboard->set_entry("Third", 512);
	blackboard->set_entry("Fourth", json);
	blackboard->set_entry("Fifth", node);
	entries = blackboard->export_entries();
	CHECK(entries.size() == 5);

	uint64_t first = entries["First"];
	int64_t third = entries["Third"];
	Object* fifth = entries["Fifth"];

	CHECK(first == 255ul);
	CHECK(entries["Second"] == String("42"));
	CHECK(third == 512);
	CHECK(entries["Fourth"] == json);
	CHECK(fifth == node);

	memdelete(blackboard);
	memdelete(node);
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Export Type Infos") {
	Dictionary type_infos = Blackboard::export_type_infos();
	REQUIRE(type_infos.size() > 0);

	const StringName ulong_name = "unsigned long";
	Variant unsigned_long = type_infos.get(ulong_name, Variant());
	REQUIRE(unsigned_long.get_type() != Variant::NIL);
	REQUIRE(unsigned_long.get("type_name") == ulong_name);
	int64_t hash_candidate = unsigned_long.get("type_key");
	CHECK(hash_candidate == ulong_name.hash());
}

} //namespace hydrogen::test
