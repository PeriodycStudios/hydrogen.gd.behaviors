//
// Created by tkey on 7/18/25.
//

#ifndef BEHAVIORS_NODE_BASE_HPP
#define BEHAVIORS_NODE_BASE_HPP

#include "rid_data.hpp"
#include "blackboard.hpp"

namespace hydrogen {

class BehaviorsNodeBase : public RidData {

public:
	BehaviorsNodeBase();
	virtual ~BehaviorsNodeBase();
};

} // hydrogen

#endif //BEHAVIORS_NODE_BASE_HPP
