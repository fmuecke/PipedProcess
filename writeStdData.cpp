#include <iostream>
#include <vector>
#include <windows.h>

using namespace std;

int main()
{
	if (cin.good())
	{
		cin.seekg(0, cin.end);
		auto len = cin.tellg();
		cout << "cin len: " << len << endl;
		cin.seekg(0, cin.beg);
		if (len > 0)
		{
			std::vector<char> buffer(static_cast<int>(len) + 1, 0);
			cin.read(&buffer[0], len);
			cout << "stdin data: " << &buffer[0] << endl;
		}
	}
::Sleep(5000);
	cout << "something\nfor\nstd out" << endl;
	cerr << "something\nfor\nstd err" << endl;
	return 0;
}
