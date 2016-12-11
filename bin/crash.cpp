#include <memory>

#define MAKE_IT_CRASH

#ifdef MAKE_IT_CRASH
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcovered-switch-default"
	#include "crash.hpp"
	#pragma clang diagnostic pop
#else
	#include "crash.hpp"
#endif // MAKE_IT_CRASH

int main()
{
	foo();
	return 0;
}