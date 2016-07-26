// This file is part of the PipedProcess project.
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

#pragma once

#include "StdPipes.h"
#include "windows.h"
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#include <string>
#include <system_error>
#include <iostream>
#endif

class PipedProcess
{
public:
    enum class WindowMode { Visible = 0, Hidden = 1 };

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
        EmptyAbortEvent abortEvent;
        auto userToken = nullptr;
        return Run(program, arguments, abortEvent, WindowMode::Visible, userToken);
	}

	DWORD Run(const char* program, const char* arguments, WindowMode windowMode)
	{
		EmptyAbortEvent abortEvent;
        auto userToken = nullptr;
		return Run(program, arguments, abortEvent, windowMode, userToken);
	}

	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments)
	{
        EmptyAbortEvent abortEvent;
        return RunAs(token, program, arguments, abortEvent, WindowMode::Visible);
	}

	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, WindowMode windowMode)
	{
		EmptyAbortEvent abortEvent;
		return Run(program, arguments, abortEvent, windowMode, &token);
	}

	template<class T>
	DWORD Run(const char* program, const char* arguments, T& abortEvent)
	{
		return Run(program, arguments, abortEvent, WindowMode::Visible);
	}

	template<class T>
	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, T& abortEvent, WindowMode windowMode)
	{
		return Run(program, arguments, abortEvent, windowMode, &token);
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
    template<class T>
    DWORD Run(const char* program, const char* arguments, T& abortEvent, WindowMode windowMode, HANDLE const* pUserAccessToken)
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

        SetWindowFlags(startInfo, windowMode);

        PROCESS_INFORMATION procInfo;
        ::SecureZeroMemory(&procInfo, sizeof(procInfo));

        // Create the child process.
        bool success = false;
        if (pUserAccessToken)
        {
            success = ::CreateProcessAsUserA(
                *pUserAccessToken,
                program,          // executable
                &args[0],         // argumenst (writable buffer)
                NULL,             // process security attributes
                NULL,             // primary thread security attributes
                TRUE,             // handles are inherited
                0,                // creation flags
                NULL,             // use parent's environment
                NULL,             // use parent's current directory
                &startInfo,       // STARTUPINFO
                &procInfo) != 0;  // receives PROCESS_INFORMATION
        }
        else
        {
            success = ::CreateProcessA(
                program,          // executable
                &args[0],         // argumenst (writable buffer)
                NULL,             // process security attributes
                NULL,             // primary thread security attributes
                TRUE,             // handles are inherited
                0,                // creation flags
                NULL,             // use parent's environment
                NULL,             // use parent's current directory
                &startInfo,       // STARTUPINFO
                &procInfo) != 0;  // receives PROCESS_INFORMATION
        }

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

    void SetWindowFlags(STARTUPINFOA& startInfo, WindowMode mode)
    {
        if (mode == WindowMode::Hidden)
        {
            startInfo.dwFlags |= STARTF_USESHOWWINDOW;
            startInfo.wShowWindow |= SW_HIDE;
        }
    }

    std::vector<char> stdInBytes;
	std::vector<char> stdOutBytes;
	std::vector<char> stdErrBytes;
};


