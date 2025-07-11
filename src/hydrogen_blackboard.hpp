#ifndef HYDROGEN_BLACKBOARD_HPP
#define HYDROGEN_BLACKBOARD_HPP

#include "behavior_server.hpp"
#include "blackboard.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>

using namespace godot;

class HydrogenBlackboard : public Resource {
	GDCLASS(HydrogenBlackboard, Resource);

	static HashMap<RID, HydrogenBlackboard*> lookups;
	static HydrogenBlackboard *get_by_rid(RID rid);

	RID blackboard;

protected:
	static void _bind_methods();

public:

	HydrogenBlackboard();
	~HydrogenBlackboard() override;

	[[nodiscard]] RID _get_rid() const override { return blackboard; }

	[[nodiscard]] bool set_parent(Ref<HydrogenBlackboard> parent) const;
	[[nodiscard]] Ref<HydrogenBlackboard> get_parent() const;

	template<typename T>
	bool try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	template<typename T>
	[[nodiscard]] const T &get_entry_fast(const StringName &p_name, const T &p_default, bool p_check_parents = true) const;

	template <typename T>
	[[nodiscard]] T get_entry(const StringName &p_name, T p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	void set_entry_fast(const StringName &p_name, const T& p_value);

	template <typename T>
	void set_entry(const StringName &p_name, T p_value);

	bool set_from_dictionary(Dictionary data) const;

	[[nodiscard]] bool erase_entry(const StringName &p_name) const;

	[[nodiscard]] bool has_entry(const StringName &p_name) const;

	[[nodiscard]] Dictionary export_entries() const;

	static Dictionary export_type_infos();
};

template <typename T>
_FORCE_INLINE_ bool HydrogenBlackboard::try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents) {
	return hydrogen::BehaviorServer::get_singleton()->blackboard_try_get_entry(blackboard, p_name, p_out_result, p_check_parents);
}

template <typename T>
const T &HydrogenBlackboard::get_entry_fast(const StringName &p_name, const T &p_default, bool p_check_parents) const {
	return hydrogen::BehaviorServer::get_singleton()->blackboard_get_entry(blackboard, p_name, p_default, p_check_parents);
}

template <typename T>
T HydrogenBlackboard::get_entry(const StringName &p_name, T p_default, bool p_check_parents) const {
	return hydrogen::BehaviorServer::get_singleton()->blackboard_get_entry(blackboard, p_name, p_default, p_check_parents);
}

template <typename T>
void HydrogenBlackboard::set_entry_fast(const StringName &p_name, const T &p_value) {
	hydrogen::BehaviorServer::get_singleton()->blackboard_set_entry(blackboard, p_name, p_value);
}

template <typename T>
void HydrogenBlackboard::set_entry(const StringName &p_name, T p_value) {
	hydrogen::BehaviorServer::get_singleton()->blackboard_set_entry(blackboard, p_name, p_value);
}

#endif //HYDROGEN_BLACKBOARD_HPP