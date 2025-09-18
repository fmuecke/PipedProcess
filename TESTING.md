# PipedProcess Test Suite

## Running Tests

### Prerequisites
- Build the solution in Debug configuration for x64 platform
- All executables are automatically placed in the `bin` directory

### Running Tests

**Important**: Tests are designed to run from the `bin` directory where all executables are located.

#### Option 1: Use the Test Runner Script (Recommended)
```batch
.\run_tests.bat
```

#### Option 2: Manual Execution
```batch
# Change to bin directory where all executables are located
cd bin
.\Tests_doctest.exe
```

#### Option 3: Run Specific Test Categories
```batch
# From bin directory
cd bin

# Run only StdPipe write tests (includes partial write validation)
.\Tests_doctest.exe --test-case="*Write*"

# Run only process execution tests
.\Tests_doctest.exe --test-case="*Run*"

# Run with verbose output
.\Tests_doctest.exe --verbose
```

### Build and Test Workflow
```batch
# Build the solution
msbuild PipedProcess.sln /p:Configuration=Debug /p:Platform=x64

# Run all tests
.\run_tests.bat
```

### Test Structure

The test suite includes:
- **PipedProcess functionality tests**: Process execution, stdin/stdout handling, error conditions
- **StdPipe functionality tests**: Pipe management, read/write operations, large data handling
- **Partial write validation**: Tests for handling data larger than DWORD limits (>4GB scenarios)
- **Input validation tests**: Null pointer safety and exception handling

### Recent Improvements

1. **Partial Write Support**: The `StdPipe::Write()` method now properly handles large data by chunking it into DWORD-sized pieces
2. **Input Validation**: Added null pointer checks with proper exception throwing
3. **Build Configuration**: All executables are centralized in the `bin` directory
4. **Test Coverage**: Added comprehensive tests for edge cases and large data scenarios

### Troubleshooting

If tests fail with error code 3 ("path not found"):
- Ensure you're running from the solution root directory
- Verify that `stdEcho.exe` and `DemoChildProc.exe` exist in the `bin` directory
- Check that the current working directory is correct

If build fails:
- Ensure Visual Studio 2022 with C++ workload is installed
- Verify the platform is set to x64
- Check that all dependencies are available