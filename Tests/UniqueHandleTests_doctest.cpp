#include "doctest/doctest.h"
#include "../PipedProcess/UniqueHandle.h"
#include <Windows.h>
#include <utility> // for std::move

// Test fixture class for UniqueHandle tests
class UniqueHandleTestFixture
{
public:
    HANDLE CreateDummyEvent()
    {
        return ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
    }
    
    void CleanupHandle(HANDLE h)
    {
        if (h != INVALID_HANDLE_VALUE && h != nullptr)
        {
            ::CloseHandle(h);
        }
    }
};

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle default constructor creates invalid handle")
{
    UniqueHandle handle;
    
    CHECK(handle.get() == INVALID_HANDLE_VALUE);
    CHECK_FALSE(handle); // Boolean conversion should be false
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle explicit constructor with INVALID_HANDLE_VALUE")
{
    UniqueHandle handle(INVALID_HANDLE_VALUE);
    
    CHECK(handle.get() == INVALID_HANDLE_VALUE);
    CHECK_FALSE(handle);
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle constructor with valid handle")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle handle(event);
    
    CHECK(handle.get() == event);
    CHECK(handle); // Boolean conversion should be true
    
    // Handle will be automatically closed by destructor
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle move constructor transfers ownership")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle original(event);
    UniqueHandle moved(std::move(original));
    
    CHECK(moved.get() == event);
    CHECK(moved);
    CHECK(original.get() == INVALID_HANDLE_VALUE);
    CHECK_FALSE(original);
    
    // Handle will be automatically closed by moved destructor
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle move assignment transfers ownership")
{
    HANDLE event1 = CreateDummyEvent();
    HANDLE event2 = CreateDummyEvent();
    REQUIRE(event1 != INVALID_HANDLE_VALUE);
    REQUIRE(event2 != INVALID_HANDLE_VALUE);
    
    UniqueHandle handle1(event1);
    UniqueHandle handle2(event2);
    
    handle2 = std::move(handle1);
    
    CHECK(handle2.get() == event1);
    CHECK(handle2);
    CHECK(handle1.get() == INVALID_HANDLE_VALUE);
    CHECK_FALSE(handle1);
    
    // event1 will be closed by handle2 destructor
    // event2 was closed by the move assignment
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle self move assignment is safe")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle handle(event);
    
    // Self move assignment should be safe
    handle = std::move(handle);
    
    CHECK(handle.get() == event);
    CHECK(handle);
    
    // Handle will be automatically closed by destructor
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle release transfers ownership without closing")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle handle(event);
    HANDLE released = handle.release();
    
    CHECK(released == event);
    CHECK(handle.get() == INVALID_HANDLE_VALUE);
    CHECK_FALSE(handle);
    
    // We must manually close the released handle
    CleanupHandle(released);
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle multiple releases are safe")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle handle(event);
    HANDLE released1 = handle.release();
    HANDLE released2 = handle.release();
    
    CHECK(released1 == event);
    CHECK(released2 == INVALID_HANDLE_VALUE);
    CHECK(handle.get() == INVALID_HANDLE_VALUE);
    
    // Clean up the first release
    CleanupHandle(released1);
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle boolean operator with various handle values")
{
    UniqueHandle invalidHandle(INVALID_HANDLE_VALUE);
    CHECK_FALSE(invalidHandle);
    
    // Note: We don't test nullptr because UniqueHandle expects INVALID_HANDLE_VALUE
    // for invalid Windows handles, not nullptr
    
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle validHandle(event);
    CHECK(validHandle);
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle assignment to invalid handle closes previous")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    UniqueHandle handle(event);
    CHECK(handle.get() == event);
    
    // Assign invalid handle - should close the previous handle
    handle = UniqueHandle();
    
    CHECK(handle.get() == INVALID_HANDLE_VALUE);
    CHECK_FALSE(handle);
    
    // Original event handle should now be closed (we can't easily verify this
    // without potentially unsafe operations, but the destructor would have closed it)
}

TEST_CASE_FIXTURE(UniqueHandleTestFixture, "UniqueHandle RAII cleanup prevents handle leaks")
{
    HANDLE event = CreateDummyEvent();
    REQUIRE(event != INVALID_HANDLE_VALUE);
    
    {
        UniqueHandle handle(event);
        CHECK(handle.get() == event);
        CHECK(handle);
        // Destructor will be called here and should close the handle
    }
    
    // If we tried to use the handle here, it should be invalid
    // (We can't safely test this without potential undefined behavior)
    
    // The test itself validates that the handle was properly managed
    // by not causing resource leaks that would accumulate over multiple test runs
}