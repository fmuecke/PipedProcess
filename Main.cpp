#include <iostream>
#include <vector>
#include "CapturedProcess/CapturedProcess.h"

using namespace std;

int main(int argc, char *argv[])
{
	vector<char> buffer(1000000, 'Ö');
	CapturedProcess capturedProc;
	capturedProc.SetStdInData(&buffer[0], buffer.size());
	DWORD errorCode = capturedProc.Run("DemoChildProc.exe", "");
	if (errorCode == NO_ERROR)
	{
		capturedProc.FetchStdOutData(buffer);
		cout << buffer.size() << " bytes received" << endl;
	}

	return 0;
}

