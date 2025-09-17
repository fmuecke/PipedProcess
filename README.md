# PipedProcess

C++ helper class to create a child process with redirected std in/out/error streams 
using the Windows API.

## What it does

It can be used to pass arbitrary binary input data to the child process via stdin and
retrieve the result data via stdout. Errors can be received via stderr.

## What it does not

Currently the class can *not* be used for asynchronous communication (e.g. messages) to and from the child process.

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

## Create nuget package 

1. Get CoApp script from https://coapp.github.io/pages/releases.html
2. Install CoApp
3. Run the following command to create the NuGet package

    Write-NuGetPackage .\PipedProcess.autopkg


