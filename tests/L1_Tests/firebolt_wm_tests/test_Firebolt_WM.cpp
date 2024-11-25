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
#include "firebolt_wm.h"
#include "test_Firebolt_Extension.h"

#define MOCK_WST_COMPOSITOR_PTR reinterpret_cast<WstCompositor*>(0x1234)
#define MOCK_WL_DISPLAY_PTR reinterpret_cast<wl_display*>(0x5678)
#define MOCK_WL_CLIENT_PTR reinterpret_cast<wl_client*>(0x9abc)
#define FIREBOLT_WM_INTERFACE_VERSION 1
#define FIREBOLT_WM_INTERFACE_ID 123

class FireboltWmTest :public FireboltExtensionTest{
protected:
    // Used as a parameter for the capturedBindFunc function. 
    // In original function The fireboltSurfaceCreateContext function initializes the Firebolt surface context, 
    // while wl_global_create creates a global Wayland object for client interactions with the Firebolt surface.
    FireboltWindowManager *p_fireboltWindowManager = nullptr;
        
    struct TestFireboltWmInterface 
    {
        void (*set_properties)(struct wl_client *client, struct wl_resource *resource, const char *id, int32_t x, int32_t y, uint32_t width, uint32_t height,                               uint32_t render_width, uint32_t render_height, wl_fixed_t opacity, int32_t zorder, int32_t visible, wl_fixed_t crop_x,
                               wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height);
        void (*create)(struct wl_client *client, struct wl_resource *resource, const char *id);
        void (*create_with_bounds)(struct wl_client *client, struct wl_resource *resource, const char *id, 
                                   int32_t x, int32_t y, uint32_t width, uint32_t height);
        void (*create_with_properties)(struct wl_client *client, struct wl_resource *resource, const char *id, int32_t x, int32_t y, uint32_t width, 
                                       uint32_t height, uint32_t display_width, uint32_t display_height, wl_fixed_t opacity, int32_t zorder, int32_t visible,                                       wl_fixed_t crop_x, wl_fixed_t crop_y, wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t focused);
        void (*destroy)(struct wl_client *client, struct wl_resource *resource, const char *id);
        void (*set_client_bounds)(struct wl_client *client, struct wl_resource *resource, const char *id, int32_t x, int32_t y, uint32_t width, 
                                  uint32_t height);
        void (*set_client_display_bounds)(struct wl_client *client, struct wl_resource *resource, const char *id, uint32_t width, uint32_t height);
                 
        void (*set_client_focus)(struct wl_client *client,struct wl_resource *resource,const char *id);
        void (*get_properties)(struct wl_client *client,struct wl_resource *resource,const char *id);
        void (*get_focused_client)(struct wl_client *client,struct wl_resource *resource);
        void (*get_clients)(struct wl_client *client,struct wl_resource *resource);
                

        TestFireboltWmInterface& operator=(const TestFireboltWmInterface& other)
        {
           if (this != &other) 
           {
                set_properties = other.set_properties;
                create = other.create;
                create_with_bounds = other.create_with_bounds;
                create_with_properties = other.create_with_properties;
                destroy = other.destroy;
                set_client_bounds = other.set_client_bounds;
                set_client_display_bounds = other.set_client_display_bounds;
                set_client_focus = other.set_client_focus;
                get_properties = other.get_properties;
                get_focused_client = other.get_focused_client;
                get_clients = other.get_clients;
           }
           return *this;
        }
    };

    TestFireboltWmInterface testFireboltWmImpl;
        
public:
    FireboltWmTest();
    ~FireboltWmTest() override;

    void SetUp() override;
    void TearDown() override;

    void triggerSetProperties(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height,
                              uint32_t render_width, uint32_t render_height, wl_fixed_t opacity,
                              int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y,
                              wl_fixed_t crop_width, wl_fixed_t crop_height);

    void triggerCreateDisplay(const char* id);
    void triggerCreateDisplayWithBounds(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height);
    void triggerCreateWithProperties(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height,
                                     uint32_t display_width, uint32_t display_height, wl_fixed_t opacity,
                                     int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y,
                                     wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t focused);

