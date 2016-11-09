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
    const char* getName() const noexcept
    {
        return this->mIdentifier;
    }
private:
    const char* mIdentifier;
};

int main()
{
    int z = add(10, 20);
    int w = add(z, 30);
    int a = add(z, w);

    auto inst = new MyClass("hello, world!");
    inst->getName();

    return 0;
}