#include "PipedProcess/PipedProcess.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	string buffer(1000000, 'Ö');
    PipedProcess proc;

    assert(proc.Run("c:\\windows\\system32\\cmd.exe", "/c exit 0") == NO_ERROR);
    assert(!proc.HasStdErrData());
    assert(!proc.HasStdOutData());

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
		cerr << buffer.c_str() << endl;
	}

    return errorCode;
}

