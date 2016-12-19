struct World
{
    int data;
    int add(int x, int y)
    {
        return data + x + y;
    }
    int add(int z)
    {
        return data + z;
    }

    World(int x)
    {
        data = x;
    }
    World(float x)
    {
        data = static_cast<int>(x);
    }
    World(double x)
    {
        data = static_cast<int>(x);
    }
};

struct ASDF
{
	float another;
private:
    int hidden;
};

int main()
{
    World world(4);
    world.data = 5;

    world.data = 3;

    return 0;
}