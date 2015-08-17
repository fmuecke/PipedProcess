# CapturedProcess
C++ helper class to create a child process with redirected std in/out/error streams 
using the Windows API.

## What it does
It can be used to pass arbitrary binary input data to the child process via stdin and
retrieve the result data via stdout. Errors can be received via stderr.

## What it not does
Currently the class can not be used for asynchronous communication (e.g. messages) to and from the child process.

## BSD License
Feel free to use. See `LICENSE` file for further information.