    void triggerDestroy(const char* id);
    void triggerSetClientBounds(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height);
    void triggerSetClientDisplayBounds(const char* id, uint32_t width, uint32_t height);
    void triggerSetClientFocus(const char *id);
    void triggerGetProperties(const char *id);
    void triggerGetFocusedClient(const char *id); 
    void triggerGetClient(const char *id) ;
    void destroyResource(wl_resource* resource) ;
};


    FireboltWmTest::FireboltWmTest() 
    {
        p_fireboltWindowManager = new FireboltWindowManager();
        assert(nullptr != p_fireboltWindowManager);
    }

    FireboltWmTest::~FireboltWmTest()
    {
        if(p_fireboltWindowManager)
        delete p_fireboltWindowManager;
        p_fireboltWindowManager = nullptr;
    }

    void FireboltWmTest::SetUp() 
    {
        EXPECT_CALL(*p_waylandImplMock, wl_global_create(testing::_, &firebolt_wm_interface, testing::_, testing::_, testing::_))
        .WillOnce(testing::Invoke([&](struct wl_display* display, const struct wl_interface* interface, int version, void* data, wl_global_bind_func_t bind)
        {
            capturedBindFunc = bind;
            waylandMockObject = reinterpret_cast<wl_global*>(malloc(sizeof(waylandMockObject)));
            return waylandMockObject;
        }));

        EXPECT_CALL(*p_waylandImplMock, wl_resource_set_implementation(testing::_, testing::_, testing::_, testing::_))
            .WillOnce(testing::Invoke([this](struct wl_resource* resource, const void* implementation, void* data, wl_resource_destroy_func_t wl_ResrcDestroy) 
        {
            const TestFireboltWmInterface* impl = static_cast<const TestFireboltWmInterface*>(implementation);
            if (impl)
               testFireboltWmImpl = *impl;
                       destroy = wl_ResrcDestroy;
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
    }

    void FireboltWmTest::TearDown()
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
        
        
        
class FireboltWmClientInfoTest : public ::testing::Test
{
    protected:
    NiceMock<WaylandServerMockImpl> *p_waylandImplMock;
    FireboltWindowManager *p_fireboltWindowManager;
    FireboltWmClientInfoTest();
    ~FireboltWmClientInfoTest() override;
};

    FireboltWmClientInfoTest::FireboltWmClientInfoTest() 
    {
        p_waylandImplMock = new NiceMock<WaylandServerMockImpl>;
        assert(nullptr != p_waylandImplMock);
        WaylandServer::setImpl(p_waylandImplMock);

        p_fireboltWindowManager = new FireboltWindowManager();
        assert(nullptr != p_fireboltWindowManager);
    }

    FireboltWmClientInfoTest::~FireboltWmClientInfoTest()
    {
        delete p_fireboltWindowManager;
        WaylandServer::setImpl(nullptr);
        delete p_waylandImplMock;
    }



     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `set_properties` callback, which calls the `firebolt_wm_set_properties` function of the original implementation. 
     */

    void FireboltWmTest::triggerSetProperties(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height,
                              uint32_t render_width, uint32_t render_height, wl_fixed_t opacity, 
                              int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, 
                              wl_fixed_t crop_width, wl_fixed_t crop_height) 
    {
        if (capturedBindFunc)
        {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        }
        else 
        {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.set_properties != nullptr) << "Error: testFireboltWmImpl..set_properties is null";
        if (testFireboltWmImpl.set_properties)
        {
            testFireboltWmImpl.set_properties(MOCK_WL_CLIENT_PTR, mockResource, id, x, y, width,
                                              height, render_width, render_height, opacity, zorder, visible,
                                              crop_x, crop_y, crop_width, crop_height);
        } 
    }

     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `create` callback, which calls the `firebolt_wm_create` function of the original implementation. 
     */
    void FireboltWmTest::triggerCreateDisplay(const char* id)
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.create != nullptr) << "Error: testFireboltWmImpl.create is null";
        if (testFireboltWmImpl.create) {
            testFireboltWmImpl.create(MOCK_WL_CLIENT_PTR, mockResource, id);
        } 
    }
    
     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `create_with_bounds` callback, which calls the `firebolt_wm_create_with_bounds` function of the original implementation. 
     */
    void FireboltWmTest::triggerCreateDisplayWithBounds(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.create_with_bounds != nullptr) << "Error: testFireboltWmImpl.create_with_bounds is null";
        if (testFireboltWmImpl.create_with_bounds) {
            testFireboltWmImpl.create_with_bounds(MOCK_WL_CLIENT_PTR, mockResource, id, x, y, width, height);
        }
    }
    
     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `create_with_properties` callback, which calls the `firebolt_wm_create_with_properties` function of the original implementation. 
     */
    void FireboltWmTest::triggerCreateWithProperties(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height,
                                     uint32_t display_width, uint32_t display_height, wl_fixed_t opacity, 
                                     int32_t zorder, int32_t visible, wl_fixed_t crop_x, wl_fixed_t crop_y, 
                                     wl_fixed_t crop_width, wl_fixed_t crop_height, int32_t focused) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.create_with_properties != nullptr) << "Error: testFireboltWmImpl.create_with_properties is null";
        if (testFireboltWmImpl.create_with_properties) {
            testFireboltWmImpl.create_with_properties(MOCK_WL_CLIENT_PTR, mockResource, id, x, y, width,
                                                      height, display_width, display_height, opacity, zorder, visible, 
                                                      crop_x, crop_y, crop_width, crop_height, focused);
        }
    }

     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `destroy` callback, which calls the `firebolt_wm_destroy` function of the original implementation. 
     */
    void FireboltWmTest::triggerDestroy(const char* id) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.destroy != nullptr) << "Error: testFireboltWmImpl.destroy is null";
        if (testFireboltWmImpl.destroy) {
            testFireboltWmImpl.destroy(MOCK_WL_CLIENT_PTR, mockResource, id);
        }
    }

     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `set_client_bounds` callback, which calls the `firebolt_wm_set_client_bounds` function of the original implementation. 
     */
    void FireboltWmTest::triggerSetClientBounds(const char* id, int32_t x, int32_t y, uint32_t width, uint32_t height) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.set_client_bounds != nullptr) << "Error: testFireboltWmImpl.set_client_bounds is null";
        if (testFireboltWmImpl.set_client_bounds) {
            testFireboltWmImpl.set_client_bounds(MOCK_WL_CLIENT_PTR, mockResource, id, x, y, width, height);
        }
    }
     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `set_client_display_bounds` callback, which calls the `firebolt_wm_set_client_display_bounds`
     * function of the original implementation. 
     */
        
    void FireboltWmTest::triggerSetClientDisplayBounds(const char* id, uint32_t width, uint32_t height)
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.set_client_display_bounds != nullptr) << "Error:testFireboltWmImpl.set_client_display_bounds is null";
        if (testFireboltWmImpl.set_client_display_bounds) {
            testFireboltWmImpl.set_client_display_bounds(MOCK_WL_CLIENT_PTR, mockResource, id, width, height);
        }
    }
   
    /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `set_client_focus` callback, which calls the `firebolt_wm_set_client_focus` function of the original implementation. 
     */
  
    void FireboltWmTest::triggerSetClientFocus(const char *id)
    {
       if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.set_client_focus != nullptr) << "Error: testFireboltWmImpl.set_client_focus is null";
        if (testFireboltWmImpl.set_client_focus)
                {
            testFireboltWmImpl.set_client_focus(MOCK_WL_CLIENT_PTR, mockResource,id);
        }
    }

    /**
    * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
    * Then, triggers the `get_properties' callback, which calls the `firebolt_wm_get_properties` function of the original implementation. 
    */
    void FireboltWmTest::triggerGetProperties(const char *id) 
    {
        if (capturedBindFunc) 
        {
           capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.get_properties != nullptr) << "Error: testFireboltWmImpl.get_properties is null";
        if (testFireboltWmImpl.get_properties)
        {
            testFireboltWmImpl.get_properties(MOCK_WL_CLIENT_PTR, mockResource,id);
        }
    }

    /**
    * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
    * Then, triggers the `get_focused_client' callback, which calls the `firebolt_wm_get_focused_client` function of the original implementation. 
    */
    void FireboltWmTest::triggerGetFocusedClient(const char *id) 
    {
        if (capturedBindFunc) 
        {
           capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.get_focused_client != nullptr) << "Error: testFireboltWmImpl.get_properties is null";
        if (testFireboltWmImpl.get_focused_client)
        {
            testFireboltWmImpl.get_focused_client(MOCK_WL_CLIENT_PTR, mockResource);
        }
    }
    /**
    * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
    * Then, triggers the `get_clients' callback, which calls the `firebolt_wm_get_clients` function of the original implementation. 
    */
    void FireboltWmTest::triggerGetClient(const char *id) 
    {
        if (capturedBindFunc) 
        {
           capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltWmImpl.get_clients != nullptr) << "Error: testFireboltWmImpl.get_properties is null";
        if (testFireboltWmImpl.get_clients)
        {
            testFireboltWmImpl.get_clients(MOCK_WL_CLIENT_PTR, mockResource);
        }
    }

    /**
    * Triggers the destory callback, which calls the `firebolt_wm_resource_destory` function of the original implementation. 
    */
    void FireboltWmTest::destroyResource(wl_resource* resource) 
    {
        ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltWindowManager, FIREBOLT_WM_INTERFACE_VERSION, FIREBOLT_WM_INTERFACE_ID);

        ASSERT_TRUE(destroy != nullptr) << "Error: destroy is null";
        destroy(resource);
    }


