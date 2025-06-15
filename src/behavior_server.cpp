
#include "behavior_server.hpp"

namespace hydrogen {

BehaviorServer *BehaviorServer::singleton = nullptr;
BehaviorServer *BehaviorServer::get_singleton() {
	return singleton;
}


_BehaviorServer *_BehaviorServer::singleton = nullptr;
_BehaviorServer *_BehaviorServer::get_singleton() {
	return singleton;
}

}
