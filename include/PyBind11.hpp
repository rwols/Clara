#pragma once

#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcovered-switch-default"
	#pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-qual"
#endif // __clang__

#include <pybind11/pybind11.h>

#ifdef __clang__
	#pragma clang diagnostic pop
#elif defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif // __clang__