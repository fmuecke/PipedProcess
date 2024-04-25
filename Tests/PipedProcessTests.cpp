#include "CppUnitTest.h"
#include "../PipedProcess/PipedProcess.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PipedProcessTests
{
	TEST_CLASS(PipedProcessTests)
	{
	private:
		static std::string cmdPath;
		static std::string echoPath;

	public:
		PipedProcessTests()
		{
			echoPath = "stdEcho.exe";

			// Use _dupenv_s instead of getenv as getenv is not thread-safe
			char* envValue = nullptr;
			size_t envSize = 0;
			_dupenv_s(&envValue, &envSize, "COMSPEC");
			if (envValue != nullptr)
			{
				cmdPath = envValue;
				free(envValue);
			}
		}

		TEST_METHOD(Run_WithEmptyProgram_ReturnsErrorCode3)
		{
			PipedProcess process;

			int exitCode = process.Run("", ""); // some random GUID FC977CF3-4DB3-4E2C-9238-218AC94ACC3E.exe
			Assert::AreEqual(3, exitCode, L"exit code is not 3"); // 3 is the error code for ERROR_PATH_NOT_FOUND
			Assert::IsTrue(process.HasStdErrData(), L"process has no stderr data");
			Assert::AreNotEqual(process.FetchStdErrData().find("The system cannot find the path specified."), std::string::npos, L"stderr data does not contain the expected message");
		}

		TEST_METHOD(Run_WithNonExistentProgram_ReturnsErrorCode2)
		{
			PipedProcess process;
						
			int exitCode = process.Run("nonexistent.exe", "");
			Assert::AreEqual(2, exitCode, L"exit code is not 2"); // 2 is the error code for ERROR_FILE_NOT_FOUND
			Assert::IsTrue(process.HasStdErrData(), L"process has no stderr data");
			Assert::AreNotEqual(process.FetchStdErrData().find("The system cannot find the file specified."), std::string::npos, L"stderr data does not contain the expected message");
		}
		
		TEST_METHOD(Run_CmdWithExitCode1_ReturnsOne)
		{
			PipedProcess process;

			int exitCode = process.Run(cmdPath.c_str(), "/c exit 1");
			Assert::AreEqual(1, exitCode, L"exit code is not 1");
		}

		TEST_METHOD(Run_WithValidProgram_ReturnsZero)
		{
			PipedProcess process;

			int exitCode = process.Run(cmdPath.c_str(), "/c echo Hello, World!");
			Assert::AreEqual(0, exitCode, L"exit code is not 0");
		}

		TEST_METHOD(Run_WithValidProgramAndStdInData_ReturnsZero)
		{
			PipedProcess process;
			process.SetStdInData("Hello World!", 12);
			int exitCode = process.Run(echoPath.c_str(), ""); // echos data from stdin to stdout and returns 0
			Assert::AreEqual(0, exitCode, L"exit code is not 0");
			Assert::AreEqual("Hello World!", process.FetchStdOutData().c_str(), L"stdout data is not as expected");
		}

		TEST_METHOD(SetStdInData_WithEmptyData_SetsEmptyData)
		{
			PipedProcess process;

			process.SetStdInData("", 0);
			int exitCode = process.Run(cmdPath.c_str(), "/c "); // returns 0 
			Assert::AreEqual(0, exitCode);
			Assert::IsTrue(process.FetchStdOutData().empty(), L"stdout data is not empty");
			Assert::IsTrue(process.FetchStdErrData().empty(), L"stderr data is not empty");
		}

		TEST_METHOD(HasStdOutData_WithEmptyData_ReturnsFalse)
		{
			PipedProcess process;

			bool hasData = process.HasStdOutData();
			Assert::IsFalse(hasData, L"process has stdout data");
		}

		TEST_METHOD(HasStdOutData_WithNonEmptyData_ReturnsTrue)
		{
			PipedProcess process;
			process.SetStdInData("Hello World!", 12);
			int exitCode = process.Run(echoPath.c_str(), ""); // returns 0 
			Assert::AreEqual(0, exitCode, L"exit code is not 0");

			Assert::IsTrue(process.HasStdOutData(), L"process has no stdout data");
			Assert::AreEqual("Hello World!", process.FetchStdOutData().c_str());
			Assert::IsFalse(process.HasStdOutData(), L"process has data even if they were fetched");
		}

		TEST_METHOD(HasStdErrData_WithEmptyData_ReturnsFalse)
		{
			PipedProcess process;
			Assert::IsFalse(process.HasStdErrData(), L"process has stderr data");
		}

		TEST_METHOD(HasStdErrData_WithNonEmptyData_ReturnsTrue)
		{
			PipedProcess process;
			int exitCode = process.Run(echoPath.c_str(), ""); // returns 1 and writes to stderr
			Assert::AreEqual(1, exitCode, L"exit code is not 1");
			Assert::IsTrue(process.HasStdErrData(), L"process has no stderr data");
		}

		TEST_METHOD(FetchStdOutData_WithEmptyData_ReturnsEmptyString)
		{
			PipedProcess process;
			std::string data = process.FetchStdOutData();
			Assert::IsTrue(data.empty(), L"stdout data is not empty");
		}

		TEST_METHOD(FetchStdErrData_WithEmptyData_ReturnsEmptyString)
		{
			PipedProcess process;

			std::string data = process.FetchStdErrData();
			Assert::IsTrue(data.empty(), L"stderr data is not empty");
		}
	};

	std::string PipedProcessTests::cmdPath;
	std::string PipedProcessTests::echoPath;
}
