#pragma once

#include "TestConstants.h"
#include "../PipedProcess/PipedProcess.h"
#include <chrono>
#include <future>
#include <functional>
#include <stdexcept>
#include <memory>
#include <vector>

namespace TestUtils
{
    // Exception thrown when a test operation times out
    class TimeoutException : public std::runtime_error
    {
    public:
        explicit TimeoutException(const std::string& message) 
            : std::runtime_error("Test timeout: " + message) {}
    };

    // Run a function with a timeout
    template<typename Func, typename... Args>
    auto RunWithTimeout(int timeoutMs, Func&& func, Args&&... args) -> decltype(func(args...))
    {
        auto future = std::async(std::launch::async, std::forward<Func>(func), std::forward<Args>(args)...);
        
        if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout)
        {
            throw TimeoutException("Operation timed out after " + std::to_string(timeoutMs) + "ms");
        }
        
        return future.get();
    }

    // Run PipedProcess with timeout protection
    class TimedPipedProcess
    {
    private:
        PipedProcess process;
        int timeoutMs;

    public:
        explicit TimedPipedProcess(int timeout = TestConstants::DEFAULT_TEST_TIMEOUT_MS) 
            : timeoutMs(timeout) {}

        void SetTimeout(int timeout) { timeoutMs = timeout; }
        
        void SetStdInData(const char* data, size_t size)
        {
            process.SetStdInData(data, size);
        }
        
        void SetWindowMode(PipedProcess::WindowMode mode)
        {
            process.SetWindowMode(mode);
        }

        DWORD Run(const char* program, const char* arguments)
        {
            return RunWithTimeout(timeoutMs, [this, program, arguments]() -> DWORD
            {
                return process.Run(program, arguments);
            });
        }

        template<class T>
        DWORD Run(const char* program, const char* arguments, T& abortEvent)
        {
            return RunWithTimeout(timeoutMs, [this, program, arguments, &abortEvent]() -> DWORD
            {
                return process.Run(program, arguments, abortEvent);
            });
        }

        bool HasStdOutData() const { return process.HasStdOutData(); }
        bool HasStdErrData() const { return process.HasStdErrData(); }
        std::string FetchStdOutData() { return process.FetchStdOutData(); }
        std::string FetchStdErrData() { return process.FetchStdErrData(); }
    };

    // Utility to create test data of various sizes
    std::string CreateTestData(size_t size, char fillChar = 'A')
    {
        return std::string(size, fillChar);
    }

    // Utility to create binary test data
    std::string CreateBinaryTestData()
    {
        std::vector<char> binaryData = {
            static_cast<char>(0x00), static_cast<char>(0xFF), 
            static_cast<char>(0x01), static_cast<char>(0x02), 
            static_cast<char>(0xFE), static_cast<char>(0x00), 
            static_cast<char>(0x41), static_cast<char>(0x42)
        };
        return std::string(binaryData.begin(), binaryData.end());
    }
}