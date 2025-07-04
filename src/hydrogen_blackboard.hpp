#ifndef HYDROGEN_BLACKBOARD_HPP
#define HYDROGEN_BLACKBOARD_HPP

#include "godot_cpp/templates/hash_map.hpp"

#include <godot_cpp/classes/resource.hpp>
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
	T get_entry(const StringName &p_name, const T &p_default, bool p_check_parents = true);

	[[nodiscard]] bool erase_entry(const StringName &p_name) const;

	[[nodiscard]] bool has_entry(const StringName &p_name) const;

	Dictionary export_entries() const;

	static Dictionary export_type_infos();
};



#endif //HYDROGEN_BLACKBOARD_HPP