#include <iostream>
#include <algorithm>
#include <vector>
#include "CapturedProcess.h"

using namespace std;

int main(int argc, char *argv[])
{
	vector<char> buffer(1000000, 'Ö');
	CapturedProcess capturedProc;
	capturedProc.SetStdInData(&buffer[0], buffer.size());
	DWORD errorCode = capturedProc.Run("writeStdData.exe", "");
	if (errorCode == NO_ERROR)
	{
		capturedProc.FetchStdOutData(buffer);
	}

	return 0;
}

