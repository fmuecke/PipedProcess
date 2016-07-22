// This file is part of the PipedProcess project.
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

#pragma once

#include "StdPipes.h"
#include "windows.h"
#include <vector>
#include <algorithm>
#include <functional>

#ifdef _DEBUG
#include <string>
#include <system_error>
#include <iostream>
#endif

using namespace std::placeholders;

typedef std::function<BOOL(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION)> createProcess_t;

class PipedProcess
{
public:
	struct EmptyAbortEvent
	{
		bool IsSet() const { return false; }
	};

	PipedProcess()
	{}

	~PipedProcess()
	{}

	DWORD Run(const char* program, const char* arguments)
	{
		return Run(program, arguments, false);
	}

	DWORD Run(const char* program, const char* arguments, const bool hideWindow)
	{
		EmptyAbortEvent event;
		return Run(program, arguments, event, hideWindow);
	}

	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments)
	{
		return RunAs(token, program, arguments, false);
	}

	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, const bool hideWindow)
	{
		EmptyAbortEvent event;
		return RunAs(token, program, arguments, event, hideWindow);
	}

	template<class T>
	DWORD Run(const char* program, const char* arguments, T& abortEvent)
	{
		return Run(program, arguments, abortEvent, false);
	}

	template<class T>
	DWORD Run(const char* program, const char* arguments, T& abortEvent, const bool hideWindow)
	{
		return Run(program, arguments, abortEvent, hideWindow, CreateProcessA);
	}

	template<class T>
	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, T& abortEvent)
	{
		return RunAs(token, program, arguments, abortEvent, false);
	}

	template<class T>
	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, T& abortEvent, const bool hideWindow)
	{
		auto createProcess = std::bind(CreateProcessAsUserA, token, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10);
		return Run(program, arguments, abortEvent, hideWindow, createProcess);
	}

	void SetStdInData(const char* pData, size_t len)
	{
		std::vector<char> tmp(pData, pData + len);
		stdInBytes.swap(tmp);
	}

	bool HasStdOutData() const { return !stdOutBytes.empty(); }
	bool HasStdErrData() const { return !stdErrBytes.empty(); }

	std::vector<char> FetchStdOutData()
	{
		std::vector<char> ret;
		ret.swap(stdOutBytes);
		return ret;
	}

	std::vector<char> FetchStdErrData()
	{
		std::vector<char> ret;
		ret.swap(stdErrBytes);
		return ret;
	}
private:
	std::vector<char> stdInBytes;
	std::vector<char> stdOutBytes;
	std::vector<char> stdErrBytes;

	template<class T>
	DWORD Run(const char* program, const char* arguments, T& abortEvent, const bool hideWindow, createProcess_t createProcess)
	{
		// arguments need to be in a non const array for the API call
		auto len = strlen(arguments) + 1;
		std::vector<char> args(static_cast<int>(len), 0);
		std::copy(arguments, arguments + len, stdext::checked_array_iterator<char*>(&args[0], len));

		StdPipes pipes;
		pipes.Create();

		STARTUPINFOA startInfo;
        ::SecureZeroMemory(&startInfo, sizeof(startInfo));
        startInfo.cb = sizeof(startInfo);
        startInfo.hStdInput = stdInBytes.empty() ? 0 : pipes.in_read;
        startInfo.hStdOutput = pipes.out_write;
		startInfo.hStdError = pipes.err_write;
        startInfo.dwFlags |= STARTF_USESTDHANDLES;

		if (hideWindow)
			HideWindow(startInfo);

		PROCESS_INFORMATION procInfo;
        ::SecureZeroMemory(&procInfo, sizeof(procInfo));

		// Create the child process.
		bool success = createProcess(
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

		DWORD exitCode = ERROR_INVALID_FUNCTION;
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
				if (!::WriteFile(pipes.in_write, &stdInBytes[0], (DWORD)stdInBytes.size(), &bytesWritten, NULL))
				{
					exitCode = ::GetLastError();
#ifdef _DEBUG
					std::error_code code(exitCode, std::system_category());
					std::cerr << "error: unable to write input stream: "
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
				if (!pipes.HasData(pipes.out_read))
				{
					if (abortEvent.IsSet())
					{
						::TerminateProcess(procInfo.hProcess, HRESULT_CODE(E_ABORT));
						break;
					}
					continue;
				}

                pipes.Read(pipes.out_read, stdOutBytes, StdPipes::ReadMode::append);
				pipes.Read(pipes.err_read, stdErrBytes, StdPipes::ReadMode::append);
            }
            
			pipes.Read(pipes.out_read, stdOutBytes, StdPipes::ReadMode::append);
			pipes.Read(pipes.err_read, stdErrBytes, StdPipes::ReadMode::append);
			
			::GetExitCodeProcess(procInfo.hProcess, &exitCode);
			::CloseHandle(procInfo.hProcess);
			::CloseHandle(procInfo.hThread);
		}

		stdInBytes.clear();

		// note: all pipe handles will be closed by the std pipe wrapper class

		return exitCode;
	}

	void HideWindow(STARTUPINFOA& startInfo)
	{
		startInfo.dwFlags |= STARTF_USESHOWWINDOW;
		startInfo.wShowWindow |= SW_HIDE;
	}
};


