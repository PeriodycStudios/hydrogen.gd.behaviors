#include "register_types.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/engine.hpp>

#include "behavior_server.hpp"
#include "blackboard.hpp"

#ifdef TESTS_ENABLED
#include "tests.hpp"
#endif

using namespace godot;
using namespace hydrogen;

static BehaviorServer *behavior_server = nullptr;
static _BehaviorServer *_behavior_server = nullptr;
class HydrogenBlackboard;

const String k_server_name = "GameAIServer";

#define CLEAN_MEM_DELETE(x) \
	memdelete(x);			\
	(x) = nullptr;			\

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	behavior_server = memnew(BehaviorServer);
	behavior_server->init();

	_behavior_server = memnew(_BehaviorServer);

	GDREGISTER_CLASS(_BehaviorServer);
	Engine::get_singleton()->register_singleton(k_server_name, _BehaviorServer::get_singleton());

}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine::get_singleton()->unregister_singleton(k_server_name);

	if (behavior_server) {
		behavior_server->finish();
		CLEAN_MEM_DELETE(behavior_server);
	}

	BehaviorServer::get_singleton()->finish();
	memdelete(BehaviorServer::get_singleton());

	if (_behavior_server) {
		CLEAN_MEM_DELETE(_behavior_server);
	}
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