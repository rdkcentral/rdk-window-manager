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

#include "test_Firebolt_Extension.h"

FireboltExtensionTest::FireboltExtensionTest() 
{
    p_compositeImplMock = new NiceMock<RdkWindowManager::MockCompositorControllerImpl>;
    assert(p_compositeImplMock);
    RdkWindowManager::CompositorController::setImpl(p_compositeImplMock);

    p_waylandImplMock = new NiceMock<WaylandServerMockImpl>;
    assert(p_waylandImplMock);
    WaylandServer::setImpl(p_waylandImplMock);

    p_westCompositorImplMock = new NiceMock<WestCompositorMockImpl>;
    assert(p_westCompositorImplMock);
    WestCompositor::setImpl(p_westCompositorImplMock);

    capturedBindFunc = nullptr;
}

FireboltExtensionTest::~FireboltExtensionTest() 
{
	RdkWindowManager::CompositorController::setImpl(nullptr);
    WaylandServer::setImpl(nullptr);
    WestCompositor::setImpl(nullptr);
	
    delete p_compositeImplMock;
    p_compositeImplMock = nullptr;
	
    delete p_waylandImplMock;
    p_waylandImplMock = nullptr; 
	 
    delete p_westCompositorImplMock;
    p_westCompositorImplMock = nullptr;
    
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

}
