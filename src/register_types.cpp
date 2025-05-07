#include "register_types.h"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/engine.hpp>

#include "behavior_server.hpp"
#include "blackboard.hpp"

using namespace godot;

static HydrogenBehaviorServer *behavior_server = NULL;
class HydrogenBlackboard;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

	behavior_server = memnew(HydrogenBehaviorServer);
	behavior_server->init();

//	GDREGISTER_CLASS(HydrogenBehaviorServer);

	Engine::get_singleton()->register_singleton("HydrogenBehaviorServer", HydrogenBehaviorServer::get_singleton());

}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

	Engine::get_singleton()->unregister_singleton("HydrogenBehaviorServer");

	HydrogenBehaviorServer::get_singleton()->finish();
	memdelete(HydrogenBehaviorServer::get_singleton());
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT hydrogen_gd_behaviors_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SERVERS);

		return init_obj.init();
	}
}