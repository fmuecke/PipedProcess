// This file is part of the PipedProcess project 
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

// This class is used to create a pipe for the standard input, output and error streams of a child process.
// The class is used by the PipedProcess class to create the pipes for the child process.

#pragma once

#include <Windows.h>
#include <array>
#include <string>
#include <system_error>
#include <iterator>

struct StdPipe
{
    StdPipe()  
    {
        m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        m_sa.bInheritHandle = true;  // pipe handles should be inherited
        m_sa.lpSecurityDescriptor = nullptr;

        if (!::CreatePipe(&_readHandle, &_writeHandle, &m_sa, 0))
        {
            auto err = ::GetLastError();
            throw std::system_error(err, std::system_category());
        }
    }

	~StdPipe()
	{
        CloseWriteHandle();
        CloseReadHandle();
    }

	void CloseReadHandle()  { Close(_readHandle); }
	void CloseWriteHandle() { Close(_writeHandle); }

    // Returns true if there is data available to read from the pipe
    bool HasData() const
    {
        DWORD bytesAvailable{ 0 };
        auto success = ::PeekNamedPipe(_readHandle, nullptr, 0, nullptr, &bytesAvailable, nullptr);
        return success && bytesAvailable > 0;
    }
	
    // Reads data from the pipe and returns it as a string 
    // In order to signal finish reading from the pipe, the child process should close the write handle
	std::string Read() const
	{
		std::array<char, 4096> buffer{};
		std::string result;

		using std::begin;
		using std::end;

        for(;;)
		{
            DWORD bytesRead{ 0 };
			bool success = ::ReadFile(_readHandle, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, NULL) != 0;
			if (!success)
			{
                auto err = ::GetLastError();
                if (err != ERROR_BROKEN_PIPE)
                {
                    throw std::system_error(err, std::system_category());
				}

				break;
			}
            else if (0 == bytesRead)
            {
                break;
            }

			std::move(begin(buffer), begin(buffer) + bytesRead, std::back_inserter(result));
		}

	    return result;
	}

    // Writes data to the pipe
    // To signal finish writing to the pipe, call CloseWriteHandle()
    void Write(const char* pBytes, int len) const
    {
        DWORD bytesWritten{ 0 };
        if (!::WriteFile(_writeHandle, pBytes, len, &bytesWritten, NULL))
        {
            auto err = ::GetLastError();
            throw std::system_error(err, std::system_category());
        }
    }

    // Returns the read and write handles of the pipe
    HANDLE GetReadHandle() const { return _readHandle; }
    HANDLE GetWriteHandle() const { return _writeHandle; }


	StdPipe(const StdPipe&) = delete; // non-copyable
	StdPipe& operator=(const StdPipe&) = delete; // non-assignable

private:
    
    // Closes the handle if it is not INVALID_HANDLE_VALUE
    static void Close(HANDLE& h)
	{
		if (h != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

	HANDLE _readHandle { INVALID_HANDLE_VALUE };
	HANDLE _writeHandle { INVALID_HANDLE_VALUE };
#if _MSC_VER > 1800 // VS 2015 and above	
	SECURITY_ATTRIBUTES m_sa {0};
#else 
	SECURITY_ATTRIBUTES m_sa;
#endif
};