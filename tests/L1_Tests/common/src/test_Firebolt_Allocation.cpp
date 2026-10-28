#include <new>
#include "test_Firebolt_Extension.h"

#if defined(TEST_FIREBOLT_SHELL)
#include "firebolt_shell.h"
using ExtensionContext = FireboltShell;
using ExtensionClientInfo = FireboltShellClientInfo;
static const wl_interface* const extensionInterface = &firebolt_shell_interface;
#elif defined(TEST_FIREBOLT_WM)
using ExtensionContext = FireboltWindowManager;
using ExtensionClientInfo = FireboltWmClientInfo;
static const wl_interface* const extensionInterface = &firebolt_wm_interface;
#elif defined(TEST_FIREBOLT_SURFACE)
#include "firebolt_surface.h"
using ExtensionContext = FireboltSurface;
using ExtensionClientInfo = FireboltSurfaceClientInfo;
static const wl_interface* const extensionInterface = &firebolt_surface_interface;
#else
#error Select a Firebolt extension for the allocation tests
#endif

namespace
{
thread_local std::size_t failAllocationSize = 0;
thread_local bool allocationFailed = false;

class FailNothrowAllocation
{
public:
    explicit FailNothrowAllocation(std::size_t size)
    {
        allocationFailed = false;
        failAllocationSize = size;
    }

    ~FailNothrowAllocation()
    {
        failAllocationSize = 0;
    }

    FailNothrowAllocation(const FailNothrowAllocation&) = delete;
    FailNothrowAllocation& operator=(const FailNothrowAllocation&) = delete;
};
}

// Test-only replacement: leave throwing allocations (including GMock's) alone.
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    if (failAllocationSize != 0 && size == failAllocationSize)
    {
        failAllocationSize = 0;
        allocationFailed = true;
        return nullptr;
    }

    try
    {
        return ::operator new(size);
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }
}

void operator delete(void* pointer, const std::nothrow_t&) noexcept
{
    ::operator delete(pointer);
}

class FireboltAllocationTest : public FireboltExtensionTest
{
protected:
    int compositorToken = 0;
    int displayToken = 0;
    int globalToken = 0;
    int clientToken = 0;
    wl_resource resource{};
    ExtensionContext* context = nullptr;

    WstCompositor* compositor()
    {
        return reinterpret_cast<WstCompositor*>(&compositorToken);
    }

    wl_display* display()
    {
        return reinterpret_cast<wl_display*>(&displayToken);
    }

    wl_client* client()
    {
        return reinterpret_cast<wl_client*>(&clientToken);
    }

    void initialize()
    {
        EXPECT_CALL(*p_waylandImplMock,
                    wl_global_create(display(), extensionInterface, 1, testing::_, testing::_))
            .WillOnce(testing::Invoke(
                [this](wl_display*, const wl_interface*, int, void* data, wl_global_bind_func_t bind)
                {
                    context = static_cast<ExtensionContext*>(data);
                    capturedBindFunc = bind;
                    return reinterpret_cast<wl_global*>(&globalToken);
                }));
        ASSERT_TRUE(moduleInit(compositor(), display()));
        ASSERT_NE(nullptr, context);
        ASSERT_NE(nullptr, capturedBindFunc);
    }

    void TearDown() override
    {
        moduleTerm(compositor());
    }
};

TEST_F(FireboltAllocationTest, ContextAllocationFailurePreservesReturnValueAndAllowsRetry)
{
    EXPECT_CALL(*p_waylandImplMock,
                wl_global_create(testing::_, testing::_, testing::_, testing::_, testing::_))
        .Times(0);
    bool initialized = true;
    {
        FailNothrowAllocation failure(sizeof(ExtensionContext));
        EXPECT_NO_THROW(initialized = moduleInit(compositor(), display()));
    }
    EXPECT_TRUE(allocationFailed);
    // Preserve the existing moduleInit return value for a CodeQL-only fix.
    EXPECT_TRUE(initialized);
    EXPECT_EQ(nullptr, context);
    EXPECT_EQ(nullptr, capturedBindFunc);
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(p_waylandImplMock));

    initialize();
    ASSERT_NE(nullptr, context);
    EXPECT_TRUE(context->mClientListMap.empty());
}

TEST_F(FireboltAllocationTest, ClientAllocationFailurePostsNoMemoryWithoutRegisteringClient)
{
    initialize();
    ASSERT_NE(nullptr, capturedBindFunc);
    ASSERT_NE(nullptr, context);

    EXPECT_CALL(*p_westCompositorImplMock, WstCompositorGetDisplayName(compositor()))
        .WillOnce(testing::Return("AllocationTestDisplay"));
    EXPECT_CALL(*p_waylandImplMock, wl_resource_create(client(), extensionInterface, 1, 123))
        .WillOnce(testing::Return(&resource));
    EXPECT_CALL(*p_waylandImplMock, wl_client_post_no_memory(client())).Times(1);
    EXPECT_CALL(*p_waylandImplMock,
                wl_resource_set_implementation(testing::_, testing::_, testing::_, testing::_))
        .Times(0);

    {
        FailNothrowAllocation failure(sizeof(ExtensionClientInfo));
        EXPECT_NO_THROW(capturedBindFunc(client(), context, 1, 123));
    }
    EXPECT_TRUE(allocationFailed);
    EXPECT_TRUE(context->mClientListMap.empty());
}