/**
 * @brief Test setProperties with valid parameters.
 * Verifies that setProperties correctly calls setClientInfo with valid parameters
 *
 * @return None.
 */

TEST_F(FireboltWmTest, setProperties_withSetClientSuccess) {
    const char* id = "test_id";
    int32_t x = 10, y = 20;
    uint32_t width = 100, height = 200;
    uint32_t render_width = 0, render_height = 0;
    wl_fixed_t opacity = wl_fixed_from_double(1.0);
    int32_t zorder = 0, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(0), crop_y = wl_fixed_from_int(0);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::StrEq(id), testing::_))
        .WillOnce(testing::Return(true));

    triggerSetProperties(id, x, y, width, height, render_width, render_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height);
}


/**
 * @brief Test setProperties with a failed client info.
 * Verifies that setProperties attempts to setClientInfo but fails due to invalid client properties.
 * Verifies that setProperties does not call enableVirtualDisplay or setVirtualResolution
 *
 * @return None.
 */
TEST_F(FireboltWmTest, setProperties_withSetClientInfoFailed)
 {
    const char* id = "test_id_fail";
    int32_t x = 10, y = 20;
    uint32_t width = 100, height = 200;
    uint32_t render_width = 0, render_height = 0;  
    wl_fixed_t opacity = wl_fixed_from_double(0.5);
    int32_t zorder = 0, visible = 0;
    wl_fixed_t crop_x = wl_fixed_from_int(0), crop_y = wl_fixed_from_int(0);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);
        
    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::StrEq(id), testing::_))
        .WillOnce(testing::Return(false));
                
    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay(testing::StrEq(id), true))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(testing::StrEq(id), render_width, render_height))
        .Times(0);

    triggerSetProperties(id, x, y, width, height, render_width, render_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height);
}

