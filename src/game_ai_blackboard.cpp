//
// Created by tkey on 5/6/25.
//

#include "game_ai_blackboard.hpp"

namespace hydrogen {

template<typename V, EnableIf<detail::is_variant_type<V>::value, V> = true>
Vector2i compute_type_key() {

	constexpr GDExtensionVariantType type = GetTypeInfo<V>::VARIANT_TYPE;
	constexpr GDExtensionClassMethodArgumentMetadata metadata = GetTypeInfo<V>::METADATA
		+ GDEXTENSION_VARIANT_TYPE_VARIANT_MAX;

	static Vector2i key = {type, metadata};
	return key;
}

template <typename V>
Variant GameAIBlackboard::EntryTable<V>::get_variant(const StringName &p_name) const {
	return Variant();
}

bool GameAIBlackboard::validate_parent(GameAIBlackboard *p_parent) {
	return false;
}

GameAIBlackboard::GameAIBlackboard() :
		parent(nullptr) {}

GameAIBlackboard::GameAIBlackboard(GameAIBlackboard *p_parent) :
		parent(p_parent) {}

GameAIBlackboard::~GameAIBlackboard() {
}

template <typename T>
typename EnableIf<detail::is_variant_type<T>::value, T>::type GameAIBlackboard::get_entry(const StringName &p_name) const {
	return {};
}
template <typename T, EnableIf<detail::is_variant_type<T>::value, T>>
void GameAIBlackboard::set_entry(const StringName &p_name, const T &p_value) {
}

template<>
Variant GameAIBlackboard::get_entry<Variant>(const StringName &p_name) const {
	return {};
}

void GameAIBlackboard::set_entry(const StringName &p_name, const Variant &p_value) {

}

bool GameAIBlackboard::erase_entry(const StringName &p_name) {
	return false;
}

bool GameAIBlackboard::has_entry(const StringName &p_name, bool check_parents) const {
	return false;
}

} //namespace Hydrogen
