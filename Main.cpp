// Purpose: Main program to demonstrate the use of the PipedProcess class.

#include "PipedProcess/PipedProcess.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace std;

int main()
{
	string buffer(1000000, 'Ö');
    PipedProcess proc;

	proc.SetStdInData(&buffer[0], buffer.size());
	DWORD errorCode = proc.Run("DemoChildProc.exe", "");

	if (errorCode == NO_ERROR)
	{
		cout << "DemoChildProc.exe ran successfully" << endl;
		buffer = proc.FetchStdOutData();
		assert(buffer.size() == 1000036);
		cout << buffer.size() << " bytes received" << endl;
	}
	else
	{
		cerr << "DemoChildProc.exe failed with error code " << errorCode << endl;
	}
	
    if (proc.HasStdErrData())
	{
		buffer = proc.FetchStdErrData();
		cerr << buffer.c_str() << endl;
		assert(buffer.size() == 84);
	}

    return errorCode;
}

