#include "doctest/doctest.h"
#include "../PipedProcess/StdPipe.h"
#include <string>
#include <thread>
#include <future>

TEST_CASE("Has valid handles after construction")
{
    StdPipe pipe;
    CHECK(pipe.GetReadHandle() != INVALID_HANDLE_VALUE);
    CHECK(pipe.GetWriteHandle() != INVALID_HANDLE_VALUE);
    CHECK(pipe.GetReadHandle() != pipe.GetWriteHandle());
}

TEST_CASE("Has invalid handles after closing")
{
    StdPipe pipe;
    pipe.CloseReadHandle();
    pipe.CloseWriteHandle();
    CHECK(pipe.GetReadHandle() == INVALID_HANDLE_VALUE);
    CHECK(pipe.GetWriteHandle() == INVALID_HANDLE_VALUE);
}

TEST_CASE("Write and read basic functionality")
{
    StdPipe pipe;
    std::string testString = "Hello World!";
    
    pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
    pipe.CloseWriteHandle();

    auto readString = pipe.Read();
    CHECK(readString == testString);
}

TEST_CASE("HasData functionality")
{
    StdPipe pipe;
    CHECK_FALSE(pipe.HasData());

    std::string testString = "Hello World!";
    pipe.Write(testString.c_str(), static_cast<int>(testString.size()));

    CHECK(pipe.HasData());
}

TEST_CASE("Close handles functionality")
{
    StdPipe pipe;
    CHECK(pipe.GetReadHandle() != INVALID_HANDLE_VALUE);
    CHECK(pipe.GetWriteHandle() != INVALID_HANDLE_VALUE);

    pipe.CloseReadHandle();
    CHECK(pipe.GetReadHandle() == INVALID_HANDLE_VALUE);

    pipe.CloseWriteHandle();
    CHECK(pipe.GetWriteHandle() == INVALID_HANDLE_VALUE);
}

TEST_CASE("Write and read large data")
{
    StdPipe pipe;
    std::string testString(4096, 'a'); // Create 4KB of data
    
    // Use async to avoid deadlock when buffer fills
    auto writeTask = std::async(std::launch::async, [&]() {
        pipe.Write(testString.c_str(), static_cast<int>(testString.size()));
        pipe.CloseWriteHandle();
    });
    
    auto readString = pipe.Read();
    writeTask.wait();
    
    CHECK(readString == testString);
}

TEST_CASE("Write and read multiple times")
{
    StdPipe pipe;
    std::string testString1 = "Hello World!";
    std::string testString2 = "Goodbye, World!";
    
    pipe.Write(testString1.c_str(), static_cast<int>(testString1.size()));
    pipe.Write(testString2.c_str(), static_cast<int>(testString2.size()));
    pipe.CloseWriteHandle();

    auto readString = pipe.Read();
    CHECK(readString == testString1 + testString2);
}

TEST_CASE("Binary data handling")
{
    StdPipe pipe;
    
    // Create binary data with null bytes
    std::string binaryData = "Hello\0World\0Binary\0Data";
    binaryData = std::string(binaryData.c_str(), 23); // Include null bytes
    
    pipe.Write(binaryData.c_str(), static_cast<int>(binaryData.size()));
    pipe.CloseWriteHandle();
    
    auto readData = pipe.Read();
    CHECK(readData.size() == binaryData.size());
    CHECK(readData == binaryData);
}

TEST_CASE("Empty data handling")
{
    StdPipe pipe;
    pipe.CloseWriteHandle();
    
    auto result = pipe.Read();
    CHECK(result.empty());
}

TEST_CASE("Error handling with invalid handles")
{
    StdPipe pipe;
    pipe.CloseReadHandle();
    pipe.CloseWriteHandle();
    
    // Writing to closed pipe should throw an exception
    std::string testString = "Hello World!";
    CHECK_THROWS(pipe.Write(testString.c_str(), static_cast<int>(testString.size())));
    
    // Reading from closed pipe should also throw an exception
    CHECK_THROWS(pipe.Read());
}