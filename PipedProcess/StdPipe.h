// This file is part of the PipedProcess project 
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

// This class is used to create a pipe for the standard input, output and error streams of a child process.
// It is used by the PipedProcess class to create the pipes for the child process.

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>
#include <string>
#include <system_error>
#include <iterator>
#include <algorithm>
#include <limits>

struct StdPipe
{
    StdPipe()  
    {
        _securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        _securityAttributes.bInheritHandle = true;  // pipe handles should be inherited
        _securityAttributes.lpSecurityDescriptor = nullptr;

        if (!::CreatePipe(&_readHandle, &_writeHandle, &_securityAttributes, 0))
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

	void CloseReadHandle()  { close(_readHandle); }
	void CloseWriteHandle() { close(_writeHandle); }

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
		const int readBufferSize{ 4096 };

		std::array<char, readBufferSize> buffer{};
		std::string result;

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

			result.append(buffer.data(), bytesRead);
		}

	    return result;
	}

    // Writes data to the pipe
    // To signal finish writing to the pipe, call CloseWriteHandle()
    void Write(const char* pBytes, size_t len) const
    {
        if (!pBytes && len > 0)
        {
            throw std::invalid_argument("pBytes cannot be null when len > 0");
        }

        const char* currentPos = pBytes;
        size_t remainingBytes = len;

        while (remainingBytes > 0)
        {
            DWORD bytesWritten{ 0 };
            DWORD bytesToWrite = static_cast<DWORD>((std::min)(remainingBytes, static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
            
            if (!::WriteFile(_writeHandle, currentPos, bytesToWrite, &bytesWritten, NULL))
            {
                auto err = ::GetLastError();
                throw std::system_error(err, std::system_category());
            }

            if (bytesWritten == 0)
            {
                // This shouldn't happen if WriteFile succeeded, but better be safe
                throw std::system_error(ERROR_WRITE_FAULT, std::system_category(), "WriteFile succeeded but wrote 0 bytes");
            }

            currentPos += bytesWritten;
            remainingBytes -= bytesWritten;
        }
    }

    HANDLE GetReadHandle() const { return _readHandle; }
    HANDLE GetWriteHandle() const { return _writeHandle; }

	StdPipe(const StdPipe&) = delete; // non-copyable
	StdPipe& operator=(const StdPipe&) = delete; // non-assignable

private:
    
    static void close(HANDLE& h)
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
	SECURITY_ATTRIBUTES _securityAttributes {0};
#else 
	SECURITY_ATTRIBUTES _securityAttributes;
#endif
};