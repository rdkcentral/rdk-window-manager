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

#include "IVncSocket.h"

#include <glib.h>
#include <gio/gio.h>

#include <deque>


class VncSoupTcpSocket final : public IVncSocket
{
public:
    VncSoupTcpSocket(GSocketConnection *conn);
    ~VncSoupTcpSocket() final;

    State state() const override;

    ssize_t read(void *buffer, size_t maxSize) override;

    GBytes *read() override;
    bool write(GBytes *data) override;

    void close() override;

private:
    static gboolean onRecvData(GPollableInputStream *inputStream,
                               gpointer userData);

    static void onSentData(GOutputStream* outputStream, GAsyncResult* result,
                           gpointer userData);

private:
    GSocketConnection *mConn;
    GSource *mRecvSource;

    GBytes *mSendInProgress;
    std::deque<GBytes*> mSendQueue;

    GCancellable *mCanceller;
};
