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

#include "VncSoupTcpSocket.h"

#include "logger.h"
using namespace RdkWindowManager;


VncSoupTcpSocket::VncSoupTcpSocket(GSocketConnection *conn)
    : mConn(G_SOCKET_CONNECTION(g_object_ref(conn)))
    , mRecvSource(nullptr)
    , mSendInProgress(nullptr)
    , mCanceller(g_cancellable_new())
{
    // get the socket object and make it non-blocking
    GSocket *socket = g_socket_connection_get_socket(mConn);

    // put the socket in non-blocking mode
    g_socket_set_blocking(socket, FALSE);

    // set up a source to listen for input events
    GInputStream *inStream = g_io_stream_get_input_stream(G_IO_STREAM(mConn));
    mRecvSource = g_pollable_input_stream_create_source(G_POLLABLE_INPUT_STREAM(inStream), nullptr);
    g_source_set_callback(mRecvSource, (GSourceFunc)onRecvData, this, nullptr);
    g_source_attach(mRecvSource, g_main_context_get_thread_default());
}

VncSoupTcpSocket::~VncSoupTcpSocket()
{
    while (!mSendQueue.empty())
    {
        g_bytes_unref(mSendQueue.front());
        mSendQueue.pop_front();
    }

    if (mCanceller)
    {
        g_cancellable_cancel(mCanceller);
        g_object_unref(mCanceller);
    }

    if (mRecvSource)
    {
        g_source_destroy(mRecvSource);
        g_source_unref(mRecvSource);
    }

    if (mConn)
    {
        g_object_unref(mConn);
    }
}

IVncSocket::State VncSoupTcpSocket::state() const
{
    if (mConn && g_socket_connection_is_connected(mConn))
        return State::Open;
    else
        return State::Closed;
}

ssize_t VncSoupTcpSocket::read(void *buffer, size_t maxSize)
{
    GInputStream *inStream = g_io_stream_get_input_stream(G_IO_STREAM(mConn));
    if (!inStream)
    {
        Logger::log(LogLevel::Error, "failed to get socket input stream");
        return -1;
    }

    GError *error = nullptr;
    gssize dataLen = g_pollable_input_stream_read_nonblocking(G_POLLABLE_INPUT_STREAM(inStream),
                                                              buffer, maxSize,
                                                              nullptr, &error);
    if (dataLen < 0)
    {
        if (error && (error->code != G_IO_ERROR_WOULD_BLOCK))
        {
            Logger::log(LogLevel::Error, "failed to read from vnc tcp socket - %s", error->message);
            close();
        }

        g_clear_error(&error);
        return -1;
    }

    if (dataLen == 0)
    {
        Logger::log(LogLevel::Error, "vnc tcp read 0 bytes - closing connection");
        close();
    }

    return dataLen;
}

/*
    Called when there is data to read on a client socket, or the client has
    disconnected.
 */
gboolean VncSoupTcpSocket::onRecvData(GPollableInputStream *inputStream,
                                      gpointer userData)
{
    (void) inputStream;

    auto self = reinterpret_cast<VncSoupTcpSocket*>(userData);

    if (g_socket_connection_is_connected(self->mConn))
    {
        Logger::log(LogLevel::Information, "received data from vnc tcp client");
        self->readReady();
    }
    else
    {
        Logger::log(LogLevel::Error, "detected close on vnc tcp socket");
        self->closed();
    }

    return TRUE;
}

GBytes *VncSoupTcpSocket::read()
{
    GInputStream *inStream = g_io_stream_get_input_stream(G_IO_STREAM(mConn));
    if (!inStream)
    {
        Logger::log(LogLevel::Error, "failed to get socket input stream");
        return nullptr;
    }

    uint8_t data[1024];
    GError *error = nullptr;
    gssize dataLen = g_pollable_input_stream_read_nonblocking(G_POLLABLE_INPUT_STREAM(inStream),
                                                              data, sizeof(data),
                                                              nullptr, &error);
    if (dataLen < 0)
    {
        if (error->code != G_IO_ERROR_WOULD_BLOCK)
        {
            Logger::log(LogLevel::Error, "failed to read from vnc tcp socket - %s",
                         error->message);
            close();
        }

        g_clear_error(&error);
        return nullptr;
    }

    if (dataLen == 0)
    {
        Logger::log(LogLevel::Error, "vnc tcp read 0 bytes - closing connection");
        close();
    }

    return g_bytes_new(data, dataLen);
}

bool VncSoupTcpSocket::write(GBytes *data)
{
    // TODO: check socket is open

    // write the data, installing a callback for when the write completes
    GOutputStream *outStream = g_io_stream_get_output_stream(G_IO_STREAM(mConn));
    if (!outStream)
    {
        return false;
    }

    // send or queue the frame
    if (mSendInProgress)
    {
        mSendQueue.push_back(data);
    }
    else
    {
        mSendInProgress = data;

        gsize size;
        const void *ptr = g_bytes_get_data(mSendInProgress, &size);

        Logger::log(LogLevel::Information, "sending %zu bytes of data to vnc tcp client", size);

        g_output_stream_write_all_async(outStream, ptr, size,
                                        G_PRIORITY_DEFAULT, mCanceller,
                                        (GAsyncReadyCallback) onSentData, this);
    }

    return true;
}

/*
    Called when the transfer of data to socket has completed.  It's at this
    point that we tell the client that the data has been consumed.
 */
void VncSoupTcpSocket::onSentData(GOutputStream* outputStream,
                                  GAsyncResult* result,
                                  gpointer userData)
{
    auto self = reinterpret_cast<VncSoupTcpSocket*>(userData);

    GBytes *sentData = self->mSendInProgress;
    self->mSendInProgress = nullptr;

    gsize written = 0;
    GError *error = nullptr;

    if ((g_output_stream_write_all_finish(outputStream, result, &written, &error) == FALSE) ||
        (written != g_bytes_get_size(sentData)))
    {
        Logger::log(LogLevel::Information, "failed to write all the bytes (%zu vs %zu) %s",
                   written, g_bytes_get_size(sentData),
                   error ? error->message : "");

        // close the socket, the close callback will handle clean-up
        self->close();
    }
    else
    {
        Logger::log(LogLevel::Information, "sent %zu bytes to vnc tcp client", written);

        // check if any more data to be queued
        if (!self->mSendQueue.empty())
        {
            self->mSendInProgress = self->mSendQueue.front();
            self->mSendQueue.pop_front();

            gsize size;
            const void *ptr = g_bytes_get_data(self->mSendInProgress, &size);

            g_output_stream_write_all_async(outputStream, ptr, size,
                                            G_PRIORITY_DEFAULT, self->mCanceller,
                                            (GAsyncReadyCallback) onSentData, self);
        }
    }

    g_bytes_unref(sentData);
    g_clear_error(&error);
}

void VncSoupTcpSocket::close()
{
    // remove everything from the send queue
    while (!mSendQueue.empty())
    {
        g_bytes_unref(mSendQueue.front());
        mSendQueue.pop_front();
    }

    // cancel any active send
    if (mCanceller)
    {
        g_cancellable_cancel(mCanceller);
    }

    // close the socket
    GSocket *socket = g_socket_connection_get_socket(mConn);
    if (!g_socket_is_closed(socket))
    {
        g_socket_close(socket, nullptr);
    }
}
