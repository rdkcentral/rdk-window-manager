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
#include "firebolt_surface.h"
#include "test_Firebolt_Extension.h"

#define MOCK_WST_COMPOSITOR_PTR reinterpret_cast<WstCompositor*>(0x1234)
#define MOCK_WL_DISPLAY_PTR reinterpret_cast<wl_display*>(0x5678)
#define MOCK_WL_CLIENT_PTR reinterpret_cast<wl_client*>(0x9abc)
#define FIREBOLT_SURFACE_INTERFACE_VERSION 1
#define FIREBOLT_SURFACE_INTERFACE_ID 123

class FireboltSurfaceTest :public FireboltExtensionTest {
protected:
    // Used as a parameter for the capturedBindFunc function. 
    // In original function The fireboltSurfaceCreateContext function initializes the Firebolt surface context, 
    // while wl_global_create creates a global Wayland object for client interactions with the Firebolt surface.
    FireboltSurface *p_fireboltSurface = nullptr;
   
    struct TestFireboltSurfaceInterface 
    {
        void (*surface_destroy)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId);
        void (*surface_set_name)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, const char *name);
        void (*surface_set_visible)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, uint32_t visible);
        void (*surface_set_bounds)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        int32_t x, int32_t y, int32_t width, int32_t height);
        void (*surface_set_crop)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId,
                                        wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t  sheight);
        void (*surface_set_zorder)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, wl_fixed_t zorder);
        void (*surface_set_opacity)(struct wl_client *client, struct wl_resource *resource, int32_t surfaceId, wl_fixed_t opacity);
                
    
        TestFireboltSurfaceInterface& operator=(const TestFireboltSurfaceInterface& other)
        {
            surface_destroy = other.surface_destroy;
            surface_set_name = other.surface_set_name;
            surface_set_visible = other.surface_set_visible;
            surface_set_bounds = other.surface_set_bounds;
            surface_set_crop = other.surface_set_crop;
            surface_set_zorder = other.surface_set_zorder;
            surface_set_opacity = other.surface_set_opacity;
            return *this;
        }
    };

    TestFireboltSurfaceInterface testFireboltSurfaceImpl;
        
