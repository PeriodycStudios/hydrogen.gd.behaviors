
#include "behavior_server.hpp"

#include "test_runner.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace hydrogen {

BehaviorServer *BehaviorServer::singleton = nullptr;

void BehaviorServer::_bind_methods() {}

BehaviorServer *BehaviorServer::get_singleton() {
	return singleton;
}

BehaviorServer::BehaviorServer() {
	singleton = this;
}

BehaviorServer::~BehaviorServer() {
	singleton = nullptr;
}

void BehaviorServer::lock() const {
	if (!mutex) {
		return;
	}

	mutex->lock();
}

void BehaviorServer::unlock() const {
	if (!mutex) {
		return;
	}

	mutex->unlock();
}

void BehaviorServer::free_rid(RID p_rid) {
	if (blackboard_owner.owns(p_rid)) {
		lock();

		Blackboard *blackboard = blackboard_owner.get_or_null(p_rid);
		blackboard_owner.free(p_rid);
		memdelete(blackboard);

		unlock();
	}
	else {
		ERR_FAIL_MSG("Invalid ID.");
	}
}

RID BehaviorServer::blackboard_create() {
	lock();

	Blackboard *blackboard = memnew(Blackboard);
	RID rid = blackboard_owner.make_rid(blackboard);
	blackboard->set_self(rid);

	unlock();

	return rid;
}

RID BehaviorServer::behavior_tree_create() {
	return {};
}

RID BehaviorServer::state_machine_create() {
	return {};
}

RID BehaviorServer::agent_create() {
	return {};
}

Error BehaviorServer::init() {
 	mutex = memnew(Mutex);
 	return OK;
}

void BehaviorServer::finish() {

	if (mutex) {
		memdelete(mutex);
		mutex = nullptr;
	}
}


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
	ClassDB::bind_method(D_METHOD("run_tests"), &_BehaviorServer::run_tests);
#endif
}

// ReSharper disable CppMemberFunctionMayBeStatic
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
// ReSharper restore CppMemberFunctionMayBeStatic
}
