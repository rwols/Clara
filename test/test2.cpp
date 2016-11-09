
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

int baz(float x1, float x2)
{
    /**
     * @brief      asdf
     * @details    asdf
     * @return     asdf
     */
    return static_cast<int>(x1 + x2);
}

int main()
{
    SomeStruct hello;
    hello.what(asdf, asdf);
    SomeStruct whatevs;

    return 0;
}