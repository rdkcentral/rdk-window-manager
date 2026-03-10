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

#pragma once

#include <boost/signals2.hpp>

#include <string>
#include <functional>

class IVncSoupSubServer
{
public:
    virtual ~IVncSoupSubServer() = default;

public:
    virtual bool start() = 0;
    virtual void stop() = 0;

    enum class State : unsigned
    {
        Starting,
        Running,
        ClientConnected,
        Stopping,
        Stopped
    };

    virtual State state() const = 0;

    boost::signals2::signal<void(State newState)> stateChanged;

};

std::string toString(IVncSoupSubServer::State state);
