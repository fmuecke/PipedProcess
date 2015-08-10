#pragma once
#include <Windows.h>
#include <vector>
#include <array>

struct StdPipes
{
	StdPipes()
		: err_read(INVALID_HANDLE_VALUE)
		, err_write(INVALID_HANDLE_VALUE)
		, out_read(INVALID_HANDLE_VALUE)
		, out_write(INVALID_HANDLE_VALUE)
		, in_read(INVALID_HANDLE_VALUE)
		, in_write(INVALID_HANDLE_VALUE)
	{}

	~StdPipes()
	{
		CloseWriteHandles();
		CloseReadHandles();
	}

	void CloseReadHandles()
	{
		Close(in_read);
		Close(out_read);
		Close(err_read);
	}

	void CloseWriteHandles()
	{
		Close(in_write);
		Close(out_write);
		Close(err_write);
	}

	bool Create()
	{
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		m_sa.bInheritHandle = true;  // pipe handles should be inherited
		m_sa.lpSecurityDescriptor = nullptr;

		if (!::CreatePipe(&out_read, &out_write, &m_sa, 0) ||
			!::CreatePipe(&err_read, &err_write, &m_sa, 0) ||
			!::CreatePipe(&in_read, &in_write, &m_sa, 0))
		{
			return false;
		}

		// read (out/err) and write (in) should not be inheritable
		if (!::SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0) ||
			!::SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0) ||
			!::SetHandleInformation(in_write, HANDLE_FLAG_INHERIT, 0))
		{
			return false;
		}

		return true;
	}

	void Close(HANDLE& h)
	{
		if (h != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	}

	static bool Read(HANDLE hPipe, std::vector<char>& result)
	{
		std::array<char, 4096> buffer;
		std::vector<char> _result;

		for (;;)
		{
			DWORD bytesRead = 0;
			bool success = ::ReadFile(hPipe, &buffer[0], buffer.size(), &bytesRead, NULL) != 0;
			if (!success || bytesRead == 0)
			{
				if (_result.empty())
				{
					//std::cerr << "error reading from pipe: code " << GetLastError() << std::endl;
					return false;
				}

				break;
			}

			_result.insert(std::end(_result), std::begin(buffer), std::begin(buffer) + bytesRead);
		}

		result.swap(_result);

		return true;
	}

	StdPipes(const StdPipes&) = delete;
	StdPipes& operator=(const StdPipes&) = delete;
	
	HANDLE err_read;
	HANDLE err_write;
	HANDLE out_read;
	HANDLE out_write;
	HANDLE in_read;
	HANDLE in_write;

private:
	SECURITY_ATTRIBUTES m_sa;
};
