# CapturedProcess
C++ helper class to create a child process that has redirected std in/out/error streams via the Windows API.

It can be used to pass arbitrary binary input data to the child process via stdin and retrieve the result data via stdout. Errors can be received via stderr.
