#pragma once

// Test constants used across the test suite
namespace TestConstants
{
    // Process exit codes
    static const int SUCCESS_EXIT_CODE = 0;
    static const int EXPECTED_ERROR_EXIT_CODE = 1;
    // Note: Windows system error codes 2 (file not found) and 3 (path not found) used directly
    
    // Test timeout values (in milliseconds)
    static const int SHORT_TEST_TIMEOUT_MS = 1000;  // 1 second
    static const int DEFAULT_TEST_TIMEOUT_MS = 5000;  // 5 seconds
    static const int LONG_RUNNING_TEST_TIMEOUT_MS = 10000;  // 10 seconds
    
    // Test data sizes
    static const size_t BUFFER_SIZE_SMALL = 100;
    static const size_t SMALL_DATA_SIZE = 1;
    static const size_t MEDIUM_DATA_SIZE = 1000;
    static const size_t LARGE_DATA_SIZE = 4096;
    static const size_t XLARGE_DATA_SIZE = 8192;
    static const size_t EXTRA_LARGE_DATA_SIZE = 10000;
    
    // Test file names
    static const char* ECHO_EXECUTABLE = "..\\bin\\stdEcho.exe";
    static const char* DEMO_CHILD_EXECUTABLE = "..\\bin\\DemoChildProc.exe";
    
    // Test messages
    static const char* HELLO_WORLD_MESSAGE = "Hello World!";
    static const char* SYSTEM_PATH_NOT_FOUND_MSG = "The system cannot find the path specified.";
    static const char* SYSTEM_FILE_NOT_FOUND_MSG = "The system cannot find the file specified.";
}