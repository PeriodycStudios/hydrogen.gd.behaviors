#ifndef HYDROGEN_BLACKBOARD_HPP
#define HYDROGEN_BLACKBOARD_HPP

#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

#include "behavior_server.hpp"

using namespace godot;

class HydrogenBlackboard : public Resource {
	GDCLASS(HydrogenBlackboard, Resource);

	static HashMap<RID, HydrogenBlackboard*> lookups;
	static HydrogenBlackboard *get_by_rid(RID rid);

	RID blackboard;
	TypedDictionary<StringName, Variant> initial_values = {};

	Ref<HydrogenBlackboard> parent;


protected:
	static void _bind_methods();

public:

	HydrogenBlackboard();
	~HydrogenBlackboard() override;

	bool is_empty() const;
	uint32_t size() const;

	[[nodiscard]] RID _get_rid() const override { return blackboard; }

	void set_initial_values(TypedDictionary<StringName, Variant> p_initial_values);
	[[nodiscard]] _FORCE_INLINE_ TypedDictionary<StringName, Variant> get_initial_values() const { return initial_values; }

	bool set_parent(const Ref<HydrogenBlackboard> &p_parent);
	[[nodiscard]] _FORCE_INLINE_ Ref<HydrogenBlackboard> get_parent() const { return parent; }
	[[nodiscard]] bool is_ancestor(const Ref<HydrogenBlackboard> &p_candidate) const;

	template<typename T>
	bool try_get_entry(const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	Variant try_get_entry_variant(const StringName &p_name, bool p_check_parents = true) const;

	template<typename T>
	[[nodiscard]] const T &get_entry_fast(const StringName &p_name, const T &p_default, bool p_check_parents = true) const;

	template <typename T>
	[[nodiscard]] T get_entry(const StringName &p_name, T p_default = {}, bool p_check_parents = true) const;

	template <typename T>
	void set_entry_fast(const StringName &p_name, const T& p_value);

	template <typename T>
	void set_entry(const StringName &p_name, T p_value);

	[[nodiscard]] bool erase_entry(const StringName &p_name) const;

	[[nodiscard]] bool has_entry(const StringName &p_name) const;

	bool import_entries(const TypedDictionary<StringName, Variant> &p_values) const;
	[[nodiscard]] Dictionary export_entries(bool p_include_parents = true) const;

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