#include <vector>
#include <string>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

using namespace std;

static char const * const fileName = "cin.out";

int main()
{
	if (cin.good())
	{
		if (_setmode(_fileno(stdin), _O_BINARY) == -1)
		{
			cerr << "error switching stdin to binary mode: " << strerror(errno);
			return -1;
		}
		cin.seekg(0, cin.end);
		auto len = cin.tellg();
		cin.seekg(0, cin.beg);
		if (len > 0)
		{
			vector<char> v(static_cast<size_t>(len), 0);
			cin.read(&v[0], len);
			ofstream outfile(fileName, ios::binary | ios::trunc);
			outfile.write(&v[0], v.size());
			outfile.close();
		
			_setmode(_fileno(stdout), _O_BINARY);
			cout.write(&v[0], v.size());
			cout << len << " bytes written to '" << fileName << "'" << endl;
			return 0;
		}
		else
		{
			cerr << "error: stdin contains no data" << endl;
		}
	}
	else
	{
		cerr << "error: unable to open stdin" << endl;
	}

	return -1;
}

