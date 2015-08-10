#pragma once

#include "windows.h"
#include <vector>
#include <algorithm>
#include "StdPipes.h"

#ifdef _DEBUG
#include <string>
#include <system_error>
#include <iostream>
#endif

class CapturedProcess
{
public:
	CapturedProcess()
	{}

	~CapturedProcess()
	{}

	DWORD Run(const char* program, const char* arguments)
	{
		// arguments need to be in a non const array for the API call
		auto len = strlen(arguments) + 1;
		std::vector<char> args(static_cast<int>(len), 0);
		std::copy(arguments, arguments + len,
			stdext::checked_array_iterator<char*>(&args[0], len));

		StdPipes pipes;
		pipes.Create();

		STARTUPINFOA startInfo;
		ZeroMemory(&startInfo, sizeof(STARTUPINFOA));
		startInfo.cb = sizeof(STARTUPINFOA);
		startInfo.hStdOutput = pipes.out_write;
		startInfo.hStdError = pipes.err_write;
		startInfo.dwFlags |= STARTF_USESTDHANDLES;
		if (!stdInBytes.empty())
		{
			startInfo.hStdInput = pipes.in_read;
		}

		PROCESS_INFORMATION procInfo;
		::ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

		// Create the child process. 
		bool success = ::CreateProcessA(
			program,          // executable
			&args[0],      // argumenst (writable buffer)
			NULL,             // process security attributes 
			NULL,             // primary thread security attributes 
			TRUE,             // handles are inherited 
			0,                // creation flags 
			NULL,             // use parent's environment 
			NULL,             // use parent's current directory 
			&startInfo,       // STARTUPINFO 
			&procInfo) != 0;  // receives PROCESS_INFORMATION

		DWORD exitCode = -1;
		if (!success)
		{
			exitCode = ::GetLastError();
#ifdef _DEBUG
			std::error_code code(exitCode, std::system_category());
			std::cerr << "error: unable to create process '"
				<< program
				<< "': " << code.message() << std::endl;
#endif
		}
		else
		{
			if (!stdInBytes.empty())
			{
				DWORD bytesWritten(0);
				if (!::WriteFile(pipes.in_write, &stdInBytes[0], stdInBytes.size(), &bytesWritten, NULL))
				{
					exitCode = ::GetLastError();
#ifdef _DEBUG
					std::error_code code(exitCode, std::system_category());
					std::cerr << "error: unable write input stream: "
						<< code.message() << std::endl;
#endif
				}
				else
				{
					pipes.Close(pipes.in_write);
					pipes.Close(pipes.in_read);
				}
			}

			pipes.CloseWriteHandles();

			while (WAIT_TIMEOUT == ::WaitForSingleObject(procInfo.hProcess, 50))
			{
				pipes.Read(pipes.out_read, stdOutBytes);
				pipes.Read(pipes.err_read, stdErrBytes);
			}
			::GetExitCodeProcess(procInfo.hProcess, &exitCode);
			::CloseHandle(procInfo.hProcess);
			::CloseHandle(procInfo.hThread);
		}

		// To avoid resource leaks in a larger application, close handles explicitly.
		// --> already done in std pipe wrapper
		return exitCode;
	}

	void SetStdInData(const char* pData, size_t len)
	{
		stdInBytes.swap(std::vector<char>(pData, pData + len));
	}
	
	bool HasStdOutData() const { return !stdOutBytes.empty(); }
	bool HasStdErrData() const { return !stdErrBytes.empty(); }
	void FetchStdOutData(std::vector<char>& outData)
	{
		outData.swap(stdOutBytes);
	}

	void FetchStdErrData(std::vector<char>& errData)
	{
		errData.swap(stdErrBytes);
	}

private:
	std::vector<char> stdInBytes;
	std::vector<char> stdOutBytes;
	std::vector<char> stdErrBytes;
};


