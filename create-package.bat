@echo off
echo =====================================
echo Creating PipedProcess NuGet Package
echo =====================================
echo.

REM Check if nuget.exe exists
where nuget >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ NuGet CLI not found in PATH!
    echo.
    echo Please install NuGet CLI:
    echo 1. Download from: https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
    echo 2. Place in PATH or current directory
    echo 3. Or install via: winget install Microsoft.NuGet
    echo.
    pause
    exit /b 1
)

REM Verify nuspec file exists
if not exist PipedProcess.nuspec (
    echo ❌ PipedProcess.nuspec not found!
    echo Make sure you're running this from the project root directory.
    pause
    exit /b 1
)

REM Verify required files exist
echo Checking required files...
if not exist PipedProcess\PipedProcess.h (
    echo ❌ PipedProcess\PipedProcess.h not found!
    pause
    exit /b 1
)
if not exist PipedProcess\StdPipe.h (
    echo ❌ PipedProcess\StdPipe.h not found!
    pause
    exit /b 1
)
if not exist PipedProcess\UniqueHandle.h (
    echo ❌ PipedProcess\UniqueHandle.h not found!
    pause
    exit /b 1
)
if not exist PipedProcess.targets (
    echo ❌ PipedProcess.targets not found!
    pause
    exit /b 1
)

echo ✅ All required files found.
echo.

REM Clean previous packages
echo Cleaning previous packages...
del PipedProcess.*.nupkg 2>nul

echo Creating NuGet package...
nuget pack PipedProcess.nuspec

if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ Package creation FAILED!
    echo Check the output above for errors.
    pause
    exit /b 1
)

echo.
echo ✅ Package created successfully!
echo.

REM Show created package
for %%f in (PipedProcess.*.nupkg) do (
    echo 📦 Package: %%f
    echo    Size: 
    dir "%%f" | findstr "%%f"
)

echo.
echo Package contents: Headers, MSBuild targets, LICENSE, README.md, and icon
echo Use 7zip or rename to .zip to inspect detailed contents

echo.
echo =====================================
echo Package creation completed!
echo =====================================
echo.
echo To publish to NuGet.org:
echo   nuget push PipedProcess.*.nupkg -Source https://api.nuget.org/v3/index.json -ApiKey YOUR_API_KEY
echo.
pause