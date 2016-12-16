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
private:
    int hidden;
};

int main()
{
    World world;
    world.data = 5;

    world.data = 3;

    return 0;
}