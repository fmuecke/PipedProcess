// This file is part of the PipedProcess project 
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

#pragma once

#include <Windows.h>
#include <vector>
#include <array>

struct StdPipes
{
	StdPipes()  {}

	~StdPipes()
	{
        Close(stdin_write);
        Close(stdout_write);
        Close(stderr_write);
        Close(stdin_read);
        Close(stdout_read);
        Close(stderr_read);
    }

	bool Create()
	{
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		m_sa.bInheritHandle = true;  // pipe handles should be inherited
		m_sa.lpSecurityDescriptor = nullptr;

		if (!::CreatePipe(&stdout_read, &stdout_write, &m_sa, 0) ||
			!::CreatePipe(&stderr_read, &stderr_write, &m_sa, 0) ||
			!::CreatePipe(&stdin_read, &stdin_write, &m_sa, 0))
		{
			return false;
		}

		// read (out/err) and write (in) should not be inheritable
		if (!::SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0) ||
			!::SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0) ||
			!::SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0))
		{
			return false;
		}

		return true;
	}

	static void Close(HANDLE& h)
	{
		if (h != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

    static bool HasData(HANDLE hPipe)
    {
        std::array<char, 256> buffer = {};
        DWORD bytesRead = 0;
        DWORD bytesAvailable = 0;
        ::PeekNamedPipe(hPipe, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL);
        return bytesAvailable > 0 || bytesRead > 0;
    }
	
	static std::vector<char> Read(HANDLE pipe)
	{
		std::array<char, 4096> buffer = {};
		std::vector<char> result;

		using std::begin;
		using std::end;

		for (;;)
		{
			DWORD bytesRead = 0;
			bool success = ::ReadFile(pipe, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, NULL) != 0;
			if (!success || bytesRead == 0)
			{
				if (result.empty())
				{
					//std::cerr << "error reading from pipe: code " << GetLastError() << std::endl;
                    auto err = ::GetLastError();
                    throw std::system_error(std::error_code(err, std::system_category()));
				}

				break;
			}

			std::move(begin(buffer), begin(buffer) + bytesRead, std::back_inserter(result));
		}

	    return result;
	}

    static void Write(HANDLE pipe, const char* pBytes, int len)
    {
        DWORD bytesWritten = 0;
        if (!::WriteFile(pipe, pBytes, len, &bytesWritten, NULL))
        {
            auto err = ::GetLastError();
            throw std::system_error(std::error_code(err, std::system_category()));
        }
    }

	StdPipes(const StdPipes&) = delete;
	StdPipes& operator=(const StdPipes&) = delete;

	HANDLE stderr_read { INVALID_HANDLE_VALUE };
	HANDLE stderr_write { INVALID_HANDLE_VALUE };
	HANDLE stdout_read { INVALID_HANDLE_VALUE };
	HANDLE stdout_write { INVALID_HANDLE_VALUE };
	HANDLE stdin_read { INVALID_HANDLE_VALUE };
	HANDLE stdin_write { INVALID_HANDLE_VALUE };

private:
	SECURITY_ATTRIBUTES m_sa;
};