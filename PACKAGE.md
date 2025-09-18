# NuGet Package Creation for PipedProcess

## Quick Start

This project now uses modern NuGet packaging instead of the deprecated CoApp toolchain.

### Prerequisites

1. **Download NuGet CLI** (if not already installed):
   ```powershell
   # Download the latest nuget.exe
   Invoke-WebRequest -Uri "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile "nuget.exe"
   
   # Or install via Chocolatey
   choco install nuget.commandline
   
   # Or install via winget
   winget install Microsoft.NuGet
   ```

2. **Verify installation**:
   ```powershell
   nuget help
   ```

### Package Creation Steps

1. **Build the solution** (ensure tests pass):
   ```powershell
   .\build.bat
   ```

2. **Create the package**:
   ```powershell
   .\create-package.bat
   ```

3. **Package will be created** as `PipedProcess.2.1.3.nupkg`

### Package Contents

The generated package includes:
- **Headers** in `include/PipedProcess/`:
  - `PipedProcess.h` - Main process execution class
  - `StdPipe.h` - Pipe communication utilities  
  - `UniqueHandle.h` - RAII handle wrapper
- **MSBuild targets** for automatic configuration
- **Metadata** and documentation

### Using the Package

After installing in a C++ project:

```cpp
#include <PipedProcess/PipedProcess.h>
#include <PipedProcess/StdPipe.h>

// Headers are automatically included via MSBuild targets
// No manual include path configuration needed
```

### Publishing (Optional)

To publish to NuGet.org:
```powershell
nuget push PipedProcess.2.1.3.nupkg -Source https://api.nuget.org/v3/index.json -ApiKey YOUR_API_KEY
```

The new approach uses standard NuGet toolchain that's actively maintained.