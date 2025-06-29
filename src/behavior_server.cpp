
#include "behavior_server.hpp"

#include "test_runner.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace hydrogen {

BehaviorServer *BehaviorServer::singleton = nullptr;
BehaviorServer *BehaviorServer::get_singleton() {
	return singleton;
}

void BehaviorServer::_bind_methods() {}

_BehaviorServer *_BehaviorServer::singleton = nullptr;
_BehaviorServer *_BehaviorServer::get_singleton() {
	return singleton;
}

void _BehaviorServer::_bind_methods() {

	ClassDB::bind_method(D_METHOD("free_rid", "rid"), &_BehaviorServer::free_rid);
	ClassDB::bind_method(D_METHOD("blackboard_create"), &_BehaviorServer::blackboard_create);
	ClassDB::bind_method(D_METHOD("behavior_tree_create"), &_BehaviorServer::behavior_tree_create);
	ClassDB::bind_method(D_METHOD("state_machine_create"), &_BehaviorServer::state_machine_create);
	ClassDB::bind_method(D_METHOD("agent_create"), &_BehaviorServer::agent_create);

#if TESTS_ENABLED
	ClassDB::bind_method( D_METHOD("run_tests"), &_BehaviorServer::run_tests);
#endif
}

void _BehaviorServer::free_rid(RID p_rid) {
	BehaviorServer::get_singleton()->free_rid(p_rid);
}

RID _BehaviorServer::blackboard_create() {
	return BehaviorServer::get_singleton()->blackboard_create();
}

RID _BehaviorServer::behavior_tree_create() {
	return BehaviorServer::get_singleton()->behavior_tree_create();
}

RID _BehaviorServer::state_machine_create() {
	return BehaviorServer::get_singleton()->state_machine_create();
}

RID _BehaviorServer::agent_create() {
	return BehaviorServer::get_singleton()->agent_create();
}

void _BehaviorServer::run_tests() {
	tests::behavior_test_runner();
}

}
