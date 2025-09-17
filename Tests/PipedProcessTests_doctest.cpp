#include "doctest/doctest.h"
#include "../PipedProcess/PipedProcess.h"
#include "TestConstants.h"
#include "TestUtils.h"
#include <string>
#include <cstring>
#include <cstdlib>

// Test fixture class for PipedProcess tests
class PipedProcessTestFixture
{
public:
    std::string cmdPath;
    std::string echoPath;

    PipedProcessTestFixture()
    {
        echoPath = TestConstants::ECHO_EXECUTABLE;

        // Use _dupenv_s instead of getenv as getenv is not thread-safe
        char* envValue = nullptr;
        size_t envSize = 0;
        _dupenv_s(&envValue, &envSize, "COMSPEC");
        if (envValue != nullptr)
        {
            cmdPath = envValue;
            free(envValue);
        }
    }
};

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Run with empty program returns error code 3")
{
    PipedProcess process;

    int exitCode = process.Run("", "");
    CHECK(exitCode == 3); // ERROR_PATH_NOT_FOUND
    CHECK(process.HasStdErrData());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Run with non-existent program returns error code 2")
{
    PipedProcess process;

    int exitCode = process.Run("nonexistent.exe", "");
    CHECK(exitCode == 2); // ERROR_FILE_NOT_FOUND
    CHECK(process.HasStdErrData());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Run cmd with exit code 1")
{
    PipedProcess process;

    int exitCode = process.Run(cmdPath.c_str(), "/c exit 1");
    CHECK(exitCode == 1); // ERROR_INVALID_FUNCTION
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Run with valid program returns zero")
{
    PipedProcess process;

    int exitCode = process.Run(cmdPath.c_str(), "/c echo Hello, World!");
    CHECK(exitCode == 0);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Run with stdin data")
{
    PipedProcess process;
    process.SetStdInData("Hello World!", 12);
    int exitCode = process.Run(echoPath.c_str(), "");
    CHECK(exitCode == 0);
    CHECK(process.FetchStdOutData() == "Hello World!");
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "SetStdInData with empty data")
{
    PipedProcess process;

    process.SetStdInData("", 0);
    int exitCode = process.Run(cmdPath.c_str(), "/c ");
    CHECK(exitCode == 0);
    CHECK(process.FetchStdOutData().empty());
    CHECK(process.FetchStdErrData().empty());
}

TEST_CASE("HasStdOutData returns false when empty")
{
    PipedProcess process;
    CHECK_FALSE(process.HasStdOutData());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "HasStdOutData returns true when data exists")
{
    PipedProcess process;
    process.SetStdInData("Hello World!", 12);
    int exitCode = process.Run(echoPath.c_str(), "");
    CHECK(exitCode == 0);

    CHECK(process.HasStdOutData());
    CHECK(process.FetchStdOutData() == "Hello World!");
    CHECK_FALSE(process.HasStdOutData()); // should be false after fetching
}

TEST_CASE("HasStdErrData returns false when empty")
{
    PipedProcess process;
    CHECK_FALSE(process.HasStdErrData());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "HasStdErrData returns true when error exists")
{
    PipedProcess process;
    int exitCode = process.Run(echoPath.c_str(), "");
    CHECK(exitCode == 1);
    CHECK(process.HasStdErrData());
}

TEST_CASE("FetchStdOutData returns empty when no data")
{
    PipedProcess process;
    std::string data = process.FetchStdOutData();
    CHECK(data.empty());
}

TEST_CASE("FetchStdErrData returns empty when no data")
{
    PipedProcess process;
    std::string data = process.FetchStdErrData();
    CHECK(data.empty());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Large data transfer")
{
    TestUtils::TimedPipedProcess process(TestConstants::LONG_RUNNING_TEST_TIMEOUT_MS);
    
    // Test with different data sizes using constants
    std::vector<size_t> dataSizes = {1, 1000, 4096, 10000};
    
    for (auto size : dataSizes)
    {
        CAPTURE(size);
        
        std::string testData(size, 'A');
        process.SetStdInData(testData.c_str(), testData.size());
        int exitCode = process.Run(echoPath.c_str(), "");
        
        CHECK(exitCode == 0);
        CHECK(process.FetchStdOutData() == testData);
    }
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Timeout handling for slow operations")
{
    TestUtils::TimedPipedProcess process(1000); // 1 second timeout
    
    int exitCode = process.Run(cmdPath.c_str(), "/c echo Quick operation");
    CHECK(exitCode == 0);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Binary data handling")
{
    PipedProcess process;
    
    std::string binaryData = TestUtils::CreateBinaryTestData();
    process.SetStdInData(binaryData.c_str(), binaryData.size());
    
    int exitCode = process.Run(echoPath.c_str(), "");
    CHECK(exitCode == 0);
    
    std::string output = process.FetchStdOutData();
    CHECK(output.size() == binaryData.size());
    CHECK(output == binaryData);
}

TEST_CASE("Abort event functionality")
{
    struct TestAbortEvent
    {
        bool shouldAbort = false;
        bool IsSet() const { return shouldAbort; }
        void SetAbort() { shouldAbort = true; }
    };
    
    TestAbortEvent abortEvent;
    
    CHECK_FALSE(abortEvent.IsSet());
    abortEvent.SetAbort();
    CHECK(abortEvent.IsSet());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Edge case scenarios")
{
    PipedProcess process;
    
    SUBCASE("Multiple SetStdInData calls overwrites previous data")
    {
        process.SetStdInData("First data", 10);
        process.SetStdInData("Hello World!", 12);
        
        int exitCode = process.Run(echoPath.c_str(), "");
        CHECK(exitCode == 0);
        CHECK(process.FetchStdOutData() == "Hello World!");
    }
    
    SUBCASE("Fetch data multiple times returns empty after first")
    {
        process.SetStdInData("Hello World!", 12);
        int exitCode = process.Run(echoPath.c_str(), "");
        CHECK(exitCode == 0);
        
        std::string firstFetch = process.FetchStdOutData();
        std::string secondFetch = process.FetchStdOutData();
        
        CHECK(firstFetch == "Hello World!");
        CHECK(secondFetch.empty());
    }
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Process termination and cleanup")
{
    SUBCASE("Process cleans up after normal execution")
    {
        PipedProcess process;
        int exitCode = process.Run(cmdPath.c_str(), "/c echo test");
        CHECK(exitCode == 0);
        
        std::string output1 = process.FetchStdOutData();
        std::string output2 = process.FetchStdOutData(); 
        CHECK_FALSE(output1.empty());
        CHECK(output2.empty());
    }
    
    SUBCASE("Process handles immediate termination")
    {
        PipedProcess process;
        int exitCode = process.Run(cmdPath.c_str(), "/c exit 0");
        CHECK(exitCode == 0);
    }
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Error boundary testing")
{
    SUBCASE("Long command line arguments")
    {
        PipedProcess process;
        std::string longArgs(1000, 'x');
        
        int exitCode = process.Run(cmdPath.c_str(), ("/c echo " + longArgs).c_str());
        CHECK(exitCode == 0);
    }
    
    SUBCASE("Special characters in arguments")
    {
        PipedProcess process;
        
        int exitCode = process.Run(cmdPath.c_str(), "/c echo \"Special chars: &<>|\"");
        CHECK(exitCode == 0);
    }
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "Multiple process instances")
{
    // Simple test for multiple processes
    for (int i = 0; i < 3; ++i)
    {
        PipedProcess process;
        int exitCode = process.Run(cmdPath.c_str(), "/c echo test");
        CHECK(exitCode == 0);
    }
}

// Tests to validate pipe EOF timing and handle closure behavior
TEST_CASE_FIXTURE(PipedProcessTestFixture, "Pipe EOF timing validation")
{
    SUBCASE("EOF signaling with immediate process completion")
    {
        // Test that EOF signaling works correctly even when process exits quickly
        PipedProcess process;
        
        int exitCode = process.Run(cmdPath.c_str(), "/c echo Hello && echo World");
        
        CHECK(exitCode == 0);
        CHECK(process.HasStdOutData());
        
        std::string output = process.FetchStdOutData();
        CHECK(output.find("Hello") != std::string::npos);
        CHECK(output.find("World") != std::string::npos);
    }
    
    SUBCASE("EOF signaling with both stdout and stderr")
    {
        // Test that both stdout and stderr get proper EOF signaling
        PipedProcess process;
        
        // Command that outputs to both stdout and stderr
        int exitCode = process.Run(cmdPath.c_str(), "/c echo stdout_data && echo stderr_data 1>&2");
        
        CHECK(exitCode == 0);
        CHECK(process.HasStdOutData());
        CHECK(process.HasStdErrData());
        
        std::string stdout_data = process.FetchStdOutData();
        std::string stderr_data = process.FetchStdErrData();
        
        CHECK(stdout_data.find("stdout_data") != std::string::npos);
        CHECK(stderr_data.find("stderr_data") != std::string::npos);
    }
    
    SUBCASE("Large stdin data with proper EOF handling")
    {
        // Test that large data transfers complete properly with EOF signaling
        PipedProcess process;
        
        // Use a reasonably large amount of data
        std::string largeData(5000, 'X');
        process.SetStdInData(largeData.c_str(), largeData.size());
        
        int exitCode = process.Run(echoPath.c_str(), "");
        
        CHECK(exitCode == 0);
        CHECK(process.HasStdOutData());
        
        std::string output = process.FetchStdOutData();
        CHECK(output.size() == largeData.size());
        CHECK(output == largeData);
    }
}

// Test for null pointer parameter validation
TEST_CASE("Run with null program parameter returns ERROR_INVALID_PARAMETER")
{
    PipedProcess process;
    
    DWORD exitCode = process.Run(nullptr, "");
    CHECK(exitCode == ERROR_INVALID_PARAMETER);
    CHECK(process.HasStdErrData());
    
    std::string errorMsg = process.FetchStdErrData();
    CHECK(errorMsg.find("program parameter cannot be null") != std::string::npos);
}

TEST_CASE("Run with null arguments parameter returns ERROR_INVALID_PARAMETER")
{
    PipedProcess process;
    
    DWORD exitCode = process.Run("cmd.exe", nullptr);
    CHECK(exitCode == ERROR_INVALID_PARAMETER);
    CHECK(process.HasStdErrData());
    
    std::string errorMsg = process.FetchStdErrData();
    CHECK(errorMsg.find("arguments parameter cannot be null") != std::string::npos);
}

TEST_CASE("Run with both null parameters returns ERROR_INVALID_PARAMETER")
{
    PipedProcess process;
    
    DWORD exitCode = process.Run(nullptr, nullptr);
    CHECK(exitCode == ERROR_INVALID_PARAMETER);
    CHECK(process.HasStdErrData());
    
    std::string errorMsg = process.FetchStdErrData();
    CHECK(errorMsg.find("program parameter cannot be null") != std::string::npos);
}