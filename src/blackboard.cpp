//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

void HydrogenBlackboard::set_value(const StringName &p_name, const Variant &p_value) {
	entries_owner[p_name] = p_value;
}

Variant HydrogenBlackboard::get_value(const StringName &p_name) {
	Variant* value_ptr = entries_owner.getptr(p_name);
	return value_ptr != nullptr ? *value_ptr : Variant();
}

bool HydrogenBlackboard::clear_entry(const StringName &p_name) {
	return entries_owner.erase(p_name);
}

bool HydrogenBlackboard::has_entry(const StringName &p_name) {
	return entries_owner.has(p_name);
}

Vector<Pair<StringName, Variant>> HydrogenBlackboard::get_entries() const {

	Vector<Pair<StringName, Variant>> entry_pairs = Vector<Pair<StringName, Variant>>();
	entry_pairs.resize(entries_owner.size());
	int i = 0;
	Pair<StringName, Variant>* entry_pair_ptr = entry_pairs.ptrw();
	for (const auto& entry : entries_owner) {
		entry_pair_ptr[i] = Pair(entry.key, entry.value);
		i++;
	}

	return entry_pairs;
}
