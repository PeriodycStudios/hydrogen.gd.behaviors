#ifndef BLACKBOARD_NAME_HPP
#define BLACKBOARD_NAME_HPP

#include "type_name.hpp"
#include <godot_cpp/variant/string_name.hpp>
#include <string>


#define DEFINE_NAME_STATIC(entry_name)			        \
	static const StringName &entry_name##_name() {		\
		const static StringName name(#entry_name, true);\
		return name;									\
	}

template<typename T>
godot::StringName get_type_name_static() {
    static const godot::StringName type_name = godot::StringName(std::string(::type_name<T>()).c_str(), true);
    return type_name;
}

#endif