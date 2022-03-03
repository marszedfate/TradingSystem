#include <iostream>

class MyTest {
public:
	static int& GetInstance()
	{
		static int instance;
		return instance;
	}
};

int main()
{
	int &x = MyTest::GetInstance();
	x++;
	int &y = MyTest::GetInstance();
	std::cout << y;
	return 0;
}