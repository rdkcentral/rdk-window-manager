/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "firebolt_shell.h"
#include "test_Firebolt_Extension.h"


#define MOCK_WST_COMPOSITOR_PTR reinterpret_cast<WstCompositor*>(0x1234)
#define MOCK_WL_DISPLAY_PTR reinterpret_cast<wl_display*>(0x5678)
#define MOCK_WL_CLIENT_PTR reinterpret_cast<wl_client*>(0x9abc)
#define FIREBOLT_SHELL_INTERFACE_VERSION 1
#define FIREBOLT_SHELL_INTERFACE_ID 123

class FireboltShellTest :public FireboltExtensionTest {
protected:
    // Used as a parameter for the capturedBindFunc function. 
    // In original function The fireboltShellCreateContext function initializes the Firebolt shell context, 
    // while wl_global_create creates a global Wayland object for client interactions with the Firebolt shell.
    FireboltShell *p_fireboltShell = nullptr;
    
    struct TestFireboltShellInterface 
    {
        
        void (*get_firebolt_surface)(struct wl_client *client,struct wl_resource *resource,int32_t surfaceId,uint32_t type);
        
        
        TestFireboltShellInterface(): get_firebolt_surface(nullptr){ }

        TestFireboltShellInterface& operator=(const TestFireboltShellInterface& other)
        {
            get_firebolt_surface = other.get_firebolt_surface;
            return *this;
        }
    };

    TestFireboltShellInterface testFireboltShellImpl;

public:
    FireboltShellTest();
    ~FireboltShellTest() override;

    void SetUp() override;
    void TearDown() override;

    void triggerGetShellSurface(const char *id,int32_t surfaceId,uint32_t type) ;
    void triggerDestroy(wl_resource* resource) ;
};


    FireboltShellTest::FireboltShellTest() 
    {
        p_fireboltShell = new FireboltShell();
        assert(nullptr != p_fireboltShell);
    }

    FireboltShellTest::~FireboltShellTest()
    {
        if(p_fireboltShell)
        delete p_fireboltShell;
        p_fireboltShell = nullptr;
    }

    void FireboltShellTest::SetUp() 
    {
        EXPECT_CALL(*p_waylandImplMock, wl_global_create(testing::_, &firebolt_shell_interface, testing::_, testing::_, testing::_))
        .WillOnce(testing::Invoke([&](struct wl_display* display, const struct wl_interface* interface, int version, void* data, wl_global_bind_func_t bind)
        {
            // Capturing the bind function here because the component under test expects a function pointer of 
            // type 'wl_global_bind_func_t'. Just capture the pointer without valid data in order to verify that it is passed correctly.
            capturedBindFunc = bind;
            waylandMockObject = reinterpret_cast<wl_global*>(malloc(sizeof(waylandMockObject)));
            return waylandMockObject;
        }));

        EXPECT_CALL(*p_waylandImplMock, wl_resource_set_implementation(testing::_, testing::_, testing::_, testing::_))
            .WillOnce(testing::Invoke([this](struct wl_resource* resource, const void* implementation, void* data,
                                            wl_resource_destroy_func_t wl_ResrcDestroy) 
        {
            const TestFireboltShellInterface* impl = static_cast<const TestFireboltShellInterface*>(implementation);
            if (impl)
            {
                testFireboltShellImpl = *impl;
                destroy = wl_ResrcDestroy;  
           }                               
        }));

        // Allocate and initialize the mock resource 
        mockResource = static_cast<wl_resource*>(malloc(sizeof(wl_resource)));
        memset(mockResource, 0, sizeof(wl_resource));

        EXPECT_CALL(*p_westCompositorImplMock, WstCompositorGetDisplayName(testing::_))
            .WillOnce(testing::Return("MockDisplayName"));

        EXPECT_CALL(*p_waylandImplMock, wl_resource_create(testing::_, testing::_, testing::_, testing::_))
            .WillOnce(testing::Return(mockResource));
                

        // Call moduleInit with sample value for parameters for testing purposes.
        // The values (MOCK_WST_COMPOSITOR_PTR and MOCK_WL_DISPLAY_PTR) are dummy addresses used to simulate the WstCompositor and wl_display pointers
        bool result = moduleInit(MOCK_WST_COMPOSITOR_PTR, MOCK_WL_DISPLAY_PTR);
        ASSERT_TRUE(result) << "Error: moduleInit failed; initialization was not successful.";
    }

    void FireboltShellTest::TearDown()
    {
        if (waylandMockObject)
        {
            free(waylandMockObject);
            waylandMockObject = nullptr;
        }
        if (mockResource)
        {
            free(mockResource);
            mockResource = nullptr;
        }
        moduleTerm(MOCK_WST_COMPOSITOR_PTR);
    }
    
    /**
    * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
    * Then, triggers the `get_firebolt_surface' callback, which calls the `firebolt_wm_get_clients` function of the original implementation. 
    */
    void FireboltShellTest::triggerGetShellSurface(const char *id ,int32_t surfaceId,uint32_t type ) 
    {
        
        ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltShell, FIREBOLT_SHELL_INTERFACE_VERSION, FIREBOLT_SHELL_INTERFACE_ID);

        ASSERT_TRUE(testFireboltShellImpl.get_firebolt_surface != nullptr) << "Error: testFireboltShellImpl.get_properties is null";
        testFireboltShellImpl.get_firebolt_surface(MOCK_WL_CLIENT_PTR, mockResource, surfaceId, type);
    }
        
        /**
    * Then, triggers the destory callback, which calls the `firebolt_shell_resource_destory` function of the original implementation. 
    */
    void FireboltShellTest::triggerDestroy(wl_resource* resource) 
    {
        ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltShell, FIREBOLT_SHELL_INTERFACE_VERSION, FIREBOLT_SHELL_INTERFACE_ID);

        ASSERT_TRUE(destroy != nullptr) << "Error: destroy is null";
        destroy(resource);
    }


