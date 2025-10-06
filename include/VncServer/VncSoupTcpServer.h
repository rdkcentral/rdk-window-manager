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

#include "IVncSoupSubServer.h"

#include <glib.h>
#include <gio/gio.h>

#include <map>
#include <memory>
#include <functional>


class VncClient;

class IScreenCapture;

namespace AIPlatform { class ISystemInfo; }


class VncSoupTcpServer final : public IVncSoupSubServer
{
public:
    explicit VncSoupTcpServer(int port = RDK_WINDOW_MANAGER_VNC_SERVER_PORT);
    ~VncSoupTcpServer() final;

public:
    bool start() override;
    void stop() override;
    State state() const override;

private:
    static void onConnection(GSocketService *service, GSocketConnection *conn,
                             GObject *sourceObject, gpointer userData);
    static std::string getRemoteAddress(GSocketConnection *conn);
    void moveToState(State state);
    void onClientTerminated();

private:
    const guint mPort;
    State mState;
    GSocketService *mServer;
    GCancellable  *mCanceller;

    std::list<std::shared_ptr<VncClient>> mClients;
};
