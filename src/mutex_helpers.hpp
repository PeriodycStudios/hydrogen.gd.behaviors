#pragma once

#define LOCK_ONE(mtx)					\
	ERR_FAIL_NULL(mtx);					\
	std::scoped_lock lock(*mtx)			\

#define LOCK_ONE_V(mtx, fail_result)	\
	ERR_FAIL_NULL_V(mtx, fail_result);	\
	std::scoped_lock lock(*mtx)			\

#define LOCK_TWO(first, second)					\
	ERR_FAIL_NULL(first);						\
	ERR_FAIL_NULL(second);						\
	std::scoped_lock lock(*first, *second)		\

#define LOCK_TWO_V(first, second, fail_result)  \
    ERR_FAIL_NULL_V(first, fail_result);        \
    ERR_FAIL_NULL_V(second, fail_result);       \
    std::scoped_lock lock(*first, *second)      \

#define LOCK_THREE(first, second, third)			\
	ERR_FAIL_NULL(first);							\
	ERR_FAIL_NULL(second);							\
	ERR_FAIL_NULL(third);							\
	std::scoped_lock lock(*first, *second, *third)	\

#define LOCK_THREE_V(first, second, third, fail_result) \
	ERR_FAIL_NULL_V(first, fail_result);				\
	ERR_FAIL_NULL_V(second, fail_result);				\
	ERR_FAIL_NULL_V(third, fail_result);				\
    std::scoped_lock lock(*first, *second, *third)      \