public:
    FireboltSurfaceTest();
    ~FireboltSurfaceTest() override;

    void SetUp() override;
    void TearDown() override;

    void surfaceDestroy(struct wl_resource *resource,int32_t id);
    void surfaceSetName(struct wl_resource *resource,int32_t surfaceId,const char* surfaceName);
    void surfaceSetVisible(struct wl_resource *resource,int32_t surfaceId,uint32_t visible);
    void surfaceSetZOrder(struct wl_resource *resource,int32_t surfaceId,wl_fixed_t zorder);
    void surfaceSetOpacity(struct wl_resource *resource,int32_t surfaceId,wl_fixed_t opacity);
    void surfaceSetBounds(struct wl_resource *resource,int32_t surfaceId,int32_t x, int32_t y, int32_t width, int32_t height);
    void surfaceSetCrops(struct wl_resource *resource,int32_t surfaceId,wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t sheight);
    void triggerDestroy(wl_resource* resource) ;
};


    FireboltSurfaceTest::FireboltSurfaceTest() 
    {
        p_fireboltSurface = new FireboltSurface();
        assert(nullptr != p_fireboltSurface);
    }

    FireboltSurfaceTest::~FireboltSurfaceTest()
    {
        if(p_fireboltSurface)
        delete p_fireboltSurface;
        p_fireboltSurface = nullptr;
    }


    void FireboltSurfaceTest::SetUp() 
    {
        EXPECT_CALL(*p_waylandImplMock, wl_global_create(testing::_, &firebolt_surface_interface, testing::_, testing::_, testing::_))
        .WillOnce(testing::Invoke([&](struct wl_display* display, const struct wl_interface* interface, int version, void* data, wl_global_bind_func_t bind)
        {
                        // Capturing the bind function here because the component under test expects a function pointer of 
            // type 'wl_global_bind_func_t'. Just capture the pointer without valid data in order to verify that it is passed correctly.
            capturedBindFunc = bind;
            waylandMockObject = reinterpret_cast<wl_global*>(malloc(sizeof(waylandMockObject)));
            return waylandMockObject;
        }));

        EXPECT_CALL(*p_waylandImplMock, wl_resource_set_implementation(testing::_, testing::_, testing::_, testing::_))
            .WillOnce(testing::Invoke([this](struct wl_resource* resource, const void* implementation, 
                                            void* data, wl_resource_destroy_func_t wl_ResrcDestroy) 
        {
            const TestFireboltSurfaceInterface* impl = static_cast<const TestFireboltSurfaceInterface*>(implementation);
            if (impl)
               testFireboltSurfaceImpl = *impl;
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

    void FireboltSurfaceTest::TearDown()
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


        
class FireboltSurfaceClientInfoTest : public ::testing::Test
{
    protected:
    NiceMock<WaylandServerMockImpl> *p_waylandImplMock;
    FireboltSurface *p_fireboltSurface;
    FireboltSurfaceClientInfoTest();
    ~FireboltSurfaceClientInfoTest() override;
};

    FireboltSurfaceClientInfoTest::FireboltSurfaceClientInfoTest() 
    {
        p_waylandImplMock = new NiceMock<WaylandServerMockImpl>;
        WaylandServer::setImpl(p_waylandImplMock);

        p_fireboltSurface = new FireboltSurface();
    }

    FireboltSurfaceClientInfoTest::~FireboltSurfaceClientInfoTest()
    {
        delete p_fireboltSurface;
        WaylandServer::setImpl(nullptr);
        delete p_waylandImplMock;
    }


     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `destroy` callback, which calls the `firebolt_wm_destroy` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceDestroy(struct wl_resource *resource,int32_t surfaceId) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

        ASSERT_TRUE(testFireboltSurfaceImpl.surface_destroy != nullptr) << "Error: testFireboltSurfaceImpl.surface_destroy is null";
        if (testFireboltSurfaceImpl.surface_destroy)
        {
            testFireboltSurfaceImpl.surface_destroy(MOCK_WL_CLIENT_PTR, resource, surfaceId);
        }
    }
        
        
     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `surface_set_name` callback, which calls the `firebolt_surface_set_name` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceSetName(struct wl_resource *resource,int32_t surfaceId,const char* surfaceName) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltSurfaceImpl.surface_set_name != nullptr) << "Error: testFireboltSurfaceImpl.surface_set_name is null";
        if (testFireboltSurfaceImpl.surface_set_name) {
            testFireboltSurfaceImpl.surface_set_name(MOCK_WL_CLIENT_PTR, resource, surfaceId,surfaceName);
        }
    }
        
                
     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `surface_set_visible` callback, which calls the `firebolt_surface_set_visible` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceSetVisible(struct wl_resource *resource,int32_t surfaceId,uint32_t surfaceVisible) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

            ASSERT_TRUE(testFireboltSurfaceImpl.surface_set_visible != nullptr) << "Error: testFireboltSurfaceImpl.surface_set_visible is null";
        if (testFireboltSurfaceImpl.surface_set_visible) {
            testFireboltSurfaceImpl.surface_set_visible(MOCK_WL_CLIENT_PTR, resource, surfaceId,surfaceVisible);
        }
    }


  /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `surface_set_zorder` callback, which calls the `firebolt_surface_set_zorder` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceSetZOrder(struct wl_resource *resource,int32_t surfaceId,wl_fixed_t surfaceZOrder) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

        ASSERT_TRUE(testFireboltSurfaceImpl.surface_set_zorder != nullptr) << "Error: testFireboltSurfaceImpl.surface_set_zorder is null";
        if (testFireboltSurfaceImpl.surface_set_zorder) {
            testFireboltSurfaceImpl.surface_set_zorder(MOCK_WL_CLIENT_PTR, resource, surfaceId,surfaceZOrder);
        }
    }

  /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `surface_set_opacity` callback, which calls the `firebolt_surface_set_opacity` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceSetOpacity(struct wl_resource *resource,int32_t surfaceId,wl_fixed_t surfaceOpacity) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

        ASSERT_TRUE(testFireboltSurfaceImpl.surface_set_opacity != nullptr) << "Error: testFireboltSurfaceImpl.surface_set_opacity is null";
        if (testFireboltSurfaceImpl.surface_set_opacity) {
            testFireboltSurfaceImpl.surface_set_opacity(MOCK_WL_CLIENT_PTR, resource, surfaceId,surfaceOpacity);
        }
    }
        
        
        /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `surface_set_bounds` callback, which calls the `firebolt_surface_set_bounds` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceSetBounds(struct wl_resource *resource,int32_t surfaceId,int32_t x, int32_t y, int32_t width, int32_t height) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

        ASSERT_TRUE(testFireboltSurfaceImpl.surface_set_bounds != nullptr) << "Error: testFireboltSurfaceImpl.surface_set_bounds is null";
        if (testFireboltSurfaceImpl.surface_set_bounds) {
            testFireboltSurfaceImpl.surface_set_bounds(MOCK_WL_CLIENT_PTR, resource, surfaceId,x,y,width,height);
        }
    }

     /**
     * Uses `capturedBindFunc`, a callback captured in setup, to simulate binding.
     * Then, triggers the `surface_set_crop` callback, which calls the `firebolt_surface_set_crop` function of the original implementation. 
     */
    void FireboltSurfaceTest::surfaceSetCrops(struct wl_resource *resource,int32_t surfaceId,wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t swidth, wl_fixed_t sheight) 
    {
        if (capturedBindFunc) {
            capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);
        } else {
            ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        }

        ASSERT_TRUE(testFireboltSurfaceImpl.surface_set_crop != nullptr) << "Error: testFireboltSurfaceImpl.surface_set_crop is null";
        if (testFireboltSurfaceImpl.surface_set_crop) {
            testFireboltSurfaceImpl.surface_set_crop(MOCK_WL_CLIENT_PTR, resource, surfaceId,sx,sy,swidth,sheight);
        }
    }

    /**
    * Triggers the destory callback, which calls the `firebolt_surface_resource_destory` function of the original implementation. 
    */
    void FireboltSurfaceTest::triggerDestroy(wl_resource* resource) 
    {
        ASSERT_TRUE(capturedBindFunc != nullptr) << "Error: capturedBindFunc is null.";
        capturedBindFunc(MOCK_WL_CLIENT_PTR, p_fireboltSurface, FIREBOLT_SURFACE_INTERFACE_VERSION, FIREBOLT_SURFACE_INTERFACE_ID);

        ASSERT_TRUE(destroy != nullptr) << "Error: destroy is null";
        destroy(resource);
    }


