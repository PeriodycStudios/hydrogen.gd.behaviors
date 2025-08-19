//
// Created by tkey on 5/9/25.
//

#ifndef TESTS_HPP
#define TESTS_HPP

#include "test_runner.hpp"
#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest.h>

#include <cstdint>
#include <godot_cpp/classes/os.hpp>
#include <vector>

namespace hydrogen::tests {

using namespace godot;

// TODO: Move this out into it's own plugin.
int behavior_test_runner() {
	auto *os = OS::get_singleton();
	auto packed_args = os->get_cmdline_user_args();

	const int32_t packed_args_count = packed_args.size();
	std::vector<const char*> args;
	args.reserve(packed_args_count);

	for (const auto& packed_arg : packed_args) {
		if (unlikely(packed_arg == "--test")) continue;
		const CharString c_str = packed_arg.ascii();
		args.push_back(c_str.ptr());
	}

	const char **argv = args.data();
	const int32_t argc = static_cast<int32_t>(args.size());
	doctest::Context context(argc, argv);

	const int result = context.run();
	print_line("Test run result: ", result);

	return result;
}

void behavior_tests_runner_include() {}

}

#endif //TESTS_HPP
