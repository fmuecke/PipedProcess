// This file is part of the PipedProcess project.
// See LICENSE file for further information (BSD 3-clause)
// https://github.com/fmuecke/PipedProcess

#include <vector>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include "..\PipedProcess\ChildProc.h"

using namespace std;

static char const * const fileName = "cin.out";

static int ReadInput(vector<char>& resultData)
{
    if (cin.good())
	{
		if (_setmode(_fileno(stdin), _O_BINARY) == -1)
		{
			cerr << "error switching stdin to binary mode: " << strerror(errno);
		}
		else
		{
			cin.seekg(0, cin.end);
			auto len = cin.tellg();
			cin.seekg(0, cin.beg);
			if (len > 0)
			{
				vector<char> v(static_cast<size_t>(len), 0);
				cin.read(&v[0], len);
				resultData.swap(v);
				return 0;
			}
		}
		cerr << "error: stdin contains no data" << endl;
	}

	cerr << "error: unable to open stdin" << endl;
	return -1;
}

static int WriteOurput(vector<char> const& resultData)
{
	ofstream outfile(fileName, ios::binary | ios::trunc);
	if (!outfile.good())
	{
		cerr << "error: cannot open output file '" << fileName << "' for writing" << endl;
		return -1;
	}
	outfile.write(&resultData[0], resultData.size());
	outfile.close();

	if (_setmode(_fileno(stdout), _O_BINARY) == -1)
	{
		cerr << "error switching stdout to binary mode: " << strerror(errno);
		return -1;
	}
	cout.write(&resultData[0], resultData.size());
	cerr << resultData.size() << " bytes written to '" << fileName << "'" << endl;
	
	return 0;
}

int main()
{
    ChildProc proc;
    auto msg = proc.ReadMessageFromParent();
    
    vector<char> resultData;

	//auto retVal = ReadInput(resultData);
	//if (0 == retVal)
	{
		// do some program logic here...
		cerr << "... doing program logic ..." << endl;
           		
		cout << "nonsens" << flush;
        Sleep(50);
        cout << "newline" << endl;
        cout << "newline2" << endl;
        Sleep(50);
        cerr << "errline" << endl;
        cout << "newline3" << endl;
        cerr << "errline2" << endl;
		Sleep(3000);
		
		return WriteOurput(resultData);
	}

	return 0; //retVal;
}

