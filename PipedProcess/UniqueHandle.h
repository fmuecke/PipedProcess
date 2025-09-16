// This file is part of the PipedProcess project.
// See LICENSE file for further information
// https://github.com/fmuecke/PipedProcess

#pragma once

#include <Windows.h>

// RAII wrapper for Windows handles
class UniqueHandle {
public:
    explicit UniqueHandle(HANDLE h = INVALID_HANDLE_VALUE) noexcept : _handle(h) {}
    
    ~UniqueHandle() noexcept
    {
        if (_handle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(_handle);
        }
    }
    
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    
    UniqueHandle(UniqueHandle&& other) noexcept : _handle(other._handle)
    {
        other._handle = INVALID_HANDLE_VALUE;
    }
    
    UniqueHandle& operator=(UniqueHandle&& other) noexcept
    {
        if (this != &other)
        {
            if (_handle != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(_handle);
            }
            _handle = other._handle;
            other._handle = INVALID_HANDLE_VALUE;
        }
        return *this;
    }
    
    HANDLE get() const noexcept { return _handle; }
    HANDLE release() noexcept
    {
        HANDLE h = _handle;
        _handle = INVALID_HANDLE_VALUE;
        return h;
    }
    
    explicit operator bool() const noexcept
    {
        return _handle != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE _handle;
};