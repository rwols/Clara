struct World
{
    int data;
    int add(int x, int y)
    {
        return data + x + y;
    }
};

struct ASDF
{
	float another;
};

int main()
{
    World world;
    world.data = 5;

    world.add(6, 10);

    return 0;
}