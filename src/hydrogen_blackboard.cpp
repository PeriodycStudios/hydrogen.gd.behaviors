
#include "hydrogen_blackboard.hpp"

#include "behavior_server.hpp"

using namespace hydrogen;

HashMap<RID, HydrogenBlackboard*> HydrogenBlackboard::lookups = {};

HydrogenBlackboard *HydrogenBlackboard::get_by_rid(RID rid) {
	const auto iter = lookups.find(rid);
	if (likely(iter != lookups.end())) {
		return iter->value;
	}
	return nullptr;
}


void HydrogenBlackboard::_bind_methods() {

}

HydrogenBlackboard::HydrogenBlackboard() {
	blackboard = BehaviorServer::get_singleton()->blackboard_create();
	lookups[blackboard] = this;
}

HydrogenBlackboard::~HydrogenBlackboard() {
	ERR_FAIL_NULL(BehaviorServer::get_singleton());
	lookups.erase(blackboard);
	BehaviorServer::get_singleton()->free_rid(blackboard);
}

_FORCE_INLINE_ bool HydrogenBlackboard::set_parent(Ref<HydrogenBlackboard> parent) const {
	RID parent_rid = parent.is_valid() ? parent->_get_rid() : RID();
	return BehaviorServer::get_singleton()->blackboard_set_parent(blackboard, parent_rid);
}

Ref<HydrogenBlackboard> HydrogenBlackboard::get_parent() const {
	RID parent_rid = BehaviorServer::get_singleton()->blackboard_get_parent(blackboard);
	if (parent_rid == RID()) {
		return {};
	}

	Ref parent = get_by_rid(parent_rid);
	ERR_FAIL_COND_V(parent.is_null(), parent);
	return parent;
}

template <typename T>
_FORCE_INLINE_ bool HydrogenBlackboard::try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_try_get(blackboard, p_name, p_out_result, p_check_parents);
}

template <typename T>
_FORCE_INLINE_ T HydrogenBlackboard::get_entry(const StringName &p_name, const T& p_default, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_get_entry(blackboard, p_name, p_default, p_check_parents);
}

_FORCE_INLINE_ bool HydrogenBlackboard::erase_entry(const StringName &p_name) const {
	return BehaviorServer::get_singleton()->blackboard_erase_entry(blackboard, p_name);
}

_FORCE_INLINE_ bool HydrogenBlackboard::has_entry(const StringName &p_name) const {
	return BehaviorServer::get_singleton()->blackboard_has_entry(blackboard, p_name);
}

_FORCE_INLINE_ Dictionary HydrogenBlackboard::export_entries() const {
	return BehaviorServer::get_singleton()->blackboard_export_entries(blackboard);
}

_FORCE_INLINE_ Array HydrogenBlackboard::export_type_infos() {
	return BehaviorServer::blackboard_export_type_infos();
}

