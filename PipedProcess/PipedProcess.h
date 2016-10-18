// This file is part of the PipedProcess project.
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

#pragma once

#include "StdPipe.h"
#include "windows.h"
#include <algorithm>
#include <future>
#include <string>
#include <vector>

#ifdef _DEBUG
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
		std::string tmp(pData, pData + len);
		stdInBytes.swap(tmp);
	}

	bool HasStdOutData() const { return !stdOutBytes.empty(); }
	bool HasStdErrData() const { return !stdErrBytes.empty(); }

	std::string FetchStdOutData()
	{
		std::string ret;
		ret.swap(stdOutBytes);
		return ret;
	}

	std::string FetchStdErrData()
	{
		std::string ret;
		ret.swap(stdErrBytes);
		return ret;
	}

private:
    template<class T>
    DWORD Run(const char* program, const char* arguments, T& abortEvent, HANDLE const* pUserAccessToken)
    {
        // arguments need to be in a non const array for the API call
        const auto len = strlen(arguments) + 1;
        std::vector<char> args(static_cast<int>(len), 0);
        std::copy(arguments, arguments + len, stdext::checked_array_iterator<char*>(&args[0], len));

        try
        {
            // Note: Raymond Chen ("The Old New Thing") has some thoughtful insights about pipes:
			// "Be careful when redirecting both a process’s stdin and stdout to pipes, for you can easily deadlock"
			// https://blogs.msdn.microsoft.com/oldnewthing/20110707-00/?p=10223
			StdPipe stdInPipe;
            StdPipe stdOutPipe;
            StdPipe stdErrPipe;
        
            // read (out/err) and write (in) should not be inheritable
            ::SetHandleInformation(stdOutPipe.GetReadHandle(), HANDLE_FLAG_INHERIT, 0);
            ::SetHandleInformation(stdErrPipe.GetReadHandle(), HANDLE_FLAG_INHERIT, 0);
            ::SetHandleInformation(stdInPipe.GetWriteHandle(), HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOA startInfo;
            ::SecureZeroMemory(&startInfo, sizeof(startInfo));
            startInfo.cb = sizeof(startInfo);
            startInfo.hStdInput = stdInBytes.empty() ? 0 : stdInPipe.GetReadHandle();
            startInfo.hStdOutput = stdOutPipe.GetWriteHandle();
            startInfo.hStdError = stdErrPipe.GetWriteHandle();
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
                auto msg = std::string("Error creating process '") + program + "': " + GetErrorString(code);
                stdErrBytes = { msg.data(), msg.data() + msg.size() };
                return exitCode;
            }
            else
            {
                stdInPipe.CloseReadHandle();
			    stdOutPipe.CloseWriteHandle();
                stdErrPipe.CloseWriteHandle();
			
			    if (!stdInBytes.empty())
                {
                    try
                    {
                        stdInPipe.Write(stdInBytes.data(), static_cast<DWORD>(stdInBytes.size()));  // cast needed for 64-bit!
                    }
                    catch (std::system_error &e)
                    {
                        auto msg = "Error writing to child's stdin stream: " + GetErrorString(e.code());
                        stdErrBytes = { msg.data(), msg.data() + msg.size() };
                        return e.code().value();
                    }
                }

                stdInPipe.CloseWriteHandle();
                stdInBytes.clear();
            
                // read asynchronously from childs stdout and stderr
                // (unfortunately calling with ptr to member did not work...)
                auto stdOutReader = std::async(std::launch::async, [&stdOutPipe] { return stdOutPipe.Read(); } );
                auto stdErrReader = std::async(std::launch::async, [&stdErrPipe] { return stdErrPipe.Read(); } );

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
                    auto msg = "Error reading from child's stdout stream: " + GetErrorString(e.code());
                    // exception during read operation will be written to stdERR
                    stdErrBytes = { msg.data(), msg.data() + msg.size() };
                    return e.code().value();
                }

                try
                {
                    stdErrBytes = stdErrReader.get();
                }
                catch (std::system_error& e)
                {
                    auto msg = "Error reading from child's stderr stream: " + GetErrorString(e.code());
                    stdErrBytes = { msg.data(), msg.data() + msg.size() };
                                    return e.code().value();
                }
            }

            return exitCode;
        }
        catch (std::system_error& e)
        {
            auto msg = "Error creating std io pipes: " + GetErrorString(e.code());
            stdErrBytes = { msg.data(), msg.data() + msg.size() };
            return e.code().value();
        }
    }  // note: all pipe handles will be closed by the std pipe wrapper class

    static void SetWindowFlags(STARTUPINFOA& startInfo, WindowMode mode)
    {
        if (mode == WindowMode::Hidden)
        {
            startInfo.dwFlags |= STARTF_USESHOWWINDOW;
            startInfo.wShowWindow |= SW_HIDE;
        }
    }

    static std::string GetErrorString(std::error_code const& code)
    {
        // MSVC++ prior to VS2015 does not map all system error codes within std::system_category
        // --> use FormatMessage to retrieve proper error message

#if _MSC_VER <= 1800 // VS 2015 and above	
        if (code.message() == "unknown error")
        {

            if (code.value() == 0) { return std::string(); }

            char* strBuffer = nullptr;
            auto size = ::FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                code.value(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&strBuffer),
                0,
                NULL);

            auto str = std::string(strBuffer, size);
            ::LocalFree(strBuffer);

            return str;
        }
    #endif
        return code.message();
    }


    std::string stdInBytes;
	std::string stdOutBytes;
	std::string stdErrBytes;

    WindowMode windowMode = { WindowMode::Hidden };
};


