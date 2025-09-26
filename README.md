# PipedProcess

C++ helper class to create a child process with redirected std in/out/error streams using the Windows API.
[Want to use it with .net?](README.NET.md)

## What it does

It can be used to pass arbitrary binary input data to the child process via stdin and
retrieve the result data via stdout. Errors can be received via stderr.

## What it does not

Currently the class can *not* be used for asynchronous communication (e.g. messages) to and from the child process.

## Public Interface

The `PipedProcess` class provides a simple, exception-free public API that returns error codes for all operations.

### Core Methods

```cpp
// Basic execution
DWORD Run(const char* program, const char* arguments);

// Execution with abort capability
template<class T>
DWORD Run(const char* program, const char* arguments, T& abortEvent);

// Execution with user token (run as different user)
DWORD RunAs(const HANDLE& token, const char* program, const char* arguments);
template<class T>
DWORD RunAs(const HANDLE& token, const char* program, const char* arguments, T& abortEvent);
```

### Input/Output Methods

```cpp
// Set data to send to child process stdin
void SetStdInData(const char* pData, size_t len);

// Check if output data is available
bool HasStdOutData() const;
bool HasStdErrData() const;

// Retrieve output data (moves data out of internal buffers)
std::string FetchStdOutData();
std::string FetchStdErrData();
```

### Configuration

```cpp
// Control child process window visibility
enum class WindowMode { Visible = 0, Hidden = 1 };
void SetWindowMode(WindowMode mode);  // Default is Hidden
```

### Return Values

All `Run` methods return `DWORD` error codes:

- `0` = Success (child process completed successfully)
- `ERROR_INVALID_PARAMETER` = Null parameters provided
- `ERROR_FILE_NOT_FOUND` = Program executable not found
- `ERROR_PATH_NOT_FOUND` = Empty program path
- Other Windows error codes for various failure conditions

Error details are available via `FetchStdErrData()` when errors occur.

## Usage Examples

### Basic Usage

```cpp
#include "PipedProcess/PipedProcess.h"

// Simple command execution
PipedProcess process;
DWORD exitCode = process.Run("cmd.exe", "/c echo Hello World");

if (exitCode == 0 && process.HasStdOutData()) {
    std::string output = process.FetchStdOutData();
    std::cout << "Output: " << output << std::endl;
}
```

### Sending Input Data

```cpp
// Execute a program that reads from stdin
PipedProcess process;
std::string input = "Hello from parent process\n";
process.SetStdInData(input.c_str(), input.length());

DWORD exitCode = process.Run("sort.exe", "");
if (exitCode == 0) {
    std::string output = process.FetchStdOutData();
    std::cout << "Sorted output: " << output << std::endl;
}
```

### Error Handling

```cpp
PipedProcess process;
DWORD exitCode = process.Run("nonexistent.exe", "");

if (exitCode != 0) {
    std::cout << "Process failed with exit code: " << exitCode << std::endl;
    
    if (process.HasStdErrData()) {
        std::string errorMsg = process.FetchStdErrData();
        std::cout << "Error details: " << errorMsg << std::endl;
    }
}
```

### Using Abort Events

```cpp
// Custom abort event
struct MyAbortEvent {
    std::atomic<bool> shouldAbort{false};
    bool IsSet() const { return shouldAbort.load(); }
    void Set() { shouldAbort = true; }
};

PipedProcess process;
MyAbortEvent abortEvent;

// Start long-running process in another thread
auto future = std::async(std::launch::async, [&]() {
    return process.Run("long-running-program.exe", "", abortEvent);
});

// Later, signal abort if needed
abortEvent.Set();
DWORD exitCode = future.get();
```

### Hidden vs Visible Windows

```cpp
PipedProcess process;

// Show the child process window (useful for debugging)
process.SetWindowMode(PipedProcess::WindowMode::Visible);

// Or keep it hidden (default behavior)
process.SetWindowMode(PipedProcess::WindowMode::Hidden);

DWORD exitCode = process.Run("notepad.exe", "test.txt");
```

## Security Considerations

**Important:** This library does not validate input parameters. It is the **caller's responsibility** to ensure:

- **Program paths** are trusted and do not contain malicious executable paths
- **Arguments** do not contain command injection vectors (e.g., `|`, `&`, `;`, `>`, `<` when used with shell commands)
- **Input data** size is reasonable to prevent memory exhaustion
- **Program execution** is limited to trusted executables in secure environments

For applications handling untrusted input, implement validation before calling PipedProcess methods.

## MIT License

Feel free to use. See `LICENSE` file for further information.

## How to build

The project is a Visual Studio 2022 solution. It should be possible to build it with other compilers, but I have not tested it.

## Creating NuGet Package

### Prerequisites
- [NuGet CLI](https://docs.microsoft.com/en-us/nuget/reference/nuget-exe-cli-reference) or [dotnet CLI](https://docs.microsoft.com/en-us/dotnet/core/tools/)
- Visual Studio 2022 (for building)

### Package Creation Steps

1. **Build the solution** to ensure all tests pass:
   ```
   msbuild PipedProcess.sln -p:Configuration=Release -p:Platform=x64
   ```

2. **Create the NuGet package** using the modern nuspec file:
   ```
   nuget pack PipedProcess.nuspec
   ```
   
   Or with dotnet CLI:
   ```
   dotnet pack PipedProcess.nuspec
   ```

3. **Publish to NuGet.org** (optional):
   ```
   nuget push PipedProcess.{version}.nupkg -Source https://api.nuget.org/v3/index.json -ApiKey {your-api-key}
   ```

### Package Contents
The NuGet package includes:
- **Headers**: `PipedProcess.h`, `StdPipe.h`, `UniqueHandle.h` in `include/PipedProcess/` directory
- **MSBuild targets**: Automatic include path configuration and preprocessor definitions
- **Documentation**: Inline comments and usage examples

### Using the Package
After installing the package in a project, simply include the headers:
```cpp
#include <PipedProcess/PipedProcess.h>
#include <PipedProcess/StdPipe.h>
```

The MSBuild targets automatically configure include paths and preprocessor definitions.


