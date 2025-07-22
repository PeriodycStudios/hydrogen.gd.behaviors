//
// Created by tkey on 7/20/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "rid_data.hpp"

namespace hydrogen {

class Pipeline : public RidData {


	Blackboard *blackboard;
	BehaviorsNodeBase *root;
	bool halting = false;

protected:

	Pipeline(Blackboard *p_blackboard, BehaviorsNodeBase *p_root) : blackboard{p_blackboard}, root {p_root} {};
	virtual ~Pipeline();

	static const StringName &halting_name() {
		static const StringName halting_name{"__halting"};
		return halting_name;
	}

	static const StringName &error_name() {
		static const StringName error_name{"__error"};
		return error_name;
	}

	_FORCE_INLINE_ Blackboard *get_blackboard() { return blackboard; }
	_FORCE_INLINE_ BehaviorsNodeBase *get_root() { return root; }

public:
	virtual void execute() = 0;

	virtual void halt() { halting = true; };
	bool is_halting() const;
	virtual bool is_fully_halted() const = 0;
	virtual void clear_halt() { halting = false; }

	_FORCE_INLINE_ const Blackboard *get_blackboard() const { return blackboard; }
	_FORCE_INLINE_ const BehaviorsNodeBase *get_root() const { return root; }

	virtual String get_error() const = 0;
};
}

#endif //PIPELINE_HPP
