# Process Redirection in .NET

This guide demonstrates how to use the built-in .NET `System.Diagnostics.Process` class to achieve the same functionality as the C++ PipedProcess library.

## Features

The .NET `Process` class provides:

- Redirection of standard input/output/error streams
- Synchronous and asynchronous operation
- Process window visibility control
- Process termination support
- User impersonation (RunAs)
- Error handling and exit codes
- Cross-platform support

## Basic Usage

### Simple Command Execution

```csharp
using System.Diagnostics;

var process = new Process
{
    StartInfo = new ProcessStartInfo
    {
        FileName = "cmd.exe",
        Arguments = "/c echo Hello World",
        RedirectStandardOutput = true,
        UseShellExecute = false,
        CreateNoWindow = true
    }
};

process.Start();
string output = process.StandardOutput.ReadToEnd();
process.WaitForExit();

Console.WriteLine($"Output: {output}");
Console.WriteLine($"Exit code: {process.ExitCode}");
```

### Sending Input Data

```csharp
var process = new Process
{
    StartInfo = new ProcessStartInfo
    {
        FileName = "sort.exe",
        RedirectStandardInput = true,
        RedirectStandardOutput = true,
        UseShellExecute = false,
        CreateNoWindow = true
    }
};

process.Start();

// Send data to the process
string input = "line2\nline1\nline3";
process.StandardInput.WriteLine(input);
process.StandardInput.Close(); // Important: close the input stream

// Read the sorted output
string sortedOutput = process.StandardOutput.ReadToEnd();
process.WaitForExit();

Console.WriteLine($"Sorted output: {sortedOutput}");
```

### Modern Async/Await Pattern

```csharp
async Task RunProcessAsync(string filename, string arguments, string input = null)
{
    using var process = new Process
    {
        StartInfo = new ProcessStartInfo
        {
            FileName = filename,
            Arguments = arguments,
            RedirectStandardInput = input != null,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        }
    };

    process.Start();

    // Create tasks for reading output and error streams
    var readOutput = process.StandardOutput.ReadToEndAsync();
    var readError = process.StandardError.ReadToEndAsync();

    // Write input if provided
    if (input != null)
    {
        await process.StandardInput.WriteAsync(input);
        process.StandardInput.Close();
    }

    // Wait for process to complete and collect all output
    await process.WaitForExitAsync();
    string output = await readOutput;
    string error = await readError;

    Console.WriteLine($"Output: {output}");
    if (!string.IsNullOrEmpty(error))
        Console.WriteLine($"Error: {error}");
    Console.WriteLine($"Exit code: {process.ExitCode}");
}
```

### Error Handling

```csharp
try
{
    var process = new Process
    {
        StartInfo = new ProcessStartInfo
        {
            FileName = "nonexistent.exe",
            RedirectStandardError = true,
            UseShellExecute = false
        }
    };

    process.Start();
}
catch (System.ComponentModel.Win32Exception ex)
{
    Console.WriteLine($"Failed to start process: {ex.Message}");
}
```

### Cancellation Support

```csharp
async Task RunWithCancellationAsync(CancellationToken cancellationToken)
{
    using var process = new Process
    {
        StartInfo = new ProcessStartInfo
        {
            FileName = "long-running-process.exe",
            RedirectStandardOutput = true,
            UseShellExecute = false
        }
    };

    // Register cancellation
    cancellationToken.Register(() =>
    {
        try
        {
            if (!process.HasExited)
                process.Kill();
        }
        catch (InvalidOperationException) { } // Process already exited
    });

    process.Start();
    await process.WaitForExitAsync(cancellationToken);
}
```

### Window Visibility Control

```csharp
var process = new Process
{
    StartInfo = new ProcessStartInfo
    {
        FileName = "notepad.exe",
        Arguments = "test.txt",
        UseShellExecute = true,  // Required for window manipulation
        WindowStyle = ProcessWindowStyle.Hidden // or Normal, Minimized, Maximized
    }
};

process.Start();
```

### Run As Different User

```csharp
var process = new Process
{
    StartInfo = new ProcessStartInfo
    {
        FileName = "cmd.exe",
        UseShellExecute = false,
        RedirectStandardOutput = true,
        Domain = "DOMAIN",
        UserName = "username",
        Password = secureString, // System.Security.SecureString
    }
};

process.Start();
```

## Security Considerations

- Validate all input parameters, especially when handling user-provided values
- Be careful with shell execution (`UseShellExecute = true`) as it can be vulnerable to command injection
- Use proper error handling and timeout mechanisms
- Consider using allowlists for executable paths
- Handle process output streams to prevent memory issues with large outputs
- Use secure strings for sensitive data like passwords
- Implement proper cleanup of process resources using `using` statements or explicit Dispose calls

## Best Practices

1. Always dispose of Process objects:

   ```csharp
   using var process = new Process();
   ```

2. Close redirected streams when done:

   ```csharp
   process.StandardInput.Close();
   ```

3. Use async methods for better scalability:

   ```csharp
   await process.WaitForExitAsync();
   ```

4. Set appropriate timeouts:

   ```csharp
   if (!process.WaitForExit(timeoutMs))
       process.Kill();
   ```

5. Handle both output and error streams:

   ```csharp
   var outputTask = process.StandardOutput.ReadToEndAsync();
   var errorTask = process.StandardError.ReadToEndAsync();
   await Task.WhenAll(outputTask, errorTask);
   ```

## Differences from PipedProcess (C++)

The .NET implementation offers several advantages:

1. Native async/await support for non-blocking operations
2. Stream-based I/O for more flexible data handling
3. Built-in cross-platform support
4. More comprehensive process management capabilities
5. Integration with .NET security features
6. Exception-based error handling (can be wrapped if exception-free API is preferred)
