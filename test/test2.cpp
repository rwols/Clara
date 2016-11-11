
#include <iostream>

/**
 * @brief      some brief comment about SomeStruct!
 * @details    A very long description.
 */
struct SomeStruct
{

    int data;


    /**
     * @brief      does whatty things
     *
     * @param[in]  another         another SomeStruct
     * @param[in]  extraParamater  The extra paramater
     *
     * @return     returns floaty things.
     */
    float what(const SomeStruct& another, float extraParamater = 2.0f) const;
};

namespace A
{
    /**
     * @brief      does foo things
     *
     * @param[in]  arg1        the arg 1
     * @param[in]  arg2        the arg 2 very floaty
     * @param[in]  defaultArg  don't touch this unless you have to
     */
    void foo(int arg1, float arg2, unsigned defaultArg = 0)
    {
        int x = 0;
    }
}


/**
 * @brief      asdf
 * @details    asdf
 * @return     asdf
 */
int baz(float x1, float x2)
{
    return static_cast<int>(x1 + x2);
}

int main()
{
    std::cout << "Hello, world!\n";

    baz(10, 20);

    SomeStruct hello;

    

    return 0;
}