/**
 * @brief Tests the behavior of retrieving client details when the compositor is not valid.
 * 
 * This test verifies that the function can still retrieve the FireboltShell instance associated 
 * with a resource and triggers the appropriate shell surface event when the compositor is not set.
 *
 * @return None.
 */
TEST_F(FireboltShellTest, GetFireboltSurface_CompositorNotValid)
{
    const char* id = "testClientId";
    const std::string clientName = "TestClient";
    const int32_t surfaceId = 1;
    const uint32_t type = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO;
    
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltShell));
    
    triggerGetShellSurface(id,surfaceId,type);
}

/**
 * @brief Tests the retrieval of client details and the notification of an event when the compositor is valid.
 * 
 * This test verifies that the function retrieves the correct client name,
 *  successfully trigger a shell surface event when a valid compositor is set.
 *
 * @return None.
 */
TEST_F(FireboltShellTest, GetFireboltSurface_CompositorValid)
{
    const char* id = "testClientId";
    const std::string clientName = "TestClient";  // Ensure clientName is set correctly.
    const int32_t surfaceId = 1;
    const uint32_t type = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO;
    
    // Set the compositor to a valid mock object pointer
    p_fireboltShell->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    // Expect the correct FireboltShell instance to be retrieved
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltShell));

    // Mock the getClientName call to return the correct client name
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));  // Set clientName correctly
    
    // Expect getFireboltSurface to be called with the correct parameters
    EXPECT_CALL(*p_compositeImplMock, getFireboltSurface(testing::StrEq(clientName), surfaceId, type))
        .WillOnce(testing::Return(true));

    triggerGetShellSurface(id, surfaceId, type);
}

/**
 * @brief Tests the resource destruction behavior when the FireboltShell context is valid and the client is found.
 * 
 * This test verifies that when the FireboltShell context is valid and the associated client info is found,
 * the resource is properly destroyed, and the client info is removed from the client list map.
 *
 * @return None.
 */

TEST_F(FireboltShellTest, ResourceDestroySuccess_FbShellCtxNotNull_ClientFound) 
{
        
    FireboltShellClientInfo clientInfo;
    // Set the compositor and fbShellCtx to valid pointers
    p_fireboltShell->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
    p_fireboltShell->mClientListMap[mockResource] = &clientInfo;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
      .WillRepeatedly(testing::Return(p_fireboltShell));

    EXPECT_CALL(*p_waylandImplMock, wl_resource_set_user_data(mockResource, nullptr));

    triggerDestroy(mockResource);

    // Verify that clientInfo was removed from the map and deleted
    ASSERT_EQ(p_fireboltShell->mClientListMap.find(mockResource), p_fireboltShell->mClientListMap.end());
}

/**
 * @brief Tests the resource destruction behavior when the FireboltShell context is null.
 * 
 * This test verifies that when the FireboltShell context is null, the resource destruction
 * does not remove the client info from the client list map, ie; no action is taken
 * when there is no valid context.
 *
 * @return None.
 */
TEST_F(FireboltShellTest, ResourceDestroyFailed_FbShellCtxNull) 
{
    // Simulate wl_resource_get_user_data returning null (fbShellCtx is null)
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
      .WillRepeatedly(testing::Return(nullptr));

    triggerDestroy(mockResource);
        
        // Verify that clientInfo was not removed from the map
        ASSERT_NE(p_fireboltShell->mClientListMap.find(mockResource), p_fireboltShell->mClientListMap.end());

}

/**
 * @brief Tests the resource destruction behavior when the client is not found in the client list map.
 * 
 * This test verifies that when the FireboltShell context is valid, but the associated resource is
 * not found in the client list map, the resource destruction failed without removing
 * any client info from the map.
 *
 * @return None.
 */

TEST_F(FireboltShellTest, ResourceDestroy_ClientListMapNotFound) 
{
    // Set fbShellCtx to a valid pointer but do not add the resource to the map
    p_fireboltShell->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
    .WillOnce([this]() {
        // Clear the client list map before returning p_fireboltShell
        p_fireboltShell->mClientListMap.clear();
        return p_fireboltShell; // Return the valid pointer after clearing
    });
        
    triggerDestroy(mockResource);
}

/**
 * @brief Tests the resource destruction behavior when client info is null and the resource does not match.
 * 
 * This test verifies that when the client info is null and the resource associated with the client info
 * does not match the resource being destroyed,then it does not remove the client
 * info from the client list map
 *
 * @return None.
 */

TEST_F(FireboltShellTest, ResourceDestroy_ClientInfoNullAndResourceMisMatch) 
{
    // Create a clientInfo instance
    FireboltShellClientInfo clientInfo;

    // Set the compositor and fbShellCtx to valid pointers
    p_fireboltShell->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    // Set clientInfo's resource to a different value than mockResource
    wl_resource *differentResource = reinterpret_cast<wl_resource*>(0x1234); // or any valid resource
    clientInfo.resource = differentResource; // Assign a different resource

    // Add clientInfo to the map with mockResource
    p_fireboltShell->mClientListMap[mockResource] = &clientInfo;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
        .WillRepeatedly(testing::Return(nullptr));

    EXPECT_CALL(*p_waylandImplMock, wl_resource_set_user_data(testing::_, testing::_))
        .Times(0);

    triggerDestroy(mockResource);

    // Verify that clientInfo was not removed from the map
    ASSERT_NE(p_fireboltShell->mClientListMap.find(mockResource), p_fireboltShell->mClientListMap.end());
}
