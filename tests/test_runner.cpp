//
// Created by tkey on 5/9/25.
//

#ifndef TESTS_HPP
#define TESTS_HPP

#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest.h>

#include <cstdint>
#include <godot_cpp/classes/os.hpp>
#include <vector>

namespace hydrogen::tests {

using namespace godot;

int behavior_test_runner() {
	auto *os = OS::get_singleton();
	auto packed_args = os->get_cmdline_user_args();

	const int32_t argc = packed_args.size();
	std::vector<const char*> args;
	args.reserve(argc);

	for (const auto& packed_arg : packed_args) {
		const CharString c_str = packed_arg.ascii();
		args.push_back(c_str.ptr());
	}

	const char **argv = args.data();
	doctest::Context context(argc, argv);

	return context.run();
}
}

#endif //TESTS_HPP
