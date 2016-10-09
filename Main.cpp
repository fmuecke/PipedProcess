#include "PipedProcess/PipedProcess.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[])
{
	vector<char> buffer(1000000, 'Ö');
    PipedProcess proc;
	proc.SetStdInData(&buffer[0], buffer.size());
	DWORD errorCode = proc.Run("DemoChildProc.exe", "");

	if (errorCode == NO_ERROR)
	{
		buffer = proc.FetchStdOutData();
        assert(buffer.size() == 1000036);
		cout << buffer.size() << " bytes received" << endl;
	}
	
    if (proc.HasStdErrData())
	{
		buffer = proc.FetchStdErrData();
        assert(buffer.size() == 84);
		cerr << string(&buffer[0], buffer.size()).c_str() << endl;
	}

    return errorCode;
}

