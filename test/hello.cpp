#include <iostream>

void somefunc()
{
	std::cerr << "We are in a function.";
}

int main()
{
	std::cout << "Hello, world!\n";
	somefunc();
	return 0;
}