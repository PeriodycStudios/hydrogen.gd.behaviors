#define DOCTEST_CONFIG_IMPLEMENT_WITH_DLL
#include "behavior_server.hpp"

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

// TODO: Multi-Threaded tests
// TODO: Switch to using BehaviorServer API

template<typename T>
void test_simple_get_set(BehaviorServer *server, RID blackboard, const StringName &p_name, T p_value) {
	server->blackboard_set_entry_fast(blackboard, p_name, p_value);
	CHECK(server->blackboard_has_entry(blackboard, p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(server->blackboard_try_get_entry<T>(blackboard, p_name, result));
		CHECK(result == p_value);
	}

	CHECK(server->blackboard_get_entry_fast<T>(blackboard, p_name) == p_value);
	server->blackboard_erase_entry(blackboard, p_name);
	CHECK_FALSE(server->blackboard_has_entry(blackboard, p_name));

	if constexpr (
		!traits::is_exactly_gd_object_v<T> &&
		traits::is_variant_type_v<T> &&
		std::is_pointer_v<T> &&
		traits::is_gd_object_type_v<std::remove_pointer_t<T>>
		) {

		typedef traits::resolve_object_ptr_type_t<T> obj_ptr_t;
		typedef std::remove_pointer_t<obj_ptr_t> obj_without_ptr;
		typedef std::remove_pointer_t<T> without_ptr;

		obj_ptr_t obj = Object::cast_to<obj_without_ptr>(p_value);
		Variant v1 = obj;
		server->blackboard_set_entry(blackboard, p_name, v1);
		CHECK(server->blackboard_has_entry(blackboard, p_name));

		Variant v2 = server->blackboard_get_entry<Variant>(blackboard, p_name);
		obj_ptr_t obj2 = v2;
		T result = Object::cast_to<without_ptr>(obj);
		CHECK(result == p_value);
	}
	else if constexpr (traits::is_variant_type_v<T>) {
		Variant v1 = p_value;
		server->blackboard_set_entry(blackboard, p_name, v1);
		CHECK(server->blackboard_has_entry(blackboard, p_name));

		Variant v2 = server->blackboard_get_entry<Variant>(blackboard, p_name);
		T result = v2;
		CHECK(result == p_value);
	}

	server->blackboard_set_entry(blackboard, p_name, p_value);
	CHECK(server->blackboard_has_entry(blackboard, p_name));
	server->blackboard_set_entry(blackboard, p_name, Variant());
	CHECK_FALSE(server->blackboard_has_entry(blackboard, p_name));
}

template<typename T, typename U>
void test_convertable_get_set(BehaviorServer *server, RID blackboard, const StringName &p_name, T p_value) {
	static_assert(traits::is_variant_type_v<U>, "U must have a supported Variant conversion.");

	server->blackboard_set_entry_fast(blackboard, p_name, p_value);
	CHECK(server->blackboard_has_entry(blackboard, p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(server->blackboard_try_get_entry<T>(blackboard, p_name, result));
		CHECK(result == p_value);
	}

	CHECK(server->blackboard_get_entry_fast<T>(blackboard, p_name) == p_value);
	server->blackboard_erase_entry(blackboard, p_name);
	CHECK_FALSE(server->blackboard_has_entry(blackboard, p_name));

	U conversion = p_value;
	Variant v1 = conversion;
	server->blackboard_set_entry(blackboard, p_name, v1);

	CHECK(server->blackboard_has_entry(blackboard, p_name));

	Variant v2 = server->blackboard_get_entry<Variant>(blackboard, p_name);
	U conversion_result = v2;
	T result = conversion_result;
	CHECK(result == p_value);

	server->blackboard_erase_entry(blackboard, p_name);
	CHECK_FALSE(server->blackboard_has_entry(blackboard, p_name));
}

template <typename T, typename U>
void test_overwrite_get_set(BehaviorServer *server, RID blackboard, const StringName &p_name, const T &p_value_1, const T &p_value_2, const U &p_value_3) {

	server->blackboard_set_entry_fast(blackboard, p_name, p_value_1);
	CHECK(server->blackboard_has_entry(blackboard, p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(server->blackboard_try_get_entry<T>(blackboard, p_name, result));
		CHECK(result == p_value_1);
	}

	CHECK(server->blackboard_get_entry_fast<T>(blackboard, p_name) == p_value_1);
	server->blackboard_set_entry(blackboard, p_name, p_value_2);
	CHECK(server->blackboard_has_entry(blackboard, p_name));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK(server->blackboard_try_get_entry<T>(blackboard, p_name, result));
		CHECK(result == p_value_2);
	}

	CHECK(server->blackboard_get_entry<T>(blackboard, p_name) == p_value_2);

	server->blackboard_set_entry(blackboard, p_name, p_value_3);
	CHECK(server->blackboard_has_entry(blackboard, p_name));

	if constexpr (!std::is_const_v<U>) {
		U result;
		CHECK(server->blackboard_try_get_entry<U>(blackboard, p_name, result));
		CHECK(result == p_value_3);
	}

	CHECK(server->blackboard_get_entry<U>(blackboard, p_name) == p_value_3);

	Variant v1 = Variant();

	server->blackboard_set_entry(blackboard, p_name, v1);
	CHECK(!server->blackboard_has_entry(blackboard, p_name));
}

template <typename T>
void test_default_get(BehaviorServer *server, RID blackboard, const StringName &p_name, const T &p_value) {
	CHECK(server->blackboard_get_entry<T>(blackboard, p_name, p_value) == p_value);

	if constexpr (std::is_default_constructible_v<T>) {
		const T default_result = {};
		CHECK(server->blackboard_get_entry<T>(blackboard, p_name) == default_result);
	}

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK_FALSE(server->blackboard_try_get_entry(blackboard, p_name, result));
	}

	CHECK_FALSE(server->blackboard_has_entry(blackboard, p_name));
}

template <typename T>
void test_blackboard_get_ignore_parents(BehaviorServer* server, RID blackboard, const StringName &p_name, const T &p_value) {
	CHECK_FALSE(server->blackboard_has_entry(blackboard, p_name, false));

	if constexpr (!std::is_const_v<T>) {
		T result;
		CHECK_FALSE(server->blackboard_try_get_entry<T>(blackboard, p_name, result, false));
	}

	CHECK(server->blackboard_get_entry<T>(blackboard, p_name, p_value, false) == p_value);
}

#define TEST_SIMPLE_GET_SET(type, value) test_simple_get_set<type>(server, blackboard, #type, value);

#define TEST_CONVERTABLE_GET_SET(type, convert_type, value) test_convertable_get_set<type, convert_type>(server, blackboard, #type, value);

#define TEST_OVERWRITE_GET_SET(type, value1, value2, value3) test_overwrite_get_set(server, blackboard, #type, value1, value2, value3);

#define TEST_DEFAULT_GET(type, value) test_default_get<type>(server, blackboard, #type, value);

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Simple Get and Set") {
	BehaviorServer *server = BehaviorServer::get_singleton();
	REQUIRE(server);

	RID blackboard = server->blackboard_create();
	REQUIRE(blackboard != RID());

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

	server->free_rid(blackboard);
	memdelete(node);
	ref_obj.unref();
	json.unref();
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Set Overwrite") {
	BehaviorServer *server = BehaviorServer::get_singleton();
	REQUIRE(server);

	RID blackboard = server->blackboard_create();
	REQUIRE(blackboard != RID());

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

	server->free_rid(blackboard);
	memdelete(node_3d);
	memdelete(node2);
	memdelete(node);
	json2.unref();
	json.unref();
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Get default values") {
	BehaviorServer* server = BehaviorServer::get_singleton();
	REQUIRE(server != nullptr);

	RID blackboard = server->blackboard_create();
	REQUIRE(blackboard != RID());

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

	server->free_rid(blackboard);
	memdelete(node);
	json.unref();
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Parents") {
	BehaviorServer* server = BehaviorServer::get_singleton();
	REQUIRE(server != nullptr);

	RID blackboard = server->blackboard_create();
	REQUIRE(blackboard != RID());

	server->blackboard_set_entry(blackboard, "First", 255ul);

	RID blackboard2 = server->blackboard_create();
	REQUIRE(blackboard2 != RID());
	server->blackboard_set_entry(blackboard2, "Second", String("42"));

	RID blackboard3 = server->blackboard_create();
	REQUIRE(blackboard3 != RID());
	Array array = Array();
	array.push_back(255ul);
	array.push_back(42ul);
	server->blackboard_set_entry(blackboard3, "Third", array);

	RID blackboard4 = server->blackboard_create();
	REQUIRE(blackboard4 != RID());

	Ref<JSON> json = {};
	json.instantiate();
	server->blackboard_set_entry(blackboard4, "Fourth", json);

	CHECK(server->blackboard_set_parent(blackboard, blackboard2));
	CHECK(server->blackboard_set_parent(blackboard2, blackboard3));
	CHECK(server->blackboard_set_parent(blackboard3, blackboard4));
	CHECK_FALSE(server->blackboard_set_parent(blackboard4, blackboard));

	CHECK(server->blackboard_get_parent(blackboard) == blackboard2);
	CHECK(server->blackboard_get_parent(blackboard2) == blackboard3);
	CHECK(server->blackboard_get_parent(blackboard3) == blackboard4);
	CHECK(server->blackboard_get_parent(blackboard4) == RID());

	CHECK(server->blackboard_get_entry<String>(blackboard, "Second") == String("42"));
	CHECK(server->blackboard_get_entry<Array>(blackboard,"Third") == array);
	CHECK(server->blackboard_get_entry<Variant>(blackboard, "Fourth") == json);

	test_blackboard_get_ignore_parents(server, blackboard, "Second", String("FooBarBazQux"));
	test_blackboard_get_ignore_parents(server, blackboard,"Third", 512);
	test_blackboard_get_ignore_parents(server, blackboard,"Fourth", 42.0);

	server->free_rid(blackboard3);

	CHECK_FALSE(server->blackboard_is_ancestor(blackboard, blackboard4));
	CHECK_FALSE(server->blackboard_is_ancestor(blackboard2, blackboard4));
	CHECK(server->blackboard_get_parent(blackboard2) == RID());

	server->free_rid(blackboard);
	server->free_rid(blackboard2);
	server->free_rid(blackboard4);

	json.unref();
}

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Export Entries") {
	BehaviorServer* server = BehaviorServer::get_singleton();
	REQUIRE(server != nullptr);

	RID blackboard = server->blackboard_create();
	REQUIRE(blackboard != RID());

	Dictionary entries = server->blackboard_export_entries(blackboard);
	CHECK(entries.size() == 0);

	Node *node = memnew(Node);
	Ref json = memnew(JSON);

	server->blackboard_set_entry(blackboard, "First", 255ul);
	server->blackboard_set_entry(blackboard, "Second", String("42"));
	server->blackboard_set_entry(blackboard, "Third", 512);
	server->blackboard_set_entry(blackboard, "Fourth", json);
	server->blackboard_set_entry(blackboard, "Fifth", node);
	entries = server->blackboard_export_entries(blackboard);
	CHECK(entries.size() == 5);

	uint64_t first = entries["First"];
	int64_t third = entries["Third"];
	Object* fifth = entries["Fifth"];

	CHECK(first == 255ul);
	CHECK(entries["Second"] == String("42"));
	CHECK(third == 512);
	CHECK(entries["Fourth"] == json);
	CHECK(fifth == node);

	server->free_rid(blackboard);
	memdelete(node);
	json.unref();
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

