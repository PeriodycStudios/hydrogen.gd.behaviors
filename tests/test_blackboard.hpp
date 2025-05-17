//
// Created by tkey on 5/9/25.
//

#ifndef TEST_BLACKBOARD_HPP
#define TEST_BLACKBOARD_HPP

#include "blackboard.hpp"
#include <doctest.h>

namespace hydrogen {
namespace test {

TEST_CASE("[Blackboard] Set and Get") {
	CHECK(true);

	Blackboard* blackboard = memnew(Blackboard());
	REQUIRE(blackboard != nullptr);

	const StringName entry_name = "SomeEntry";

	blackboard->set_entry<uint8_t>(entry_name, 255);
	REQUIRE(blackboard->get_entry<uint8_t>(entry_name) == 255);

	memdelete(blackboard);
}

}
}

#endif //TEST_BLACKBOARD_HPP
