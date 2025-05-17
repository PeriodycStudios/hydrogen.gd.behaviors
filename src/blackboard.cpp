//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

namespace hydrogen {

template <typename V>
Variant Blackboard::EntryTable<V>::get_variant(const StringName &p_name) const {
	return Variant();
}

bool Blackboard::validate_parent(Blackboard *p_parent) {
	return false;
}

Blackboard::Blackboard() :
		parent(nullptr) {}

Blackboard::Blackboard(Blackboard *p_parent) :
		parent(p_parent) {}

Blackboard::~Blackboard() {
}

template <typename T>
typename EnableIf<blackboard_storage_type<T>::value, T>::type Blackboard::get_entry(const StringName &p_name) const {
	return {};
}

template<>
Variant Blackboard::get_entry<Variant>(const StringName &p_name) const {
	return {};
}

template <typename T>
void set_entry(const StringName &p_name, const typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_value) {

}

template<>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value) {

}

bool Blackboard::erase_entry(const StringName &p_name) {
	return false;
}

bool Blackboard::has_entry(const StringName &p_name, bool check_parents) const {
	return false;
}

} //namespace Hydrogen
