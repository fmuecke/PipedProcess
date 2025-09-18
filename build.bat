@echo off
setlocal enabledelayedexpansion

echo ========================================
echo  Building PipedProcess Solution
echo ========================================

REM First check if MSBuild is already available (e.g., in Developer Command Prompt)
where msbuild >nul 2>&1
if %errorlevel% equ 0 (
    echo MSBuild found in current environment, using existing setup...
    goto :build
)

REM Try to find and use Visual Studio Developer Command Prompt
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    echo Finding Visual Studio installation...
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VSINSTALLDIR=%%i"
    )
    
    if defined VSINSTALLDIR (
        echo Found Visual Studio at: !VSINSTALLDIR!
        
        REM Try different approaches to setup Visual Studio environment
        echo Setting up Visual Studio environment...
        
        REM Method 1: Try VsDevCmd.bat with specific parameters
        if exist "!VSINSTALLDIR!\Common7\Tools\VsDevCmd.bat" (
            call "!VSINSTALLDIR!\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo >nul 2>&1
            
            REM Check if MSBuild is now available
            where msbuild >nul 2>&1
            if !errorlevel! equ 0 (
                echo ✅ Visual Studio environment setup successful
                goto :build
            )
        )
        
        REM Method 2: Try vcvarsall.bat directly
        if exist "!VSINSTALLDIR!\VC\Auxiliary\Build\vcvarsall.bat" (
            echo Trying alternate setup method...
            call "!VSINSTALLDIR!\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
            
            REM Check if MSBuild is now available
            where msbuild >nul 2>&1
            if !errorlevel! equ 0 (
                echo ✅ Visual Studio environment setup successful
                goto :build
            )
        )
        
        echo ❌ ERROR: Failed to setup Visual Studio environment
        echo Try running from a Visual Studio Developer Command Prompt instead
        pause
        exit /b 1
    ) else (
        echo ❌ ERROR: Visual Studio installation not found
        echo Please install Visual Studio 2017 or later with C++ build tools
        pause
        exit /b 1
    )
) else (
    echo ❌ ERROR: Visual Studio Installer not found
    echo Please install Visual Studio 2017 or later
    pause
    exit /b 1
)

:build

echo.
echo Cleaning previous builds...
msbuild PipedProcess.sln /p:Configuration=Release /p:Platform=x64 /t:Clean /nologo
if errorlevel 1 (
    echo ❌ ERROR: Clean failed
    pause
    exit /b 1
)

echo.
echo Building solution in Release mode (x64)...
msbuild PipedProcess.sln /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal

if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ Build FAILED!
    echo Check the output above for errors.
    pause
    exit /b 1
)

echo.
echo ✅ Build completed successfully!
echo.

echo Running tests to verify build...
if not exist bin\Tests_doctest.exe (
    echo ❌ Test executable not found!
    pause
    exit /b 1
)

pushd bin
Tests_doctest.exe
if %ERRORLEVEL% neq 0 (
    popd
    echo.
    echo ❌ Tests FAILED!
    pause
    exit /b 1
)
popd

echo.
echo ✅ All tests passed!
echo.
echo Build artifacts available in bin\ directory:
dir bin\*.exe /b

echo.
echo ========================================
echo  BUILD SUCCESSFUL!
echo ========================================
echo.
echo Ready to create NuGet package with:
echo   .\create-package.bat
echo.
pause