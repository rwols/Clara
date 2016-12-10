
#include <iostream>
#include <string>

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
    float what(const SomeStruct& another, float extraParamater = 2.0f) const
    {
        std::cout << "another data: " << another.data
            << ", my data: " << this->data << " with extra param " << extraParamater << '\n';
        return extraParamater + data;
    }

private:

    float secret;
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

enum Color { red, green, blue };

int main()
{
    baz(10.0f, 20.0f);

    SomeStruct asdf;
    asdf.what(asdf, 20.0f);
    baz(10.0f, 20.0f);

    Color color = red;
    switch (color)
    {
        case red:
            break;
        case blue:
            break;
        case green:
            break;
        default:
            break;
    }

    baz(30.0f, 40.0f);

    SomeStruct asdf;

    return 0;
}