/**
 * @brief Test surfaceDestroy with a null wl_resource.
 * Verifies that no attempt is made to get user data or destroy the surface when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, surfaceDestroy_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);
    surfaceDestroy(nullptr, testSurfaceId);

}

/**
 * @brief Test surfaceDestroy with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface destruction occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, surfaceDestroy_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);
    surfaceDestroy(&resource, testSurfaceId);

}

/**
 * @brief Test surfaceDestroy with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface destruction is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, surfaceDestroy_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);

    surfaceDestroy(&resource, testSurfaceId);

}

/**
 * @brief Test surfaceDestroy when client list is not found.
 * Verifies that if the client list map is empty, no surface destruction occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, surfaceDestroy_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

     EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);

     surfaceDestroy(&resource, testSurfaceId);

}

/**
 * @brief Test surfaceDestroy with null client information.
 * Verifies that no surface destruction occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, surfaceDestroy_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);

    surfaceDestroy(&resource, testSurfaceId);

}

/**
 * @brief Test surfaceDestroy when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface destruction occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceDestroy_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);

    surfaceDestroy(&resource,testSurfaceId);
}

/**
 * @brief Test surfaceDestroy with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface destruction occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceDestroy_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful destruction of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);

    surfaceDestroy(&resource,testSurfaceId);
}

 /**
 * @brief Test surfaceDestroy with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface destruction occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceDestroy_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(testing::_, testing::_))
        .Times(0);

    surfaceDestroy(&resource,testSurfaceId);
}

 /**
 * @brief Test surfaceDestroy with successful surface destruction.
 * Verifies that the surface is successfully destroyed when the client name is valid and surface destruction succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceDestroy_fireboltSurfaceDestroySuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful destruction of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(clientName, testSurfaceId))
        .Times(1)
        .WillOnce(testing::Return(true));

    surfaceDestroy(&resource,testSurfaceId);
}

/**
 * @brief Test surfaceDestroy when surface destruction fails.
 * Verifies that the surface destruction attempt is made but fails when the client name is valid, and fireboltSurfaceDestroy returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceDestroy_fireboltSurfaceDestroyFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful destruction of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, fireboltSurfaceDestroy(clientName, testSurfaceId))
        .Times(1)
        .WillOnce(testing::Return(false));

    surfaceDestroy(&resource,testSurfaceId);
}


/**
 * @brief Test surfaceSetName with a null wl_resource.
 * Verifies that no attempt is made to set surface name when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
    const char * testSurfaceName ="surface_name";
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetName(nullptr, testSurfaceId,testSurfaceName);

}

/**
 * @brief Test SurfaceSetName with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface set name occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    const char * testSurfaceName ="surface_name";
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetName(&resource, testSurfaceId,testSurfaceName);

}

/**
 * @brief Test surfaceSetName with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface set name is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    const char * testSurfaceName ="surface_name";
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetName(&resource, testSurfaceId,testSurfaceName);

}

/**
 * @brief Test surfaceSetName when client list is not found.
 * Verifies that if the client list map is empty, no surface set name occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
     const char * testSurfaceName ="surface_name";
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);

     surfaceSetName(&resource, testSurfaceId,testSurfaceName);

}

/**
 * @brief Test surfaceSetName with null client information.
 * Verifies that no surface set name occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
    const char * testSurfaceName ="surface_name";
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetName(&resource, testSurfaceId,testSurfaceName);

}

/**
 * @brief Test surfaceSetName when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface set name occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    const char * testSurfaceName ="surface_name";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetName(&resource,testSurfaceId,testSurfaceName);
}

/**
 * @brief Test surfaceSetName with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface set name occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
    const char * testSurfaceName ="surface_name";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful set name of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetName(&resource,testSurfaceId,testSurfaceName);
}

 /**
 * @brief Test surfaceSetName with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface set name occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
    const char * testSurfaceName ="surface_name";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetName(&resource,testSurfaceId,testSurfaceName);
}

 /**
 * @brief Test surfaceSetName with successful surface set name.
 * Verifies that  when the client name is valid then surface set name succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetName_setFireboltSurfaceNameSuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
    const char * testSurfaceName ="surface_name";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set name of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(clientName, testSurfaceId,testSurfaceName))
        .Times(1)
        .WillOnce(testing::Return(true));


    surfaceSetName(&resource,testSurfaceId,testSurfaceName);
}

/**
 * @brief Test surfaceSetName when surface set name fails.
 * Verifies that the surface set name attempt is made but fails when the client name is valid, and setFireboltSurfaceName returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceSetName_setFireboltSurfaceNameFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
    const char * testSurfaceName ="surface_name";
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set name of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceName(clientName, testSurfaceId,testSurfaceName))
        .Times(1)
        .WillOnce(testing::Return(false));

    surfaceSetName(&resource,testSurfaceId,testSurfaceName);
}

/**
 * @brief Test surfaceSetVisible with a null wl_resource.
 * Verifies that no attempt is made to  set visible the surface when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
    uint32_t isVisible = 1;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetVisible(nullptr, testSurfaceId,isVisible);

}

/**
 * @brief Test SurfaceSetVisible with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface set visible occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        uint32_t isVisible = 1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetVisible(&resource, testSurfaceId,isVisible);

}

/**
 * @brief Test surfaceSetVisible with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface set visible is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        uint32_t isVisible = 1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

  EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetVisible(&resource, testSurfaceId,isVisible);

}

/**
 * @brief Test surfaceSetVisible when client list is not found.
 * Verifies that if the client list map is empty, no surface set visible occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
         uint32_t isVisible = 1;
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);

     surfaceSetVisible(&resource, testSurfaceId,isVisible);

}

/**
 * @brief Test surfaceSetVisible with null client information.
 * Verifies that no surface set visible occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
        uint32_t isVisible = 1;
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetVisible(&resource, testSurfaceId,isVisible);

}

/**
 * @brief Test surfaceSetVisible when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface set visible occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
        uint32_t isVisible = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetVisible(&resource,testSurfaceId,isVisible);
}

/**
 * @brief Test surfaceSetVisible with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface set visible occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
        uint32_t isVisible = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful set visible of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetVisible(&resource,testSurfaceId,isVisible);
}

 /**
 * @brief Test surfaceSetVisible with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface set visible occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
        uint32_t isVisible = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetVisible(&resource,testSurfaceId,isVisible);
}

 /**
 * @brief Test surfaceSetVisible with successful surface set visible.
 * Verifies that the surface is successfully destroyed when the client name is valid and surface set visible succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_setFireboltSurfaceVisibilitySuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        uint32_t isVisible = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set visible of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(clientName, testSurfaceId,isVisible))
        .Times(1)
        .WillOnce(testing::Return(true));

    surfaceSetVisible(&resource,testSurfaceId,isVisible);
}

/**
 * @brief Test surfaceSetVisible when surface set visible fails.
 * Verifies that the surface set visible attempt is made but fails when the client name is valid, and setFireboltSurfaceName returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceSetVisible_setFireboltSurfaceVisibilityFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        uint32_t isVisible = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set visible of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceVisibility(clientName, testSurfaceId,isVisible))
        .Times(1)
        .WillOnce(testing::Return(false));

    surfaceSetVisible(&resource,testSurfaceId,isVisible);
}


/**
 * @brief Test surfaceSetZOrder with a null wl_resource.
 * Verifies that no attempt is made to  set zorder the surface when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
        uint32_t isVisible = 1;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetZOrder(nullptr, testSurfaceId,isVisible);

}

/**
 * @brief Test SurfaceSetZorder with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface set zorder occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        uint32_t zorder = 1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetZOrder(&resource, testSurfaceId,zorder);

}

/**
 * @brief Test surfaceSetZOrder with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface set zorder is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        uint32_t zorder = 1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

  EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetZOrder(&resource, testSurfaceId,zorder);

}

/**
 * @brief Test surfaceSetZOrder when client list is not found.
 * Verifies that if the client list map is empty, no surface set zorder occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
         uint32_t zorder = 1;
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);

     surfaceSetZOrder(&resource, testSurfaceId,zorder);

}

/**
 * @brief Test surfaceSetZOrder with null client information.
 * Verifies that no surface set zorder occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
        uint32_t zorder = 1;
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetZOrder(&resource, testSurfaceId, zorder );

}

/**
 * @brief Test surfaceSetZOrder when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface set zorder occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
        uint32_t  zorder  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetZOrder(&resource,testSurfaceId, zorder );
}

/**
 * @brief Test surfaceSetZOrder with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface set zorder occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
        uint32_t  zorder  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful set zorder of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetZOrder(&resource,testSurfaceId, zorder );
}

 /**
 * @brief Test surfaceSetZOrder with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface set zorder occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
        uint32_t  zorder  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetZOrder(&resource,testSurfaceId, zorder );
}

 /**
 * @brief Test surfaceSetZOrder with successful surface set visible.
 * Verifies that  when the client name is valid then surface set zorder succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_setFireboltSurfaceZOrderSuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        uint32_t  zorder  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set zorder of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(clientName, testSurfaceId, wl_fixed_to_int(zorder) ))
        .Times(1)
        .WillOnce(testing::Return(true));

    surfaceSetZOrder(&resource,testSurfaceId, zorder );
}

/**
 * @brief Test surfaceSetZOrder when surface set zorder fails.
 * Verifies that the surface set zorder attempt is made but fails when the client name is valid, and setFireboltSurfaceName returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceSetZOrder_setFireboltSurfaceZOrderFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        uint32_t  zorder  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set zorder of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceZorder(clientName, testSurfaceId,wl_fixed_to_int(zorder) ))
        .Times(1)
        .WillOnce(testing::Return(false));

    surfaceSetZOrder(&resource,testSurfaceId, zorder );
}

/**
 * @brief Test surfaceSetOpacity with a null wl_resource.
 * Verifies that no attempt is made to  set opacity the surface when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
        uint32_t isVisible = 1;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetOpacity(nullptr, testSurfaceId,isVisible);

}

/**
 * @brief Test SurfaceSetOpacity with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface set opacity occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        uint32_t opacity = 1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetOpacity(&resource, testSurfaceId,opacity);

}

/**
 * @brief Test surfaceSetOpacity with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface set opacity is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        uint32_t opacity = 1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

  EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetOpacity(&resource, testSurfaceId,opacity);

}

/**
 * @brief Test surfaceSetOpacity when client list is not found.
 * Verifies that if the client list map is empty, no surface set opacity occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
         uint32_t opacity = 1;
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);

     surfaceSetOpacity(&resource, testSurfaceId,opacity);

}

/**
 * @brief Test surfaceSetOpacity with null client information.
 * Verifies that no surface set opacity occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
        uint32_t opacity = 1;
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetOpacity(&resource, testSurfaceId, opacity );

}

/**
 * @brief Test surfaceSetOpacity when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface set opacity occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
        uint32_t  opacity  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetOpacity(&resource,testSurfaceId, opacity );
}

/**
 * @brief Test surfaceSetOpacity with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface set opacity occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
        uint32_t  opacity  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful set opacity of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetOpacity(&resource,testSurfaceId, opacity );
}

 /**
 * @brief Test surfaceSetOpacity with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface set opacity occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
        uint32_t  opacity  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetOpacity(&resource,testSurfaceId, opacity );
}

 /**
 * @brief Test surfaceSetOpacity with successful surface set visible.
 * Verifies that the surface is successfully destroyed when the client name is valid and surface set opacity succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_setFireboltSurfaceOpacitySuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        double opacity  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set opacity of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(clientName, testSurfaceId, wl_fixed_to_double(opacity) ))
        .Times(1)
        .WillOnce(testing::Return(true));

    surfaceSetOpacity(&resource,testSurfaceId, opacity );
}

/**
 * @brief Test surfaceSetOpacity when surface set opacity fails.
 * Verifies that the surface set opacity attempt is made but fails when the client name is valid, and setFireboltSurfaceName returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceSetOpacity_setFireboltSurfaceOpacityFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        double opacity  = 1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set opacity of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceOpacity(clientName, testSurfaceId, wl_fixed_to_double(opacity) ))
        .Times(1)
        .WillOnce(testing::Return(false));

    surfaceSetOpacity(&resource,testSurfaceId, opacity );
}



/**
 * @brief Test surfaceSetBounds with a null wl_resource.
 * Verifies that no attempt is made to  set bounds the surface when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
        int32_t x=1,y=1,width=1,height=1;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetBounds(nullptr, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test SurfaceSetBounds with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        int32_t x=1,y=1,width=1,height=1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetBounds with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface set bounds is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    int32_t x=1,y=1,width=1,height=1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

  EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetBounds when client list is not found.
 * Verifies that if the client list map is empty, no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
         int32_t x=1,y=1,width=1,height=1;
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

     surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetBounds with null client information.
 * Verifies that no surface set bounds occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
        int32_t x=1,y=1,width=1,height=1;
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

   surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetBounds when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
        int32_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);
}

/**
 * @brief Test surfaceSetBounds with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
        int32_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful set bounds of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);
}

 /**
 * @brief Test surfaceSetBounds with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
        int32_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);
}

 /**
 * @brief Test surfaceSetBounds with successful surface set visible.
 * Verifies that the surface is successfully destroyed when the client name is valid and surface set bounds succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_setFireboltSurfaceBoundsSuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        int32_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set bounds of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(clientName, testSurfaceId, x,y,width,height))
        .Times(1)
        .WillOnce(testing::Return(true));

    surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);
}

/**
 * @brief Test surfaceSetBounds when surface set bounds fails.
 * Verifies that the surface set bounds attempt is made but fails when the client name is valid, and setFireboltSurfaceName returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceSetBounds_setFireboltSurfaceBoundsFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "test_client";
        int32_t x=1,y=1,width=1,height=1;                        
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set bounds of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceBounds(clientName, testSurfaceId,x,y,width,height ))
        .Times(1)
        .WillOnce(testing::Return(false));

   surfaceSetBounds(&resource, testSurfaceId,x,y,width,height);
}


/**
 * @brief Test surfaceSetCrops with a null wl_resource.
 * Verifies that no attempt is made to  set bounds the surface when a null resource is passed.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_InvalidParamResourceNotFound) 
{
    int32_t testSurfaceId = 123;
        wl_fixed_t x=1,y=1,width=1,height=1;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .Times(0);
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetCrops(nullptr, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test SurfaceSetBounds with no surface context found.
 * Verifies that if wl_resource_get_user_data returns null, no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_fbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
        wl_fixed_t x=1,y=1,width=1,height=1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);
    surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetCrops with getFireboltSurfaceClientName returning surface context not found.
 * Verifies that if the getFireboltSurfaceClientName Retunrs nullPtr on callingwl_resource_get_user_data  , the surface set bounds is skipped.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_getFireboltSurfaceClientNameRetunrsfbSurfaceCtxNoFound) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    wl_fixed_t x=1,y=1,width=1,height=1;
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface))
        .WillOnce(testing::Return(nullptr)); 

  EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetCrops when client list is not found.
 * Verifies that if the client list map is empty, no surface set bounds occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_getFireboltSurfaceClientNameRetunrsClientListMapNotfound) 
{
     int32_t testSurfaceId = 123;
     wl_resource resource;
         wl_fixed_t x=1,y=1,width=1,height=1;
                
     // Set the compositor to a valid mock object pointer
     p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
     //simulate clientListMap empty
     p_fireboltSurface->mClientListMap.clear();
        
     //call one time as part of bind
     EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
     EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

     surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetCrops with null client information.
 * Verifies that no surface set bounds occurs if the client information is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_getFireboltSurfaceClientNameRetunrsClientInfoNull) 
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo* clientInfo = nullptr;
        wl_fixed_t x=1,y=1,width=1,height=1;
                
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = clientInfo;
        
    //call one time as part of bind
    EXPECT_CALL(*p_compositeImplMock, getClientName(testing::_, testing::_))
          .Times(1);
   
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

   surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);

}

/**
 * @brief Test surfaceSetCrops when the client name is empty.
 * Verifies that if the client name in the ClientInfo is empty, no surface set crop occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_ClientNameEmptyInClientInfo)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
        wl_fixed_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
    
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);
}

/**
 * @brief Test surfaceSetCrops with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns false, 
 * no surface set crop occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_ClientNameEmptyInClientInfoAndFailedGetClientName)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string  clientName= "";
        wl_fixed_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(false)));

    // Mock the successful set crop of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);
}

 /**
 * @brief Test surfaceSetCrops with an empty client name in clientInfo and failed getClientName.
 * Verifies that if no client name given in the clientInfo and then getClientName returns empty clientname, 
 * no surface set crop occurs.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_ClientNameEmptyInClientInfoAndGetClientNameReturnEmpty)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    //empty clientName
    std::string  clientName= "";
        wl_fixed_t x=1,y=1,width=1,height=1;
        
    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
        
    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;
    
    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));
                
    
    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true))) 
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(testing::_, testing::_,testing::_,testing::_, testing::_,testing::_))
        .Times(0);

    surfaceSetCrops(&resource, testSurfaceId,x,y,width,height);
}

 /**
 * @brief Test surfaceSetCrops with successful surface set visible.
 * Verifies that the surface is successfully destroyed when the client name is valid and surface set crop succeeds.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_setFireboltSurfaceCropSuccess)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string clientName = "test_client";
    
    wl_fixed_t x = wl_fixed_from_int(256); 
    wl_fixed_t y = wl_fixed_from_int(512); 
    wl_fixed_t width = wl_fixed_from_int(1024);
    wl_fixed_t height = wl_fixed_from_int(2048);

    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;

    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set crop of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(clientName, testSurfaceId, 1, 2, 4, 8))
        .Times(1)
        .WillOnce(testing::Return(true));


    surfaceSetCrops(&resource, testSurfaceId, wl_fixed_to_int(x), wl_fixed_to_int(y), wl_fixed_to_int(width), wl_fixed_to_int(height));
}

/**
 * @brief Test surfaceSetCrops when surface set crop fails.
 * Verifies that the surface set crop attempt is made but fails when the client name is valid, and setFireboltSurfaceName returns false.
 *
 * @return None.
 */  
