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
	virtual ~Pipeline() = default;

	_FORCE_INLINE_ Blackboard *get_blackboard() { return blackboard; }
	_FORCE_INLINE_ BehaviorsNodeBase *get_root() { return root; }

	bool _is_dirty = false;

	virtual void _update_dirty() { _is_dirty = false; };

public:
	virtual void execute() = 0;

	virtual void halt() { halting = true; };
	[[nodiscard]] _FORCE_INLINE_ bool is_halting() const { return halting; };
	[[nodiscard]] virtual bool is_fully_halted() const = 0;
	virtual void clear_halt() { halting = false; }

	[[nodiscard]] _FORCE_INLINE_ const Blackboard *get_blackboard() const { return blackboard; }
	[[nodiscard]] _FORCE_INLINE_ const BehaviorsNodeBase *get_root() const { return root; }

	[[nodiscard]] virtual String get_error() const = 0;

	void mark_dirty() {
		if (is_fully_halted()) {
			_update_dirty();
		}
		else {
			_is_dirty = true;
		}
	}
	[[nodiscard]] _FORCE_INLINE_ bool is_dirty() const { return _is_dirty; }
};
}

#endif //PIPELINE_HPP
