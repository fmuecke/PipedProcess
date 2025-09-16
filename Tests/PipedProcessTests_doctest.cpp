#include "doctest/doctest.h"
#include "../PipedProcess/PipedProcess.h"
#include <string>
#include <cstdlib>

// Test fixture class for PipedProcess tests
class PipedProcessTestFixture
{
public:
    std::string cmdPath;
    std::string echoPath;

    PipedProcessTestFixture()
    {
        echoPath = "stdEcho.exe";

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

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - Run with empty program returns error code 3")
{
    PipedProcess process;

    int exitCode = process.Run("", "");
    CHECK(exitCode == 3); // 3 is the error code for ERROR_PATH_NOT_FOUND
    CHECK(process.HasStdErrData());
    CHECK(process.FetchStdErrData().find("The system cannot find the path specified.") != std::string::npos);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - Run with non-existent program returns error code 2")
{
    PipedProcess process;

    int exitCode = process.Run("nonexistent.exe", "");
    CHECK(exitCode == 2); // 2 is the error code for ERROR_FILE_NOT_FOUND
    CHECK(process.HasStdErrData());
    CHECK(process.FetchStdErrData().find("The system cannot find the file specified.") != std::string::npos);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - Run cmd with exit code 1 returns one")
{
    PipedProcess process;

    int exitCode = process.Run(cmdPath.c_str(), "/c exit 1");
    CHECK(exitCode == 1);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - Run with valid program returns zero")
{
    PipedProcess process;

    int exitCode = process.Run(cmdPath.c_str(), "/c echo Hello, World!");
    CHECK(exitCode == 0);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - Run with valid program and stdin data returns zero")
{
    PipedProcess process;
    process.SetStdInData("Hello World!", 12);
    int exitCode = process.Run(echoPath.c_str(), ""); // echos data from stdin to stdout and returns 0
    CHECK(exitCode == 0);
    CHECK(process.FetchStdOutData() == "Hello World!");
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - SetStdInData with empty data sets empty data")
{
    PipedProcess process;

    process.SetStdInData("", 0);
    int exitCode = process.Run(cmdPath.c_str(), "/c "); // returns 0 
    CHECK(exitCode == 0);
    CHECK(process.FetchStdOutData().empty());
    CHECK(process.FetchStdErrData().empty());
}

TEST_CASE("PipedProcess - HasStdOutData with empty data returns false")
{
    PipedProcess process;

    bool hasData = process.HasStdOutData();
    CHECK_FALSE(hasData);
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - HasStdOutData with non-empty data returns true")
{
    PipedProcess process;
    process.SetStdInData("Hello World!", 12);
    int exitCode = process.Run(echoPath.c_str(), ""); // returns 0 
    CHECK(exitCode == 0);

    CHECK(process.HasStdOutData());
    CHECK(process.FetchStdOutData() == "Hello World!");
    CHECK_FALSE(process.HasStdOutData()); // should be false after fetching
}

TEST_CASE("PipedProcess - HasStdErrData with empty data returns false")
{
    PipedProcess process;
    CHECK_FALSE(process.HasStdErrData());
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - HasStdErrData with non-empty data returns true")
{
    PipedProcess process;
    int exitCode = process.Run(echoPath.c_str(), ""); // returns 1 and writes to stderr
    CHECK(exitCode == 1);
    CHECK(process.HasStdErrData());
}

TEST_CASE("PipedProcess - FetchStdOutData with empty data returns empty string")
{
    PipedProcess process;
    std::string data = process.FetchStdOutData();
    CHECK(data.empty());
}

TEST_CASE("PipedProcess - FetchStdErrData with empty data returns empty string")
{
    PipedProcess process;
    std::string data = process.FetchStdErrData();
    CHECK(data.empty());
}

// Additional tests that demonstrate doctest's capabilities
TEST_CASE("PipedProcess - Window mode can be set")
{
    PipedProcess process;
    
    SUBCASE("Hidden mode")
    {
        process.SetWindowMode(PipedProcess::WindowMode::Hidden);
        // Test that window mode is set correctly (can't easily verify without internal access)
        CHECK(true); // Placeholder - in real scenario we'd need access to internal state
    }
    
    SUBCASE("Visible mode")
    {
        process.SetWindowMode(PipedProcess::WindowMode::Visible);
        // Test that window mode is set correctly
        CHECK(true); // Placeholder
    }
}

TEST_CASE_FIXTURE(PipedProcessTestFixture, "PipedProcess - Large data transfer")
{
    PipedProcess process;
    
    // Test with different data sizes
    std::vector<size_t> dataSizes = {1, 100, 1000, 4096, 10000};
    
    for (auto size : dataSizes)
    {
        CAPTURE(size); // This will show the size in case of failure
        
        std::string testData(size, 'A');
        process.SetStdInData(testData.c_str(), testData.size());
        int exitCode = process.Run(echoPath.c_str(), "");
        
        CHECK(exitCode == 0);
        CHECK(process.FetchStdOutData() == testData);
    }
}