TEST_CASE("[Hydrogen][Behaviors][Blackboard] Set From Dictionary") {
	Blackboard::register_type<Ref<JSON>>();
	Blackboard::register_type<Ref<RefCounted>>();
	Blackboard::register_type<Node*>();
	Blackboard::register_type<const Node*>();

	BehaviorServer* server = BehaviorServer::get_singleton();
	REQUIRE(server != nullptr);

	RID blackboard = server->blackboard_create();
	REQUIRE(blackboard != RID());

	Dictionary dict = Dictionary();
	dict.set_typed(Variant::STRING_NAME, "", {}, Variant::NIL, "", {});

	REQUIRE(dict.get_typed_key_builtin() == Variant::STRING_NAME);
	REQUIRE(dict.get_typed_value_builtin() == Variant::NIL);

	Node *node = memnew(Node);
	REQUIRE(node != nullptr);

	Ref json = memnew(JSON);
	REQUIRE(json.is_valid());

	StringName first = "First";
	StringName second = "Second";
	StringName third = "Third";
	StringName fourth = "Fourth";
	StringName fifth = "Fifth";

	dict[first] = 255ul;
	dict[second] = String("42");
	dict[third] = -512;
	dict[fourth] = json;
	dict[fifth] = node;

	server->blackboard_set_from_dictionary(blackboard, dict);

	// CHECK(server->blackboard_get_entry<int64_t>(blackboard, first) == 255ul);
	// CHECK(server->blackboard_get_entry<String>(blackboard, second) == String("42"));
	// CHECK(server->blackboard_get_entry<int64_t>(blackboard, third) == -512);
	// CHECK(server->blackboard_get_entry<Ref<JSON>>(blackboard, fourth) == json);
	CHECK(server->blackboard_get_entry<Node*>(blackboard, fifth) == node);

	server->free_rid(blackboard);
	memdelete(node);
	json.unref();
}

// TODO: Create / Get / Set from multiple threads
// TODO: Spread Heirarchy across multiple threads

} //namespace hydrogen::test
