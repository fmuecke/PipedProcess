// This file is part of the PipedProcess project 
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

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

    bool HasData() const
    {
        std::array<char, 256> buffer = {};
        DWORD bytesRead = 0;
        DWORD bytesAvailable = 0;
        ::PeekNamedPipe(_readHandle, &buffer[0], static_cast<DWORD>(buffer.size()), &bytesRead, &bytesAvailable, NULL);
        return bytesAvailable > 0 || bytesRead > 0;
    }
	
	std::string Read()
	{
		std::array<char, 4096> buffer = {};
		std::string result;

		using std::begin;
		using std::end;

		for (;;)
		{
			DWORD bytesRead = 0;
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
            else if (bytesRead == 0)
            {
                break;
            }

			std::move(begin(buffer), begin(buffer) + bytesRead, std::back_inserter(result));
		}

	    return result;
	}

    void Write(const char* pBytes, int len) const
    {
        DWORD bytesWritten = 0;
        if (!::WriteFile(_writeHandle, pBytes, len, &bytesWritten, NULL))
        {
            auto err = ::GetLastError();
            throw std::system_error(err, std::system_category());
        }
    }

    HANDLE GetReadHandle() const { return _readHandle; }
    HANDLE GetWriteHandle() const { return _writeHandle; }

	StdPipe(const StdPipe&) = delete;
	StdPipe& operator=(const StdPipe&) = delete;

private:
    void Close(HANDLE& h)
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