/**
 * @brief Test setProperties with NULL client ID.
 * Verifies that setProperties does not call setClientInfo when the client ID is NULL.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, setProperties_withInvalidParam) 
{
    const char* id = NULL;  // Simulate NULL id
    int32_t x = 10, y = 20;
    uint32_t width = 100, height = 200;
    uint32_t render_width = 0, render_height = 0;
    wl_fixed_t opacity = wl_fixed_from_double(0.5);
    int32_t zorder = 0, visible = 0;
    wl_fixed_t crop_x = wl_fixed_from_int(0), crop_y = wl_fixed_from_int(0);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .Times(0);

    triggerSetProperties(id, x, y, width, height, render_width, render_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height);
}

/**
 * @brief Test setProperties with valid rendering dimensions.
 * Verifies that setProperties correctly calls enableVirtualDisplay and setVirtualResolution
 * when valid render dimensions are provided.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, setProperties_withValidDimensions) 
{
    const char* id = "test_id";
    int32_t x = 10, y = 20;
    uint32_t width = 100, height = 200;
    uint32_t render_width = 1920, render_height = 1080; // Valid dimensions
    wl_fixed_t opacity = wl_fixed_from_double(1.0);
    int32_t zorder = 0, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(0), crop_y = wl_fixed_from_int(0);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);

    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay(testing::StrEq(id), false))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(testing::StrEq(id), render_width, render_height))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::StrEq(id), testing::_))
        .WillOnce(testing::Return(true));

    triggerSetProperties(id, x, y, width, height, render_width, render_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height);
}


/**
 * @brief Test setProperties with zero render dimensions.
 * Verifies that setProperties does not call enableVirtualDisplay or setVirtualResolution
 * when render dimensions are set to zero.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, setProperties_withZeroDimensions) 
{
    const char* id = "test_id";
    int32_t x = 10, y = 20;
    uint32_t width = 100, height = 200;
    uint32_t render_width = 0, render_height = 0; // Invalid dimensions
    wl_fixed_t opacity = wl_fixed_from_double(1.0);
    int32_t zorder = 0, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(0), crop_y = wl_fixed_from_int(0);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);

    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay(testing::StrEq(id), true))
        .Times(1);
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(testing::StrEq(id), render_width, render_height))
        .Times(1);

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::StrEq(id), testing::_))
        .WillOnce(testing::Return(true));

    triggerSetProperties(id, x, y, width, height, render_width, render_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height);
}


/**
 * @brief  Tests when getClientInfo succeeds, ensuring getTopmost is not called.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplay_withoutTopmostQuery)
{
    const char* id = "topmost_display_id";
    
    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, getClientInfo(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(RdkWindowManager::ClientInfo{ .zorder = 5 }), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, getTopmost(testing::_)).Times(0);

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(true));

    triggerCreateDisplay(id);
}
/**
 * @brief  Tests when getClientInfo fails and getTopmost called
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplay_withTopmostQueryOnClientInfoFailure)
{
    const char* id = "topmost_display_id";
    std::string topClientName = "top_client";
    
    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, getClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*p_compositeImplMock, getTopmost(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(topClientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, getClientInfo(topClientName, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(RdkWindowManager::ClientInfo{ .zorder = 5 }), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(true));

    triggerCreateDisplay(id);
}
/**
 * @brief   Tests when both getClientInfo and getTopmost fail.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, createDisplay_withClientInfo_NoTopmost) {
    const char* id = "topmost_display_id";

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, getClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*p_compositeImplMock, getTopmost(testing::_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(true));

    triggerCreateDisplay(id);
}
/**
 * @brief   Tests when createDisplay itself fails.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, createDisplay_withCreateDisplayFailure)
{
    const char* id = "topmost_display_id";

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*p_compositeImplMock, getClientInfo(testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, getTopmost(testing::_))
        .Times(0);

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .Times(0);

    triggerCreateDisplay(id);
}

/**
 * @brief Test createDisplayWithBounds with successful display creation and successful bounds setting.
 * Verifies that createDisplay and setBounds are called successfully when valid parameters are provided.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithBounds_withCreateDisplaySuccess)
{
    const char* id = "display_with_bounds_id";
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_, testing::_,testing::_, testing::_,testing::_))
        .WillOnce(testing::Return(true));

    triggerCreateDisplayWithBounds(id, x, y, width, height);
}

/**
 * @brief Test createDisplayWithBounds with successful display creation and failure in setting bounds.
 * Verifies that createDisplay is called successfully, but setBounds fails when valid parameters are provided.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, createDisplayWithBounds_withSetBoundsFailure)
{
    const char* id = "display_with_bounds_id_set_bounds_fail";
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));

    triggerCreateDisplayWithBounds(id, x, y, width, height);
}

/**
 * @brief Test createDisplayWithBounds with failure in display creation and valid bounds setting.
 * Verifies that createDisplay fails and setBounds is not called when the display creation fails.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithBounds_withCreateDisplayFailure)
{
    const char* id = "display_with_bounds_id_fail";
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_, testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    triggerCreateDisplayWithBounds(id, x, y, width, height);
}

/**
 * @brief Test createDisplayWithBounds with an invalid ID.
 * Verifies that neither createDisplay nor setBounds is called when an invalid ID is provided.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithBounds_withInvalidId)
{
    const char* id = nullptr;  // Invalid id 
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .Times(0);

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_, testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    triggerCreateDisplayWithBounds(id, x, y, width, height);
}

/**
 * @brief Test createDisplayWithProperties with successful display creation and successful client info setting.
 * Verifies that createDisplay and setClientInfo are called successfully when valid parameters are provided.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithProperties_whenCreateDisplaySuccess)
{
    const char* id = "display_with_properties_id";
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;
    uint32_t display_width = 1920, display_height = 1080;
    wl_fixed_t opacity = wl_fixed_from_double(0.5);
    int32_t zorder = 1, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(10), crop_y = wl_fixed_from_int(20);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);
    int32_t focused = 1;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(true));

    triggerCreateWithProperties(id, x, y, width, height, display_width, display_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, focused);
}


/**
 * @brief Test createDisplayWithProperties with successful display creation and failure in setting client info.
 * Verifies that createDisplay is called successfully, but setClientInfo fails when valid parameters are provided.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithProperties_onSetClientInfoFailure)
{
    const char* id = "display_with_properties_id_fail";
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;
    uint32_t display_width = 1920, display_height = 1080;
    wl_fixed_t opacity = wl_fixed_from_double(0.5);
    int32_t zorder = 1, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(10), crop_y = wl_fixed_from_int(20);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);
    int32_t focused = 1;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(false));

    triggerCreateWithProperties(id, x, y, width, height, display_width, display_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, focused);
}

/**
 * @brief Test createDisplayWithProperties with failure in display creation and valid client info setting.
 * Verifies that createDisplay fails and setClientInfo is not called when the display creation fails.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithProperties_onCreateDisplayFailure)
{
    const char* id = "display_with_properties_id_fail";
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;
    uint32_t display_width = 1920, display_height = 1080;
    wl_fixed_t opacity = wl_fixed_from_double(0.5);
    int32_t zorder = 1, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(10), crop_y = wl_fixed_from_int(20);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);
    int32_t focused = 1;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .Times(0);

    triggerCreateWithProperties(id, x, y, width, height, display_width, display_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, focused);
}
/**
 * @brief Test createDisplayWithProperties with invalid parameters.
 * Verifies that neither createDisplay nor setClientInfo is called when invalid parameters are provided.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, createDisplayWithProperties_withInvalidParam)
{
    const char* id = nullptr; // Invalid id 
    int32_t x = 50, y = 100;
    uint32_t width = 1280, height = 720;
    uint32_t display_width = 1920, display_height = 1080;
    wl_fixed_t opacity = wl_fixed_from_double(0.5);
    int32_t zorder = 1, visible = 1;
    wl_fixed_t crop_x = wl_fixed_from_int(10), crop_y = wl_fixed_from_int(20);
    wl_fixed_t crop_width = wl_fixed_from_int(100), crop_height = wl_fixed_from_int(200);
    int32_t focused = 1;

    EXPECT_CALL(*p_compositeImplMock, createDisplay(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
        .Times(0);

    EXPECT_CALL(*p_compositeImplMock, setClientInfo(testing::_, testing::_))
        .Times(0);

    triggerCreateWithProperties(id, x, y, width, height, display_width, display_height, opacity, zorder, visible, crop_x, crop_y, crop_width, crop_height, focused);
}

/**
 * @brief Test the successful destruction of a display.
 * Verifies the behavior of destroy function when the `kill` method is success
 *
 * @return None.
 */
