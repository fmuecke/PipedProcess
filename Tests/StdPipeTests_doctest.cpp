#include "doctest/doctest.h"
#include "../PipedProcess/StdPipe.h"
#include <string>
#include <thread>
#include <future>

TEST_CASE("StdPipe - Has valid handles after construction")
{
    StdPipe pipe;
    CHECK(pipe.GetReadHandle() != INVALID_HANDLE_VALUE);
    CHECK(pipe.GetWriteHandle() != INVALID_HANDLE_VALUE);
    CHECK(pipe.GetReadHandle() != pipe.GetWriteHandle());
}

TEST_CASE("StdPipe - Has invalid handles after closing")
{
    StdPipe pipe;
    pipe.CloseReadHandle();
    pipe.CloseWriteHandle();
    CHECK(pipe.GetReadHandle() == INVALID_HANDLE_VALUE);
    CHECK(pipe.GetWriteHandle() == INVALID_HANDLE_VALUE);
}

TEST_CASE("StdPipe - Write and read basic functionality")
{
    StdPipe pipe;
    std::string testString = "Hello, World!";
    pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
    pipe.CloseWriteHandle();

    auto readString = pipe.Read();
    CHECK(readString == testString);
}

TEST_CASE("StdPipe - HasData functionality")
{
    StdPipe pipe;
    CHECK_FALSE(pipe.HasData());

    std::string testString = "Hello, World!";
    pipe.Write(testString.c_str(), static_cast<int>(testString.size()));

    CHECK(pipe.HasData());
}

TEST_CASE("StdPipe - Close handles functionality")
{
    StdPipe pipe;
    CHECK(pipe.GetReadHandle() != INVALID_HANDLE_VALUE);
    CHECK(pipe.GetWriteHandle() != INVALID_HANDLE_VALUE);

    pipe.CloseReadHandle();
    CHECK(pipe.GetReadHandle() == INVALID_HANDLE_VALUE);

    pipe.CloseWriteHandle();
    CHECK(pipe.GetWriteHandle() == INVALID_HANDLE_VALUE);
}

TEST_CASE("StdPipe - Write and read large data")
{
    StdPipe pipe;
    
    SUBCASE("4KB data")
    {
        std::string testString(4096, 'a');
        
        // Use threading to avoid deadlock when buffer is full
        std::thread writer([&]()
        {
            pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
            pipe.CloseWriteHandle();
        });
        
        auto readString = pipe.Read();
        writer.join();
        
        CHECK(readString == testString);
    }
    
    SUBCASE("8KB data")
    {
        std::string testString(8192, 'b');
        
        std::thread writer([&]()
        {
            pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
            pipe.CloseWriteHandle();
        });
        
        auto readString = pipe.Read();
        writer.join();
        
        CHECK(readString == testString);
    }
}

TEST_CASE("StdPipe - Write and read multiple times")
{
    StdPipe pipe;
    std::string testString1 = "Hello, World!";
    std::string testString2 = "Goodbye, World!";
    
    pipe.Write(testString1.c_str(), static_cast<int>(testString1.size()));
    pipe.Write(testString2.c_str(), static_cast<int>(testString2.size()));
    pipe.CloseWriteHandle();
    
    // Writing to a closed pipe should throw an exception
    CHECK_THROWS_AS(pipe.Write("abc", 3), std::system_error);

    auto readString = pipe.Read(); 
    CHECK(readString == (testString1 + testString2));
}

TEST_CASE("StdPipe - Multithreaded access")
{
    StdPipe pipe;
    std::string testString = "Hello, World!";
    std::string readResult;
    std::exception_ptr writeException = nullptr;
    std::exception_ptr readException = nullptr;

    std::thread writerThread([&]()
    {
        try
        {
            pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
            pipe.CloseWriteHandle();
        }
        catch (...)
        {
            writeException = std::current_exception();
        }
    });

    std::thread readerThread([&]()
    {
        try
        {
            readResult = pipe.Read();
        }
        catch (...)
        {
            readException = std::current_exception();
        }
    });

    writerThread.join();
    readerThread.join();

    // Check that no exceptions occurred
    CHECK(writeException == nullptr);
    CHECK(readException == nullptr);
    CHECK(readResult == testString);
}

TEST_CASE("StdPipe - Binary data handling")
{
    StdPipe pipe;
    
    // Test with binary data including null bytes
    std::vector<char> binaryData = {static_cast<char>(0x00), static_cast<char>(0xFF), static_cast<char>(0x01), static_cast<char>(0x02), static_cast<char>(0xFE), static_cast<char>(0x00), static_cast<char>(0x41), static_cast<char>(0x42)};
    std::string testString(binaryData.begin(), binaryData.end());
    
    std::thread writer([&]()
    {
        pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
        pipe.CloseWriteHandle();
    });
    
    auto readString = pipe.Read();
    writer.join();
    
    CHECK(readString.size() == testString.size());
    CHECK(readString == testString);
}

TEST_CASE("StdPipe - Empty data handling")
{
    StdPipe pipe;
    
    pipe.CloseWriteHandle(); // Close without writing anything
    
    auto readString = pipe.Read();
    CHECK(readString.empty());
}