TEST_F(FireboltSurfaceTest, SurfaceSetCrops_setFireboltSurfaceCropFailed)
{
    int32_t testSurfaceId = 123;
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;
    std::string clientName = "test_client";
    
    wl_fixed_t x = wl_fixed_from_int(256); 
    wl_fixed_t y = wl_fixed_from_int(512); 
    wl_fixed_t width = wl_fixed_from_int(1024);
    wl_fixed_t height = wl_fixed_from_int(2048);

    // Set the compositor to a valid mock object pointer
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;

    // Mock the wl_resource_get_user_data to return a valid FireboltSurface instance
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(testing::_))
        .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_compositeImplMock, getClientName(p_fireboltSurface->mWstCompositor, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(clientName), testing::Return(true)));

    // Mock the successful set crop of the surface by CompositorController
    EXPECT_CALL(*p_compositeImplMock, setFireboltSurfaceCrop(clientName, testSurfaceId, 1, 2, 4, 8))
        .Times(1)
        .WillOnce(testing::Return(false));


    surfaceSetCrops(&resource, testSurfaceId, wl_fixed_to_int(x), wl_fixed_to_int(y), wl_fixed_to_int(width), wl_fixed_to_int(height));
}



/**
 * @brief Tests the resource destruction behavior when the FireboltSurface context is valid and the client is found.
 * 
 * This test verifies that when the FireboltSurface context is valid and the associated client info is found,
 * the resource is properly destroyed, and the client info is removed from the client list map.
 *
 * @return None.
 */