TEST_F(FireboltWmTest, destroy_withKillSuccess)
{
    const char* id = "display_to_destroy";

    EXPECT_CALL(*p_compositeImplMock, kill(testing::_))
        .WillOnce(testing::Return(true));
    
    triggerDestroy(id);
}

/**
 * @brief Test the destroy with kill failed
 * Verifies the behavior of destroy function when the `kill` method is fail
 *
 * @return None.
 */
TEST_F(FireboltWmTest, destroy_withKillFailure)
{
    const char* id = "display_to_destroy_fail";

    EXPECT_CALL(*p_compositeImplMock, kill(testing::_))
        .WillOnce(testing::Return(false));

    triggerDestroy(id);
}

/**
 * @brief Test the behavior of destroy with an invalid parameter.
 * Verifies that the `kill` method is not called when the ID is nullptr.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, destroy_withInvalidParam)
{
    const char* id = nullptr; // Invalid id 

    EXPECT_CALL(*p_compositeImplMock, kill(testing::_))
        .Times(0);

    triggerDestroy(id);
}

/**
 * @brief Test setting client bounds successfully with valid parameters.
 * Verifies  the behavior of the setClientBounds where setBounds success
 *
 * @return None.
 */
TEST_F(FireboltWmTest, setClientBounds_OnSetBoundSuccess)
{
    const char* id = "valid_id";
    int32_t x = 10;
    int32_t y = 20;
    uint32_t width = 100;
    uint32_t height = 200;

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_,testing::_,testing::_,testing::_,testing::_))
        .WillOnce(testing::Return(true));

    triggerSetClientBounds(id, x, y, width, height);

}

/**
 * @brief Test setting client bounds with a failure in setBounds.
 * Verifies  the behavior of the setClientBounds where setBounds fail
 * 
 * @return None.
 */

TEST_F(FireboltWmTest, setClientBounds_onSetBoundFailure)
{
    const char* id = "valid_id";
    int32_t x = 10;
    int32_t y = 20;
    uint32_t width = 100;
    uint32_t height = 200;

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_,testing::_,testing::_,testing::_,testing::_))
        .WillOnce(testing::Return(false));

    triggerSetClientBounds(id, x, y, width, height);

}

/**
 * @brief Test setting client bounds with an invalid parameter.
 * Verifies that `setBounds` is not called when the ID is nullptr.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, setClientBounds_withInvalidParam) 
{
    const char* id = nullptr; // Invalid id 
    int32_t x = 10;
    int32_t y = 20;
    uint32_t width = 100;
    uint32_t height = 200;

    EXPECT_CALL(*p_compositeImplMock, setBounds(testing::_,testing::_,testing::_,testing::_,testing::_))
        .Times(0);
    triggerSetClientBounds(id, x, y, width, height);

}


/**
 * @brief Test sets client display window bounds when getVirtualDisplayEnabled false
 * Verifies that enableVirtualDisplay and setVirtualResolution not called when getVirtualDisplayEnabled returns false
 *
 * @return None.
 */

TEST_F(FireboltWmTest, SetClientDisplayBounds_whenGetVirtualDisplayEnabledFalse)
{
    const char* id = "valid_id";
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool isVirtualDispEnabled = false;

    EXPECT_CALL(*p_compositeImplMock, getVirtualDisplayEnabled(id, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(isVirtualDispEnabled), testing::Return(false)));

 
    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay).Times(0);
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution).Times(0);

    triggerSetClientDisplayBounds(id,width,height);
    
}

/**
 * @brief Test sets client display window bounds when EnableVirtualDisplay returns false
 * Verifies that setVirtualResolution  called when EnableVirtualDisplay returns false
 *
 * @return None.
 */
