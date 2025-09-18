cmake_minimum_required(VERSION 3.10)
project(PipedProcessTests_doctest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the doctest include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Create the test executable
add_executable(PipedProcessTests_doctest
    doctest_main.cpp
    PipedProcessTests_doctest.cpp
    StdPipeTests_doctest.cpp
)

# Set the output directory
set_target_properties(PipedProcessTests_doctest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../Debug
)

# Add Windows-specific definitions
if(WIN32)
    target_compile_definitions(PipedProcessTests_doctest PRIVATE 
        WIN32 
        _DEBUG 
        _CONSOLE 
        _CRT_SECURE_NO_WARNINGS
        _UNICODE 
        UNICODE
    )
    
    # Link Windows libraries
    target_link_libraries(PipedProcessTests_doctest 
        kernel32 
        user32 
        gdi32 
        winspool 
        comdlg32 
        advapi32 
        shell32 
        ole32 
        oleaut32 
        uuid 
        odbc32 
        odbccp32
    )
endif()

# Enable testing
enable_testing()
add_test(NAME PipedProcessTests_doctest COMMAND PipedProcessTests_doctest)