//
// Created by tkey on 5/9/25.
//

#ifndef TEST_BLACKBOARD_HPP
#define TEST_BLACKBOARD_HPP

#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#include <doctest.h>

DOCTEST_MAKE_STD_HEADERS_CLEAN_FROM_WARNINGS_ON_WALL_BEGIN
#include <cstdio>
DOCTEST_MAKE_STD_HEADERS_CLEAN_FROM_WARNINGS_ON_WALL_END

#include <godot_cpp/variant/string.hpp>
#include "blackboard.hpp"

namespace hydrogen::test {

TEST_CASE("[Blackboard] Set and Get") {
	CHECK(true);

	Blackboard *blackboard = memnew(Blackboard("TestBlackboard"));
	REQUIRE(blackboard != nullptr);

	blackboard->set_entry<uint8_t>("SomeEntry", 255);
	uint8_t uint8_result;
	CHECK(blackboard->try_get_entry<uint8_t>("SomeEntry", uint8_result));
	CHECK(uint8_result == 255);
	CHECK(blackboard->get_entry<uint8_t>("SomeEntry") == 255);

	memdelete(blackboard);
}

} //namespace hydrogen::test

#endif //TEST_BLACKBOARD_HPP