TEST_F(FireboltWmTest, SetClientDisplayBounds_whenEnableVirtualDisplayFalse)
{
    const char* id = "valid_id";
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool isVirtualDispEnabled = false;

    EXPECT_CALL(*p_compositeImplMock, getVirtualDisplayEnabled(id, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(isVirtualDispEnabled), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay(id, true))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(id, width, height))
         .Times(1);

    triggerSetClientDisplayBounds(id,width,height);
}

/**
 * @brief Test sets client display window bounds when getVirtualDisplayEnabled ,enableVirtualDisplay ,setVirtualResolution returns sucessflly
 * Verifies the setClientDisplay bounds success  when getVirtualDisplayEnabled ,enableVirtualDisplay, setVirtualResolution returns true
 *
 * @return None.
 */

TEST_F(FireboltWmTest, SetClientDisplayBounds_whenAllFunctionsTrue)
{
    const char* id = "valid_id";
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool isVirtualDispEnabled = false;

    EXPECT_CALL(*p_compositeImplMock, getVirtualDisplayEnabled(id, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(isVirtualDispEnabled), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay(id, true))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(id, width, height))
        .WillOnce(testing::Return(true));

    triggerSetClientDisplayBounds(id,width,height);
    
}

/**
 * @brief Test sets client display window bounds with invalid Id
 * ies the setClientDisplay bounds will not call getVirtualDisplayEnabled ,enableVirtualDisplay, setVirtualResolution with invalid input id
 *
 * @return None.
 */
TEST_F(FireboltWmTest, SetClientDisplayBounds_withInvalidParam)
{
    const char* id = nullptr; // Invalid id 
    uint32_t width = 1920;
    uint32_t height = 1080;

    EXPECT_CALL(*p_compositeImplMock, getVirtualDisplayEnabled(testing::_, testing::_))
        .Times(0);
                
    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay(testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(testing::_, testing::_ ,testing::_))
        .Times(0);

    triggerSetClientDisplayBounds(id, width, height);
}
/**
 * @brief Test sets client display window bounds when isVirtualDispEnabled set true
 * Verifies that enableVirtualDisplay never called and setVirtualResolution will get  called when isVirtualDispEnabled set true
 *
 * @return None.
 */
TEST_F(FireboltWmTest, SetClientDisplayBounds_withVirtualDisplayEnabledTrue)
{
    const char* id = "valid_id";
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool isVirtualDispEnabled = true;

    EXPECT_CALL(*p_compositeImplMock, getVirtualDisplayEnabled(id, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(isVirtualDispEnabled), testing::Return(true)));
    EXPECT_CALL(*p_compositeImplMock, enableVirtualDisplay).Times(0);
    EXPECT_CALL(*p_compositeImplMock, setVirtualResolution(id, width, height))
         .Times(1);

    triggerSetClientDisplayBounds(id,width,height);
}
        
