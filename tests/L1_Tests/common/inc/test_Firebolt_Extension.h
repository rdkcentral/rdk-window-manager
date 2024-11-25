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

#ifndef TEST_FIREBOLT_EXTENSION
#define TEST_FIREBOLT_EXTENSION

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "firebolt_wm.h"
#include "compositorcontroller.h"
#include "compositorcontrollerImpl.h"
#include "compositorcontrollerMock.h"
#include "wayland-server-core.h"
#include "wayland-server-coreImpl.h"
#include "wayland-serverMock.h"
#include "wayland-client.h"
#include "westeros-compositor.h"
#include "westeros-compositorImpl.h"
#include "westeros-compositorMock.h"

using ::testing::NiceMock;

class FireboltExtensionTest : public ::testing::Test {
protected:
    NiceMock<RdkWindowManager::MockCompositorControllerImpl>* p_compositeImplMock = nullptr;
    NiceMock<WaylandServerMockImpl>* p_waylandImplMock = nullptr;
    NiceMock<WestCompositorMockImpl>* p_westCompositorImplMock = nullptr;
    
    wl_global* waylandMockObject = nullptr;
    struct wl_resource* mockResource = nullptr;

    void (*capturedBindFunc)(wl_client*, void*, unsigned int, unsigned int) = nullptr;
    void (*destroy)(struct wl_resource* resource) = nullptr;

public:
    FireboltExtensionTest();
    virtual ~FireboltExtensionTest() override;
};

#endif // TEST_FIREBOLT_EXTENSION
