#include <iostream>

/**
 * @brief      Adds two integers together.
 *
 * @param[in]  x     The first number.
 * @param[in]  y     The second number.
 *
 * @return     The sum of the two numbers.
 */
int add(int x, int y)
{
	return x + y;
}

/**
 * @brief      My awesome class.
 */
class MyClass
{
public:

	/**
	 * @brief      Constructor
	 *
	 * @param[in]  identifier  The identifier
	 */
	MyClass(const char* identifier)
	: mIdentifier(identifier)
	{

	}


	/**
	 * @brief      Destroys the object.
	 */
	~MyClass()
	{

	}

	/**
	 * @brief      Gets the name.
	 *
	 * @return     The name.
	 */
	const char* getName() const
	{
		return this->mIdentifier;
	}
private:
	const char* mIdentifier;
};

int main()
{
	MyClass asdf("hello, world!");	
	std::cout << asdf.getName() << '\n';

  	return 0;    
}
 