/**
 * @brief Test setting client focus successfully with a valid ID.
 * Verifies that the focus is set correctly when setFocus returns true.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, SetClientFocus_whenSetFocusSuccess) 
{
    const char* id = "valid_id";

    EXPECT_CALL(*p_compositeImplMock, setFocus(id))
        .WillOnce(testing::Return(true));

    
    triggerSetClientFocus(id);
}

/**
 * @brief Test setting client focus when setFocus fails.
 * Verifies that the function handles failure correctly when setFocus returns false.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, SetClientFocus_whenSetFocusFailure) 
{
    const char* id = "valid_id";

    EXPECT_CALL(*p_compositeImplMock, setFocus(id))
        .WillOnce(testing::Return(false));

    triggerSetClientFocus(id);
}

/**
 * @brief Test setting client focus with a NULL ID.
 * Verifies that the function handles a NULL ID parameter correctly and logs an error.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, SetClientFocus_withInvalidParam) 
{
    const char* id = nullptr; // NULL ID

    EXPECT_CALL(*p_compositeImplMock, setFocus(testing::_))
        .Times(0);

    triggerSetClientFocus(id);
}

/**
 * @brief Test GetProperties when GetClientInfoSuccess
 * verify  when getClientInfo succeeds, the same values are passed to wl_resource_post_event_for_client_properties, 
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetProperties_whenGetClientInfoSuccess)
{
    const char* id = "testClient";

    // values returned by getClientInfo
    RdkWindowManager::ClientInfo fetchedClientInfo;
    fetchedClientInfo.x = 50;
    fetchedClientInfo.y = 100;
    fetchedClientInfo.width = 1920;
    fetchedClientInfo.height = 1080;
    fetchedClientInfo.opacity = 0.9;
    fetchedClientInfo.zorder = 5;
    fetchedClientInfo.visible = 1;
    fetchedClientInfo.cropX = 0;
    fetchedClientInfo.cropY = 0;
    fetchedClientInfo.cropWidth = 1920;
    fetchedClientInfo.cropHeight = 1080;

    // Mock the getClientInfo function to return the captured values
    EXPECT_CALL(*p_compositeImplMock, getClientInfo(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(fetchedClientInfo), testing::Return(true)));

    // Expect wl_resource_post_event_for_client_properties to be called with the exact same values as returned by getClientInfo
    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_client_properties(
        mockResource, FIREBOLT_WM_CLIENT_PROPERTIES, id, 
        fetchedClientInfo.x, fetchedClientInfo.y,
        fetchedClientInfo.width, fetchedClientInfo.height, 
        wl_fixed_from_double(fetchedClientInfo.opacity),
        fetchedClientInfo.zorder, fetchedClientInfo.visible,
        wl_fixed_from_int(fetchedClientInfo.cropX), wl_fixed_from_int(fetchedClientInfo.cropY),
        wl_fixed_from_int(fetchedClientInfo.cropWidth), wl_fixed_from_int(fetchedClientInfo.cropHeight), 0))
        .Times(1);

    triggerGetProperties(id);
}

/**
 * @brief Test GetProperties when GetClientInfo failed
 * verify  when getClientInfo fails,  wl_resource_post_event_for_client_properties never get called
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetProperties_onGetClientInfoFailed) 
{
    const char* id = "testClient";

    EXPECT_CALL(*p_compositeImplMock, getClientInfo(testing::_, testing::_))
        .WillOnce(testing::Return(false));

    // Expect that wl_resource_post_event_for_client_properties should not be called with the expected values
    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_client_properties(
        mockResource, FIREBOLT_WM_CLIENT_PROPERTIES, id,
        testing::_, testing::_, testing::_, testing::_,
        testing::_, testing::_, testing::_, testing::_,
        testing::_, testing::_, testing::_ ,testing::_))
        .Times(0);

    triggerGetProperties(id);
}

/**
 * @brief Test getting focused client details and sends the focused_client
 * Verifies that the function successfully getting focused client details and  Notifying focused_client event to the caller of the app id
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetFocusedClient_whenGetFocusedSuccess)
{
    const char* id = "testClient";    

   
    EXPECT_CALL(*p_compositeImplMock, getFocused(testing::_))
    .WillOnce(testing::DoAll(
        testing::SetArgReferee<0>(id),testing::Return(true)  
    ));

    // Expect wl_resource_post_event_for_focused_client to be called with the exact same values as returned by getClientInfo
    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_focused_client(
       mockResource, FIREBOLT_WM_FOCUSED_CLIENT, testing::StrEq(id)))
      .Times(1);
        
    triggerGetFocusedClient(id);
}

/**
 * @brief Test getting focused client details failed
 * Verifies that the function failed to getting focused client details and  not to Notify focused_client event 
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetFocusedClient_onGetFocusedFailed) 
{
    const char* id = "testClient";

    EXPECT_CALL(*p_compositeImplMock, getFocused(testing::_))
    .WillOnce(testing::DoAll(
        testing::SetArgReferee<0>(id),testing::Return(false)  
    ));


    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_focused_client(
        mockResource, FIREBOLT_WM_FOCUSED_CLIENT, id))
        .Times(0);

    triggerGetFocusedClient(id);
}

/**
 * @brief Test getting the list of client details and Sends an clients event to the client owning the resource.
 * Verifies that the function successfully getting client details and   Notify client event 
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetClients_onRetrievedClientList)
{
    const char* id = "testClient";
    std::vector<std::string> mockClients = { "client1", "client2", "client3" };
    std::string expectedClientList = "client1,client2,client3";
    
    wl_client* mockClient = nullptr;
    wl_resource* mockResource = nullptr;

    EXPECT_CALL(*p_compositeImplMock, getClients(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mockClients), testing::Return(true)));  // Assuming getClients returns true on success

    // Expect firebolt_wm_send_clients to be called with the expected comma-separated list
    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_client(testing::_, testing::StrEq(expectedClientList.c_str())))
        .Times(1);
    
    triggerGetClient(id);

}

/**
 * @brief Test getting empty  client details  and  Sends an clients event to the client owning the resource with emptly list.
 * Verifies that the function succeeded when  getting client details as empty and send empty list in the  notification client event 
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetClients_onEmptyClientList)
{
    const char* id = "testClient";
    // empty client list
    std::vector<std::string> mockClients = {};
    
    wl_client* mockClient = nullptr;  // Mock wl_client as appropriate
    wl_resource* mockResource = nullptr;  // Mock wl_resource as appropriate

    EXPECT_CALL(*p_compositeImplMock, getClients(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mockClients), testing::Return(true)));  // Assuming getClients returns true on success

    // Expect firebolt_wm_send_clients to be called with an empty string
    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_client(testing::_, testing::StrEq("")))
        .Times(1);

    triggerGetClient(id);

}

/**
 * @brief Test getting the list of client details failed and  Sends an clients event to the client owning the resource with emptylist.
 * Verifies that the function failed when  getting client details and Notify empty client list
 *
 * @return None.
 */
TEST_F(FireboltWmTest, GetClients_onFailedGetClients)
{
    const char* id = "testClient";
    // empty client list
    std::vector<std::string> mockClients = {};
    
    wl_client* mockClient = nullptr;  // Mock wl_client as appropriate
    wl_resource* mockResource = nullptr;  // Mock wl_resource as appropriate

    EXPECT_CALL(*p_compositeImplMock, getClients(testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<0>(mockClients), testing::Return(false)));  // Assuming getClients returns true on success

    // Expect firebolt_wm_send_clients to be called with an empty string
    EXPECT_CALL(*p_waylandImplMock, wl_resource_post_event_for_client(testing::_, testing::StrEq("")))
        .Times(1);
    triggerGetClient(id);

}


/**
 * @brief Tests the resource destruction behavior when the FireboltWM context is valid and the client is found.
 * 
 * This test verifies that when the FireboltWM context is valid and the associated client info is found,
 * the resource is properly destroyed, and the client info is removed from the client list map.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, ResourceDestroySuccess_FbWMCtxNotNull_ClientFound) 
{
        
        FireboltWmClientInfo clientInfo;
    // Set the compositor and fbWMCtx to valid pointers
    p_fireboltWindowManager->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
    p_fireboltWindowManager->mClientListMap[mockResource] = &clientInfo;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
      .WillRepeatedly(testing::Return(p_fireboltWindowManager));

    EXPECT_CALL(*p_waylandImplMock, wl_resource_set_user_data(mockResource, nullptr));

    destroyResource(mockResource);

    // Verify that clientInfo was removed from the map and deleted
    ASSERT_EQ(p_fireboltWindowManager->mClientListMap.find(mockResource), p_fireboltWindowManager->mClientListMap.end());
}

/**
 * @brief Tests the resource destruction behavior when the FireboltWM context is null.
 * 
 * This test verifies that when the FireboltWM context is null, the resource destruction
 * does not remove the client info from the client list map, ie; no action is taken
 * when there is no valid context.
 *
 * @return None.
 */