TEST_F(FireboltSurfaceTest, ResourceDestroySuccess_FbSurfaceCtxNotNull_ClientFound) 
{
        
        FireboltSurfaceClientInfo clientInfo;
    // Set the compositor and fbSurfaceCtx to valid pointers
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;
    p_fireboltSurface->mClientListMap[mockResource] = &clientInfo;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
      .WillRepeatedly(testing::Return(p_fireboltSurface));

    EXPECT_CALL(*p_waylandImplMock, wl_resource_set_user_data(mockResource, nullptr));

    triggerDestroy(mockResource);

    // Verify that clientInfo was removed from the map and deleted
    ASSERT_EQ(p_fireboltSurface->mClientListMap.find(mockResource), p_fireboltSurface->mClientListMap.end());
}

/**
 * @brief Tests the resource destruction behavior when the FireboltSurface context is null.
 * 
 * This test verifies that when the FireboltSurface context is null, the resource destruction
 * does not remove the client info from the client list map, ie; no action is taken
 * when there is no valid context.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceTest, ResourceDestroyFailed_FbSurfaceCtxNull) 
{
    // Simulate wl_resource_get_user_data returning null (fbSurfaceCtx is null)
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
      .WillRepeatedly(testing::Return(nullptr));

    triggerDestroy(mockResource);
        
        // Verify that clientInfo was not removed from the map
        ASSERT_NE(p_fireboltSurface->mClientListMap.find(mockResource), p_fireboltSurface->mClientListMap.end());

}

/**
 * @brief Tests the resource destruction behavior when the client is not found in the client list map.
 * 
 * This test verifies that when the FireboltSurface context is valid, but the associated resource is
 * not found in the client list map, the resource destruction failed without removing
 * any client info from the map.
 *
 * @return None.
 */

