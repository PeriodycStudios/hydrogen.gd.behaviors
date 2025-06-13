#include "register_types.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/engine.hpp>

#include "behavior_server.hpp"
#include "blackboard.hpp"
#include "variant_type_traits.hpp"

#ifdef TESTS_ENABLED
#include "tests.hpp"
#endif

using namespace godot;
using namespace hydrogen;

static BehaviorServer *behavior_server = nullptr;
static _BehaviorServer *_behavior_server = nullptr;

const String k_server_name = "HydrogenBehaviorServer";

#define CLEAN_MEM_DELETE(x) \
	memdelete(x);			\
	(x) = nullptr;			\

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// if (traits::can_construct_from_variant<Ref<RefCounted>>::value) {
	// 	std::cout << "can construct ref refcounted" << std::endl;
	// }
	//
	// if (traits::variant_has_operator<bool>::value) {
	// 	std::cout << "has operator<bool" << std::endl;
	// }
	//
	// if (traits::has_variant_operator<Ref<RefCounted>>::value) {
	// 	std::cout << "has_variant_operator<Ref<RefCounted>" << std::endl;
	// }

	//
	// REGISTER_BLACKBOARD_DATA_TYPE(bool)
	// REGISTER_BLACKBOARD_DATA_TYPE(uint8_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(int8_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(uint16_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(int16_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(uint32_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(int32_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(uint64_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(int64_t)
	//
	// REGISTER_BLACKBOARD_DATA_TYPE(char16_t)
	// REGISTER_BLACKBOARD_DATA_TYPE(char32_t)
	//
	// REGISTER_BLACKBOARD_DATA_TYPE(float)
	// REGISTER_BLACKBOARD_DATA_TYPE(double)
	//
	// REGISTER_BLACKBOARD_DATA_TYPE(String)
	// REGISTER_BLACKBOARD_DATA_TYPE(Vector2)
	// REGISTER_BLACKBOARD_DATA_TYPE(Vector2i)
	// REGISTER_BLACKBOARD_DATA_TYPE(Rect2)
	// REGISTER_BLACKBOARD_DATA_TYPE(Rect2i)
	// REGISTER_BLACKBOARD_DATA_TYPE(Vector3)
	// REGISTER_BLACKBOARD_DATA_TYPE(Vector3i)
	// REGISTER_BLACKBOARD_DATA_TYPE(Transform2D)
	// REGISTER_BLACKBOARD_DATA_TYPE(Vector4)
	// REGISTER_BLACKBOARD_DATA_TYPE(Vector4i)
	// REGISTER_BLACKBOARD_DATA_TYPE(Plane)
	// REGISTER_BLACKBOARD_DATA_TYPE(Quaternion)
	// REGISTER_BLACKBOARD_DATA_TYPE(AABB)
	// REGISTER_BLACKBOARD_DATA_TYPE(Basis)
	// REGISTER_BLACKBOARD_DATA_TYPE(Transform3D)
	// REGISTER_BLACKBOARD_DATA_TYPE(Projection)
	// REGISTER_BLACKBOARD_DATA_TYPE(Color)
	// REGISTER_BLACKBOARD_DATA_TYPE(StringName)
	// REGISTER_BLACKBOARD_DATA_TYPE(NodePath)
	// REGISTER_BLACKBOARD_DATA_TYPE(RID)
	// REGISTER_BLACKBOARD_DATA_TYPE(ObjectID)
	// REGISTER_BLACKBOARD_DATA_TYPE(Callable)
	// REGISTER_BLACKBOARD_DATA_TYPE(Signal)
	// REGISTER_BLACKBOARD_DATA_TYPE(Dictionary)
	// REGISTER_BLACKBOARD_DATA_TYPE(Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedByteArray)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedInt32Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedInt64Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedFloat32Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedFloat64Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedStringArray)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedVector2Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedVector3Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedVector4Array)
	// REGISTER_BLACKBOARD_DATA_TYPE(PackedColorArray)
	//
	// REGISTER_BLACKBOARD_DATA_TYPE(Variant)
	//
	// REGISTER_BLACKBOARD_OBJECT_TYPE(Object)
	// REGISTER_BLACKBOARD_REFERENCE_TYPE(RefCounted)

	// behavior_server = memnew(BehaviorServer);
	// behavior_server->init();
	//
	// _behavior_server = memnew(_BehaviorServer);

	// GDREGISTER_CLASS(_BehaviorServer);

	// Engine::get_singleton()->register_singleton(k_server_name, _BehaviorServer::get_singleton());

}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Engine::get_singleton()->unregister_singleton(k_server_name);

	// if (behavior_server) {
	// 	behavior_server->finish();
	// 	CLEAN_MEM_DELETE(behavior_server);
	// }
	//
	// if (_behavior_server) {
	// 	CLEAN_MEM_DELETE(_behavior_server);
	// }
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT hydrogen_gd_behaviors_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}