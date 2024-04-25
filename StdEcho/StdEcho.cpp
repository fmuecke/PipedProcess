// This program is used to test the PipedProcess class
// It reads data from stdin and writes it to stdout
// It is used to test the communication between the parent and child processes
// The parent process writes data to the child process stdin and reads data from the child process stdout

#include <Windows.h>
#include <string>

int main()
{
    auto stdOutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    auto stdInHandle = ::GetStdHandle(STD_INPUT_HANDLE);
    auto stdErrHandle = ::GetStdHandle(STD_ERROR_HANDLE);

    if (INVALID_HANDLE_VALUE == stdOutHandle || 
        INVALID_HANDLE_VALUE == stdInHandle  || 
        INVALID_HANDLE_VALUE == stdErrHandle)
	{
		return 1;
	}

    //::Sleep(50); // wait for the parent process to write data to the child process stdin

    DWORD retCode{ 0 }; 

    // peek the data from stdin
    DWORD bytesAvailable{ 0 };
    auto success = ::PeekNamedPipe(stdInHandle, nullptr, 0, nullptr, &bytesAvailable, nullptr);
    if (!success || 0 == bytesAvailable)
	{
		DWORD bytesWritten{ 0 };
		std::string message{ "no data on std input received" };
		::WriteFile(stdErrHandle, message.c_str(), static_cast<int>(message.length()), &bytesWritten, nullptr);

        retCode = 1;
	}
    else
    {

        for (;;)
        {
            // if stdInHandle has data
            const DWORD bufferSize{ 4096 };
            std::string input;
            input.resize(bufferSize);

            // read data from stdin
            DWORD bytesRead{ 0 };
            auto success = ::ReadFile(stdInHandle, &input[0], bufferSize, &bytesRead, nullptr);
            if (!success || 0 == bytesRead)
            {
                break;
            }

            // write read data to stdout
            DWORD bytesWritten{ 0 };
            success = ::WriteFile(stdOutHandle, input.c_str(), bytesRead, &bytesWritten, nullptr);
            if (!success)
            {
                break;
            }
        }
    }

    ::CloseHandle(stdOutHandle);
    ::CloseHandle(stdInHandle);
    ::CloseHandle(stdErrHandle);
    
    return retCode;
}

