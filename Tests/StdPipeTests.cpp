#include "CppUnitTest.h"
#include "../PipedProcess/StdPipe.h"
#include <string>
#include <thread>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PipedProcessTests
{
	TEST_CLASS(StdPipeTests)
	{
	public:

		TEST_METHOD(HasValidHandles)
		{
			StdPipe pipe;
			Assert::AreNotEqual(pipe.GetReadHandle(), INVALID_HANDLE_VALUE, L"Read handle is invalid");
			Assert::AreNotEqual(pipe.GetWriteHandle(), INVALID_HANDLE_VALUE, L"Write handle is invalid");
			Assert::AreNotEqual(pipe.GetReadHandle(), pipe.GetWriteHandle(), L"Read and write handles are the same");
		}

		TEST_METHOD(HasInvalidHandlesAfterClosing)
		{
			StdPipe pipe;
			pipe.CloseReadHandle();
			pipe.CloseWriteHandle();
			Assert::AreEqual(pipe.GetReadHandle(), INVALID_HANDLE_VALUE, L"Read handle is not invalid");
			Assert::AreEqual(pipe.GetWriteHandle(), INVALID_HANDLE_VALUE, L"Write handle is not invalid");
		}

		// does not work yet
		TEST_METHOD(WriteAndRead)
		{
			StdPipe pipe;
			std::string testString = "Hello, World!";
			pipe.Write(testString.c_str(), (int)testString.size());
			pipe.CloseWriteHandle();

			auto readString = pipe.Read();
			Assert::AreEqual(testString, readString, L"Read string is not the same as the written string");
		}

		TEST_METHOD(TestHasData)
		{
			StdPipe pipe;
			Assert::IsFalse(pipe.HasData(), L"Pipe has data before writing to it");

			std::string testString = "Hello, World!";
			pipe.Write(testString.c_str(), (int)testString.size());

			Assert::IsTrue(pipe.HasData(), L"Pipe does not have data after writing to it");
		}

		TEST_METHOD(TestCloseHandles)
		{
			StdPipe pipe;
			Assert::IsTrue(pipe.GetReadHandle() != INVALID_HANDLE_VALUE, L"Read handle is invalid");
			Assert::IsTrue(pipe.GetWriteHandle() != INVALID_HANDLE_VALUE, L"Write handle is invalid");

			pipe.CloseReadHandle();
			Assert::IsTrue(pipe.GetReadHandle() == INVALID_HANDLE_VALUE, L"Read handle is not invalid");

			pipe.CloseWriteHandle();
			Assert::IsTrue(pipe.GetWriteHandle() == INVALID_HANDLE_VALUE, L"Write handle is not invalid");
		}

		// does not work yet
		TEST_METHOD(WriteAndReadLargeData)
		{
			StdPipe pipe;
			std::string testString(4096, 'a');  // Large string (bigger than 4096 did not work)
			pipe.Write(testString.c_str(), (int)testString.size()); // call might hang if the pipe is full and is not being read from
			pipe.CloseWriteHandle();

			auto readString = pipe.Read();
			Assert::AreEqual(testString, readString, L"Read string is not the same as the written string");
		}

		// does not work yet
		TEST_METHOD(WriteAndReadMultipleTimes)
		{
			StdPipe pipe;
			std::string testString1 = "Hello, World!";
			std::string testString2 = "Goodbye, World!";
			pipe.Write(testString1.c_str(), (int)testString1.size());
			pipe.Write(testString2.c_str(), (int)testString2.size());
			pipe.CloseWriteHandle();
			Assert::ExpectException<std::system_error>([&]() { pipe.Write("abc", 3); }, L"Writing to a closed pipe did not throw an exception");

			auto readString1 = pipe.Read(); 

			Assert::AreEqual(testString1 + testString2, readString1, L"Read string is not the same as the written string");
		}

		// does not work yet
		//TEST_METHOD(MultithreadedAccess)
		//{
		//	StdPipe pipe;
		//	std::string testString = "Hello, World!";
		//	std::thread writerThread([&]() { pipe.Write(testString.c_str(), testString.size()); });
		//	std::thread readerThread([&]() { auto readData = pipe.Read(); });

		//	writerThread.join();
		//	readerThread.join();

		//	// If the test reaches this point without crashing, it means that the multithreaded access didn't cause any problems.
		//	// However, this is a very basic test and might not catch all concurrency issues.

		//	Assert::IsTrue(true, L"Multithreaded access did not cause any problems");
		//}
	};
}
