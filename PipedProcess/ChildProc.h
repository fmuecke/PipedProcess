#pragma once

#include "StdPipe.h"
#include <string>
#include <array>

class ChildProc
{
public:
    static const DWORD Signature { 0x4D534724 }; // "MSG$"
 
    ChildProc()
    {
    }

	~ChildProc()
	{
		if (_readHandle != INVALID_HANDLE_VALUE) ::CloseHandle(_readHandle);
		if (_writeHandle != INVALID_HANDLE_VALUE) ::CloseHandle(_writeHandle);
	}

    std::string ReadMessageFromParent() const
    {
        return ReadMessage(_readHandle);
    }

    static std::string ReadMessage(HANDLE readHandle)
    {
        std::array<DWORD, 2> header = {};
        DWORD bytesRead = 0;
        bool success = !!::ReadFile(readHandle, header.data(), sizeof(header), &bytesRead, nullptr);
        if (!success || bytesRead != sizeof(header))
        {
            auto err = ::GetLastError();
            throw std::system_error(std::error_code(err, std::system_category()));
        }

        if (header[0] != Signature)
        {
            throw std::runtime_error("The message is invalid.");
        }

        auto result = std::string(header[1], 0);
        success = !!::ReadFile(readHandle, &result[0], static_cast<DWORD>(result.size()), &bytesRead, nullptr);
        if (!success || bytesRead == 0 || bytesRead != header[1])
        {
            auto err = ::GetLastError();
            throw std::system_error(std::error_code(err, std::system_category()));
        }

        return result;
    }
    
    void WriteMessageToParent(std::string&& msg) const
    {
        WriteMessage(_writeHandle, std::move(msg));
    }

    static void WriteMessage(HANDLE writeHandle, std::string&& msg)
    {
        std::array<DWORD, 2> header = {Signature, static_cast<DWORD>(msg.size()) };
        DWORD bytesWritten = 0;
        bool success = !!::WriteFile(writeHandle, &header[0], sizeof(header), &bytesWritten, nullptr);
        if (!success || bytesWritten != sizeof(header))
        {
            auto err = ::GetLastError();
            throw std::system_error(std::error_code(err, std::system_category()));
        }

        success = !!::WriteFile(writeHandle, &msg[0], static_cast<DWORD>(msg.size()), &bytesWritten, nullptr);
        if (!success || bytesWritten != msg.size())
        {
            auto err = ::GetLastError();
            throw std::system_error(std::error_code(err, std::system_category()));
        }
    }

private:
    HANDLE _readHandle { ::GetStdHandle(STD_INPUT_HANDLE) };
    HANDLE _writeHandle { ::GetStdHandle(STD_OUTPUT_HANDLE) };
};