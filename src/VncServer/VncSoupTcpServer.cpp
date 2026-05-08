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

#include "VncSoupTcpServer.h"
#include "VncSoupTcpSocket.h"
#include "VncClient.h"

#include "logger.h"
using namespace RdkWindowManager;


std::string toString(IVncSoupSubServer::State state)
{
    static const std::map<IVncSoupSubServer::State, std::string> stateNames =
        {
            {IVncSoupSubServer::State::Starting,        "Starting"},
            {IVncSoupSubServer::State::Running,         "Running"},
            {IVncSoupSubServer::State::ClientConnected, "ClientConnected"},
            {IVncSoupSubServer::State::Stopping,        "Stopping"},
            {IVncSoupSubServer::State::Stopped,         "Stopped"},
        };

    return stateNames.at(state);
}

VncSoupTcpServer::VncSoupTcpServer(int port)
    : mPort(port)
    , mState(State::Stopped)
    , mServer(nullptr)
    , mCanceller(g_cancellable_new())
{
    Logger::log(LogLevel::Information, "Constructor of the vnc tcp server");
}

/* Destructs the object, stopping the server if not already. */
VncSoupTcpServer::~VncSoupTcpServer()
{
    try
    {
        Logger::log(LogLevel::Information, "destructing the vnc tcp server");

        // log a warning if failed to stopped before destruction
        if ((mState != State::Stopped) && (mState != State::Stopping))
        {
            Logger::log(LogLevel::Information, "destroying VncSoupTcpServer in non-Stopped state ('%s')",
                      toString(mState).c_str());

            // ensure the server is at least stopping
            stop();
        }
    }
    catch (const std::exception& e)
    {
        // Never let exceptions escape from destructor - would cause std::terminate()
        Logger::log(LogLevel::Error, "Exception in ~VncSoupTcpServer: %s", e.what());
    }
    catch (...)
    {
        // Catch all other exceptions
        Logger::log(LogLevel::Error, "Unknown exception in ~VncSoupTcpServer");
    }

    // close all client connections - we may have to do this if timed-out
    // during stopping phase
    mClients.clear();

    // free canceller
    g_object_unref(mCanceller);

    Logger::log(LogLevel::Information, "destructed the vnc tcp server");
}

/* Starts the VNC TCP server. */
bool VncSoupTcpServer::start()
{
    g_assert(mState == State::Stopped);

    // reset the canceller
    g_cancellable_reset(mCanceller);

    // start a raw TCP server listening on port 5900 for VNC / RFB protocol requests
    mServer = g_socket_service_new();
    if (!mServer)
    {
        Logger::log(LogLevel::Error, "failed to create tvp socket service");
        return false;
    }

    GError *error = nullptr;
    if (g_socket_listener_add_inet_port(G_SOCKET_LISTENER(mServer), mPort, nullptr, &error) == FALSE)
    {
        Logger::log(LogLevel::Error, "failed to start listening on port %hu - %s", mPort, error->message);
        g_error_free(error);
        g_object_unref(mServer);
        mServer = nullptr;
        return false;
    }

    g_signal_connect(mServer, "incoming", G_CALLBACK(onConnection), this);
    g_socket_service_start(mServer);
    moveToState(State::Running);
    Logger::log(LogLevel::Information,"started vnc tcp server on port %u", mPort);

    return true;
}

/*  Stops the TCP VNC server.  This is an async operation and callers should
    monitor the state to determine when the server is fully stopped and all
    the connections closed.
 */
void VncSoupTcpServer::stop()
{
    Logger::log(LogLevel::Information, "stopping vnc tcp server");

    // move to stopping state - we actually stop when all connections are closed
    moveToState(State::Stopping);

    // trigger the canceller to stop any async operations
    g_cancellable_cancel(mCanceller);

    // stop the server
    if (mServer)
    {
        g_socket_service_stop(mServer);
        g_socket_listener_close(G_SOCKET_LISTENER(mServer));
        g_object_unref(mServer);
        mServer = nullptr;
    }

    // if we don't have any connections then done
    if (mClients.empty())
    {
        moveToState(State::Stopped);
    }
    else
    {
        // otherwise close all client connections and then wait till we
        // receive the close callback
        for (const auto &client: mClients)
        {
            client->terminate();
        }
    }

    Logger::log(LogLevel::Information, "stopped vnc tcp server");
}