TEST_F(FireboltSurfaceTest, ResourceDestroy_ClientListMapNotFound) 
{
    // Set fbSurfaceCtx to a valid pointer but do not add the resource to the map
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
    .WillOnce([this]() {
        // Clear the client list map before returning p_fireboltSurface
        p_fireboltSurface->mClientListMap.clear();
        return p_fireboltSurface; // Return the valid pointer after clearing
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

TEST_F(FireboltSurfaceTest, ResourceDestroy_ClientInfoNullAndResourceMisMatch) 
{
    // Create a clientInfo instance
    FireboltSurfaceClientInfo clientInfo;

    // Set the compositor and fbSurfaceCtx to valid pointers
    p_fireboltSurface->mWstCompositor = MOCK_WST_COMPOSITOR_PTR;

    // Set clientInfo's resource to a different value than mockResource
    wl_resource *differentResource = reinterpret_cast<wl_resource*>(0x1234); // or any valid resource
    clientInfo.resource = differentResource; // Assign a different resource

    // Add clientInfo to the map with mockResource
    p_fireboltSurface->mClientListMap[mockResource] = &clientInfo;

    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(mockResource))
        .WillRepeatedly(testing::Return(nullptr));

    EXPECT_CALL(*p_waylandImplMock, wl_resource_set_user_data(testing::_, testing::_))
        .Times(0);

    triggerDestroy(mockResource);

    // Verify that clientInfo was not removed from the map
    ASSERT_NE(p_fireboltSurface->mClientListMap.find(mockResource), p_fireboltSurface->mClientListMap.end());
}



/**
 * @brief Test getFireboltSurfaceClientInfo with a valid resource.
 * Verifies that getFireboltSurfaceClientInfo returns the correct FireboltSurfaceClientInfo object
 * when a valid wl_resource  provided.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceClientInfoTest, GetFireboltSurfaceClientInfo_Success)
{
    // Set up the resource and client info
    wl_resource resource;
    FireboltSurfaceClientInfo clientInfo;

    ASSERT_NE(&resource, nullptr);
    
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface)); 

    // Initialize the mClientListMap with mock data
    p_fireboltSurface->mClientListMap[&resource] = &clientInfo;

    FireboltSurfaceClientInfo* result = p_fireboltSurface->getFireboltSurfaceClientInfo(&resource);

    ASSERT_EQ(result, &clientInfo);
}

/**
 * @brief Test getFireboltSurfaceClientInfo with a null resource.
 * Verifies wl_resource_get_user_data is never called when the provided wl_resource is null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceClientInfoTest, GetFireboltSurfaceClientInfo_NullResource) 
{
    wl_resource resource;
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .Times(0);
                
    FireboltSurfaceClientInfo* result = p_fireboltSurface->getFireboltSurfaceClientInfo(nullptr);

    ASSERT_EQ(result, nullptr);
}

/**
 * @brief Test getFireboltSurfaceClientInfo when wl_resource_get_user_data returns null.
 * Checks if getFireboltSurfaceClientInfo returns nullptr when wl_resource_get_user_data returns null.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceClientInfoTest, GetFireboltSurfaceClientInfo_NullUserData) 
{
    wl_resource resource;

    // Mock wl_resource_get_user_data to return nullptr
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(nullptr));

    FireboltSurfaceClientInfo* result = p_fireboltSurface->getFireboltSurfaceClientInfo(&resource);

    ASSERT_EQ(result, nullptr);
}

/**
 * @brief Test getFireboltSurfaceClientInfo when resource is not found in mClientListMap.
 * Verifies that getFireboltSurfaceClientInfo returns nullptr when the resource is not found
 * in the mClientListMap of FireboltWindowManager.
 *
 * @return None.
 */
TEST_F(FireboltSurfaceClientInfoTest, GetFireboltSurfaceClientInfo_ResourceNotFound) 
{
    wl_resource resource;

    // Mock wl_resource_get_user_data to return the FireboltWindowManager context
    EXPECT_CALL(*p_waylandImplMock, wl_resource_get_user_data(&resource))
        .WillOnce(testing::Return(p_fireboltSurface));

    //  Not added the resource to mClientListMap, simulating a not found case
    // (mClientListMap[&resource] is not initialized)

    FireboltSurfaceClientInfo* result = p_fireboltSurface->getFireboltSurfaceClientInfo(&resource);

    ASSERT_EQ(result, nullptr);
}