TEST_F(FireboltWmTest, ResourceDestroyFailed_FbWMCtxNull) 
{
    // Simulate wl_resource_get_user_data returning null (fbWMCtx is null)
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
      .WillRepeatedly(testing::Return(nullptr));

    destroyResource(mockResource);
        
    // Verify that clientInfo was not removed from the map
    ASSERT_NE(p_fireboltWindowManager->mClientListMap.find(mockResource), p_fireboltWindowManager->mClientListMap.end());

}

/**
 * @brief Tests the resource destruction behavior when the client is not found in the client list map.
 * 
 * This test verifies that when the FireboltWM context is valid, but the associated resource is
 * not found in the client list map, the resource destruction failed without removing
 * any client info from the map.
 *
 * @return None.
 */

TEST_F(FireboltWmTest, ResourceDestroy_ClientListMapNotFound) 
{
    // Set fbWMCtx to a valid pointer but do not add the resource to the map
    p_fireboltWindowManager->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
    .WillOnce([this]() {
        // Clear the client list map before returning p_fireboltWindowManager
        p_fireboltWindowManager->mClientListMap.clear();
        return p_fireboltWindowManager; // Return the valid pointer after clearing
    });
        
    destroyResource(mockResource);
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

TEST_F(FireboltWmTest, ResourceDestroy_ClientInfoNullAndResourceMisMatch) 
{
    // Create a clientInfo instance
    FireboltWmClientInfo clientInfo;

    // Set the compositor and fbWMCtx to valid pointers
    p_fireboltWindowManager->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    // Set clientInfo's resource to a different value than mockResource
    wl_resource *differentResource = reinterpret_cast<wl_resource*>(0x1234); // or any valid resource
    clientInfo.resource = differentResource; // Assign a different resource

    // Add clientInfo to the map with mockResource
    p_fireboltWindowManager->mClientListMap[mockResource] = &clientInfo;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
        .WillRepeatedly(testing::Return(nullptr));

    EXPECT_CALL(*p_waylandImplMock, wl_resource_set_user_data(testing::_, testing::_))
        .Times(0);

    destroyResource(mockResource);

    // Verify that clientInfo was not removed from the map
    ASSERT_NE(p_fireboltWindowManager->mClientListMap.find(mockResource), p_fireboltWindowManager->mClientListMap.end());
}

/**
 * @brief Test getFireboltWmClientInfo with a valid resource.
 * Verifies that getFireboltWmClientInfo returns the correct FireboltWmClientInfo object
 * when a valid wl_resource  provided.
 *
 * @return None.
 */
TEST_F(FireboltWmClientInfoTest, GetFireboltWmClientInfo_Success)
{
    // Set up the resource and client info
    wl_resource resource;
    FireboltWmClientInfo clientInfo;

    ASSERT_NE(&resource, nullptr);
    
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltWindowManager)); 

    // Initialize the mClientListMap with mock data
    p_fireboltWindowManager->mClientListMap[&resource] = &clientInfo;

    FireboltWmClientInfo* result = p_fireboltWindowManager->getFireboltWmClientInfo(&resource);

    ASSERT_EQ(result, &clientInfo);
}



/**
 * @brief Test getFireboltWmClientInfo with a null resource.
 * Verifies wl_resource_get_user_data is never called when the provided wl_resource is null.
 *
 * @return None.
 */
TEST_F(FireboltWmClientInfoTest, GetFireboltWmClientInfo_NullResource) 
{
    wl_resource resource;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .Times(0);
                
    FireboltWmClientInfo* result = p_fireboltWindowManager->getFireboltWmClientInfo(nullptr);

    ASSERT_EQ(result, nullptr);
}

/**
 * @brief Test getFireboltWmClientInfo when wl_resource_get_user_data returns null.
 * Checks if getFireboltWmClientInfo returns nullptr when wl_resource_get_user_data returns null.
 *
 * @return None.
 */
TEST_F(FireboltWmClientInfoTest, GetFireboltWmClientInfo_NullUserData) 
{
    wl_resource resource;

    // Mock wl_resource_get_user_data to return nullptr
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(nullptr));

    FireboltWmClientInfo* result = p_fireboltWindowManager->getFireboltWmClientInfo(&resource);

    ASSERT_EQ(result, nullptr);
}

/**
 * @brief Test getFireboltWmClientInfo when resource is not found in mClientListMap.
 * Verifies that getFireboltWmClientInfo returns nullptr when the resource is not found
 * in the mClientListMap of FireboltWindowManager.
 *
 * @return None.
 */
TEST_F(FireboltWmClientInfoTest, GetFireboltWmClientInfo_ResourceNotFound) 
{
    wl_resource resource;

    // Mock wl_resource_get_user_data to return the FireboltWindowManager context
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltWindowManager));

    //  Not added the resource to mClientListMap, simulating a not found case
    // (mClientListMap[&resource] is not initialized)

    FireboltWmClientInfo* result = p_fireboltWindowManager->getFireboltWmClientInfo(&resource);

    ASSERT_EQ(result, nullptr);
}
