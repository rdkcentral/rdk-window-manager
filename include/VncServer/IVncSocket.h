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

#include <glib.h>

class IVncSocket
{
public:
    virtual ~IVncSocket() = default;

    enum State
    {
        Open,
        Closing,
        Closed
    };

    virtual State state() const = 0;

    virtual GBytes *read() = 0;
    virtual ssize_t read(void *buffer, size_t maxSize) = 0;
    virtual bool write(GBytes *data) = 0;

    virtual void close() = 0;

public:
    boost::signals2::signal<void(size_t)> bytesWritten;
    boost::signals2::signal<void()> readReady;
    boost::signals2::signal<void()> closed;

};

