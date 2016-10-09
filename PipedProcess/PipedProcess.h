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
#include <future>
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

    void SetWindowMode(WindowMode mode)
    {
        windowMode = mode;
    }

	DWORD Run(const char* program, const char* arguments)
	{
		EmptyAbortEvent abortEvent;
        auto userToken = nullptr;
		return Run(program, arguments, abortEvent, userToken);
	}

	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments)
	{
		EmptyAbortEvent abortEvent;
		return Run(program, arguments, abortEvent, &token);
	}

	template<class T>
	DWORD Run(const char* program, const char* arguments, T& abortEvent)
	{
		auto userToken = nullptr;
		return Run(program, arguments, abortEvent, userToken);
	}

	template<class T>
	DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, T& abortEvent)
	{
		return Run(program, arguments, abortEvent, &token);
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
    DWORD Run(const char* program, const char* arguments, T& abortEvent, HANDLE const* pUserAccessToken) noexcept
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
        startInfo.hStdInput = stdInBytes.empty() ? 0 : pipes.stdin_read;
        startInfo.hStdOutput = pipes.stdout_write;
        startInfo.hStdError = pipes.stderr_write;
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
            std::error_code code(exitCode, std::system_category());
            auto msg = std::string("Error creating process '") + program + "': " + code.message();
            stdErrBytes = { msg.data(), msg.data() + msg.size() };
            return exitCode;
        }
        else
        {
            StdPipes::Close(pipes.stdin_read);
			StdPipes::Close(pipes.stdout_write);
			StdPipes::Close(pipes.stderr_write);
			
			if (!stdInBytes.empty())
            {
                try
                {
                    StdPipes::Write(pipes.stdin_write, stdInBytes.data(), stdInBytes.size());
                }
                catch (std::system_error &e)
                {
                    auto msg = "Error writing to child's stdin stream: " + e.code().message();
                    stdErrBytes = { msg.data(), msg.data() + msg.size() };
                    return e.code().value();
                }
            }

            StdPipes::Close(pipes.stdin_write);
            stdInBytes.clear();
            
            // read asynchronously from childs stdout and stderr
            auto stdOutReader = std::async(std::launch::async, StdPipes::Read, pipes.stdout_read);
            auto stdErrReader = std::async(std::launch::async, StdPipes::Read, pipes.stderr_read);

            // check for abort signal or pipe read errors while process is still running
            while (WAIT_TIMEOUT == ::WaitForSingleObject(procInfo.hProcess, 50))
            {
                // If the reader already has a result there must have been an exception --> stop execution
                if (stdOutReader.wait_for(std::chrono::seconds(0)) == std::future_status::ready || 
                    stdErrReader.wait_for(std::chrono::seconds(0)) == std::future_status::ready || 
                    abortEvent.IsSet())
                {
                    ::TerminateProcess(procInfo.hProcess, HRESULT_CODE(E_ABORT));
                    break;
                }
            }

            ::GetExitCodeProcess(procInfo.hProcess, &exitCode);
            ::CloseHandle(procInfo.hProcess);
            ::CloseHandle(procInfo.hThread);

            try
            {
                stdOutBytes = stdOutReader.get();
            }
            catch (std::system_error& e)
            {
                // exception during read operation will be written to stdERR
                auto msg = "Error reading from child's stdout stream: " + e.code().message();
                stdErrBytes = { msg.data(), msg.data() + msg.size() };
                return e.code().value();
            }

            try
            {
                stdErrBytes = stdErrReader.get();
            }
            catch (std::system_error& e)
            {
                auto msg = "Error reading from child's stderr stream: " + e.code().message();
                stdErrBytes = { msg.data(), msg.data() + msg.size() };
                                return e.code().value();
            }
        }

        return exitCode;
        
    }  // note: all pipe handles will be closed by the std pipe wrapper class

    static void SetWindowFlags(STARTUPINFOA& startInfo, WindowMode mode)
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

    WindowMode windowMode = { WindowMode::Hidden };
};


