//
// Created by tkey on 5/9/25.
//

#ifndef TESTS_HPP
#define TESTS_HPP

#include "test_blackboard.hpp"

namespace hydrogen {
namespace tests {

struct Foo;
struct Bar {};
struct Baz { int a; };
struct Qux { void *ptr_to_something; };

const Baz k_test_baz = {42};

}
}

#endif //TESTS_HPP
