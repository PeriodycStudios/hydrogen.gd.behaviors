#define DOCTEST_CONFIG_IMPLEMENT_WITH_DLL
#include <doctest.h>

#include "blackboard.hpp"
#include "godot_cpp/classes/json.hpp"

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

	T result;
	CHECK(blackboard->try_get_entry<T>(p_name, result));
	CHECK(result == p_value);
	CHECK(blackboard->get_entry<T>(p_name) == p_value);
}

#define TEST_SIMPLE_GET_SET(type, value) test_simple_get_set<type>(blackboard, #type, value);

TEST_CASE("[Blackboard] Simple Set and Get") {

	Blackboard *blackboard = memnew(Blackboard("TestBlackboard"));
	REQUIRE(blackboard != nullptr);

	TEST_SIMPLE_GET_SET(bool, true)
	TEST_SIMPLE_GET_SET(int8_t, std::numeric_limits<int8_t>::min())
	TEST_SIMPLE_GET_SET(uint8_t, std::numeric_limits<uint8_t>::max())
	TEST_SIMPLE_GET_SET(int16_t, std::numeric_limits<int16_t>::min())
	TEST_SIMPLE_GET_SET(uint16_t, std::numeric_limits<uint16_t>::max())
	TEST_SIMPLE_GET_SET(int32_t, std::numeric_limits<int32_t>::min())
	TEST_SIMPLE_GET_SET(uint32_t, std::numeric_limits<uint32_t>::max())
	TEST_SIMPLE_GET_SET(int64_t, std::numeric_limits<int64_t>::min())
	TEST_SIMPLE_GET_SET(uint64_t, std::numeric_limits<uint64_t>::max())

	// TEST_SIMPLE_GET_SET(char16_t, std::numeric_limits<char>::min())
	// TEST_SIMPLE_GET_SET(char32_t, std::numeric_limits<char>::min())

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

	print_line("Is json Null?: ", json.is_null());

	Node *node = memnew(Node);
	Object *obj = node;
	const Object *obj_const = obj;

	TEST_SIMPLE_GET_SET(RID, rid)
	TEST_SIMPLE_GET_SET(Object*, obj)
	// TEST_SIMPLE_GET_SET(const Object*, obj_const)
	Callable callable = Callable::create(json, "get_data");
	TEST_SIMPLE_GET_SET(Callable, callable)

	Signal signal = Signal();
	signal.connect(callable);
	TEST_SIMPLE_GET_SET(Signal, signal)

	Dictionary dict = Dictionary();
	dict["A"] = 42.0;
	dict["B"] = 37;
	// dict["C"] = callable;
	// dict["S"] = signal;

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

	// ObjectID obj_id = ObjectID(node->get_instance_id());
	// TEST_SIMPLE_GET_SET(ObjectID, obj_id)

	Ref<RefCounted> ref_obj = json;
	TEST_SIMPLE_GET_SET(Ref<RefCounted>, ref_obj)

	Blackboard::register_type<Ref<JSON>>();
	TEST_SIMPLE_GET_SET(Ref<JSON>, json)
	// Blackboard::register_type<Node*>();
	// TEST_SIMPLE_GET_SET(Node*, node)

	signal.disconnect(callable);
	memdelete(blackboard);
	memdelete(node);
}

} //namespace hydrogen::test
