//
// Created by tkey on 4/2/25.
//

#ifndef GAME_AI_BLACKBOARD_HPP
#define GAME_AI_BLACKBOARD_HPP

#include "game_ai_rid.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace hydrogen {

namespace detail {
template <typename T, typename = void>
	struct is_variant_type : std::false_type {};

template<typename T>
struct is_variant_type<T, std::void_t<decltype(GetTypeInfo<T>::get_class_info)>> : std::true_type {};
}

class GameAIBlackboard final : public GameAIRid {

	class EntryTableBase : GameAIRid {
	public:
		virtual ~EntryTableBase() = default;
		virtual Variant get_variant(const StringName &p_name) const = 0;
	};

	template <typename V>
	class EntryTable final : EntryTableBase {
		HashMap<StringName, V> entries;

		Variant get_variant(const StringName &p_name) const override;
	};



	RID_PtrOwner<EntryTableBase> entries_owner;
	HashMap<Vector2i, EntryTableBase *> entries;
	HashMap<StringName, EntryTableBase *> name_to_table;
	GameAIBlackboard *parent;

	bool validate_parent(GameAIBlackboard *p_parent);

public:
	GameAIBlackboard();
	explicit GameAIBlackboard(GameAIBlackboard *p_parent);
	~GameAIBlackboard();

	_FORCE_INLINE_ bool set_parent(GameAIBlackboard *p_parent) {
		if (validate_parent(p_parent)) {
			parent = p_parent;
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ GameAIBlackboard *get_parent() const { return parent; }

	template <typename T>
	typename EnableIf<detail::is_variant_type<T>::value, T>::type get_entry(const StringName &p_name) const;

	template <typename T, EnableIf<detail::is_variant_type<T>::value, T> = true>
	void set_entry(const StringName &p_name, const T &p_value);

	void set_entry(const StringName &p_name, const Variant &p_value);

	bool erase_entry(const StringName &p_name);

	bool has_entry(const StringName &p_name, bool check_parents = true) const;
};

template <>
Variant GameAIBlackboard::get_entry<Variant>(const StringName &p_name) const;


} //namespace hydrogen

#endif //GAME_AI_BLACKBOARD_H
