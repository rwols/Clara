
#include <iostream>

/**
 * @brief some brief comment about SomeStruct!
 * @details A very long description.
 */
struct SomeStruct
{
    /**
     * @brief a brief comment about this lonely integer.
     * @details Boring details.
     */
    int data;

    float what(const SomeStruct& another, float extraParamater = 2.0f) const;
};

namespace A
{
    /**
     * @brief      Does very useful things. Sticks the thing in the thing.
     *
     * @param[in]  arg1  The argument 1
     * @param[in]  arg2  The argument 2
     */
    void foo(int arg1, float arg2, unsigned defaultArg = 0)
    {
        int x = 0;
    }
}

int baz(float x1, float x2)
{
    return static_cast<int>(x1 + x2);
}

int main()
{
    SomeStruct hello;
    hello.what(hello, 10.0f);

    
    return 0;
}