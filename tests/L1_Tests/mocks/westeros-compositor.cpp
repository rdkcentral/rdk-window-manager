/*
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
 */
 
#include <gmock/gmock.h>
#include "westeros-compositor.h"
#include "westeros-compositorImpl.h"

WestCompositorImpl* WestCompositor::impl = nullptr;

WestCompositor::WestCompositor() {}

void WestCompositor::setImpl(WestCompositorImpl* newImpl)
{
    ASSERT_TRUE(((nullptr == impl) && (nullptr != newImpl)) || ((nullptr != impl) && (nullptr == newImpl)));
    impl = newImpl;
}

const char* WestCompositor::WstCompositorGetDisplayName( WstCompositor *wctx )
{
	assert(nullptr != impl);
	return impl-> WstCompositorGetDisplayName(wctx);
}

const char* (*WstCompositorGetDisplayName)(WstCompositor* wctx) = &WestCompositor::WstCompositorGetDisplayName;
