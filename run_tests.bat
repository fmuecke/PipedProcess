@echo off
REM Test runner script for PipedProcess
REM This script ensures tests are run from the correct directory (bin)

echo Running PipedProcess Tests...
echo.

REM Change to solution root directory first
cd /d "%~dp0"

REM Verify we're in the right location
if not exist "bin\Tests_doctest.exe" (
    echo ERROR: Tests_doctest.exe not found in bin directory
    echo Please build the solution first
    pause
    exit /b 1
)

REM Change to bin directory where all executables are located
cd bin

REM Run the tests
echo Running all tests from bin directory...
.\Tests_doctest.exe

echo.
echo Tests completed with exit code: %ERRORLEVEL%
pause