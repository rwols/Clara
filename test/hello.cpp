#include <iostream>

namespace A {

// forward declaration -- 2spooky4me
void somefunc();

}

namespace B {

// oops in another namespace -- don't rename me!
void somefunc();

}

int main()
{
	using namespace A;
	std::cout << "Hello, world!\n";
	somefunc();

	void (*functionPointer)();
	functionPointer = &somefunc;
	functionPointer();
	
	return 0;
}

namespace A {

void somefunc()
{
	std::cout << "We are in a function.\n";
}

}

namespace B {

void somefunc()
{
	std::cout << "You better not rename me!\n";
}

}