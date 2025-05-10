#include "register_types.h"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/classes/engine.hpp>

#include "game_ai_blackboard.hpp"
#include "game_ai_server.hpp"

#ifdef TESTS_ENABLED
#include "tests.hpp"
#endif

using namespace godot;
using namespace hydrogen;

static bool has_tests_enabled() {
#ifdef TESTS_ENABLED
	return tests::k_test_baz.a == 42;
#else
	return false;
#endif

}

static GameAIServer *game_ai_server = NULL;
static _GameAIServer *_game_ai_server = NULL;
class HydrogenBlackboard;

const String k_server_name = "GameAIServer";

#define CLEAN_MEM_DELETE(x) \
	memdelete(x);			\
	x = nullptr;			\

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	game_ai_server = memnew(GameAIServer);
	game_ai_server->init();

	_game_ai_server = memnew(_GameAIServer);

	GDREGISTER_CLASS(_GameAIServer);
	Engine::get_singleton()->register_singleton(k_server_name, _GameAIServer::get_singleton());

}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Engine::get_singleton()->unregister_singleton(k_server_name);

	if (game_ai_server) {
		game_ai_server->finish();
		CLEAN_MEM_DELETE(game_ai_server);
	}

	GameAIServer::get_singleton()->finish();
	memdelete(GameAIServer::get_singleton());

	if (_game_ai_server) {
		CLEAN_MEM_DELETE(_game_ai_server);
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