/* Updates the state and invokes state change callback if installed. */
void VncSoupTcpServer::moveToState(State state)
{
    if (mState != state)
    {
        Logger::log(LogLevel::Information, "vnc tcp server moving state '%s' -> '%s'",
                    toString(mState).c_str(), toString(state).c_str());
        mState = state;
        stateChanged(mState);
    }
}

/* Returns the current state of the server. */
VncSoupTcpServer::State VncSoupTcpServer::state() const
{
    return mState;
}

/* Utility to get the remote address of the socket connection as a string. */
std::string VncSoupTcpServer::getRemoteAddress(GSocketConnection *conn)
{
    std::string str;

    GSocketAddress *address = g_socket_connection_get_remote_address(conn, nullptr);
    if (address)
    {
        gchar *addressStr = g_inet_address_to_string(
            g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(address)));

        if (addressStr)
        {
            str = addressStr;
            g_free(addressStr);
        }

        g_object_unref(address);
    }

    return str;
}

/* Called when a raw TCP socket has connected on port 5900 */
void VncSoupTcpServer::onConnection(GSocketService *service,
                                    GSocketConnection *conn,
                                    GObject *sourceObject, gpointer userData)
{
    (void) service;
    (void) sourceObject;

    auto self = reinterpret_cast<VncSoupTcpServer*>(userData);
    Logger::log(LogLevel::Information, "vnc raw client connected from %s", getRemoteAddress(conn).c_str());

    // wrap the socket
    auto vncSocket = std::make_shared<VncSoupTcpSocket>(conn);

    // check we're in the running or client connected state, if not then
    // reject the connection
    if ((self->mState != State::Running) && (self->mState != State::ClientConnected))
    {
        Logger::log(LogLevel::Error, "vnc tcp server is not in running state - rejecting connection");
        vncSocket->close();
        return;
    }

    // determine the client mode
    const VncClient::ClientMode clientMode = VncClient::ClientMode::RFC6143;

    // create a new client wrapping the connection
    std::shared_ptr<VncClient> client = std::make_shared<VncClient>(clientMode, std::move(vncSocket));
    if (!client || client->isTerminated())
    {
        Logger::log(LogLevel::Error, "failed to create vnc client object - closing socket");
        return;
    }

    // hook the terminated signal from the client so can use that to free it up
    client->terminated.connect(std::bind(&VncSoupTcpServer::onClientTerminated, self));

    // store the client
    self->mClients.emplace_back(std::move(client));

    // move to the 'client connected' state
    self->moveToState(State::ClientConnected);
}

/*
    Called when one of the VNC websocket clients terminates, usually as a result
    of the websocket closing
 */
void VncSoupTcpServer::onClientTerminated()
{
    Logger::log(LogLevel::Information, "received notification VNC client connection terminated");

    g_idle_add(
        [](gpointer userData) -> gboolean
        {
            auto self = reinterpret_cast<VncSoupTcpServer*>(userData);

            // check if any clients have terminated
            auto it = self->mClients.begin();
            while (it != self->mClients.end())
            {
                if ((*it)->isTerminated())
                    it = self->mClients.erase(it);
                else
                    ++it;
            }

            // if no more clients connected then leave the 'ClientConnected' state
            if (self->mClients.empty())
            {
                switch (self->mState)
                {
                    case State::ClientConnected:
                        self->moveToState(State::Running);
                        break;
                    case State::Stopping:
                        self->moveToState(State::Stopped);
                        break;
                    default:
                        break;
                }
            }

            return G_SOURCE_REMOVE;
        }, this);
}
