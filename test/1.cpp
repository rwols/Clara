struct x
{
    int d;
};

int main()
{
    x a;
    unsigned y = a.d;
    x *b;
    b = &a;
    return 0;
}
