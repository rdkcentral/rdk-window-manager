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

#include <glib.h>

#include "logger.h"
#include "VncSoupTcpServer.h"
#include "VncServer.h"
#include "secure_wrapper.h"

#define IPTABLE_INPUT_APPLY_RULE    "iptables -I INPUT -p tcp -m tcp --dport 5900 -m conntrack --ctstate NEW,ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"
#define IPTABLE_OUTPUT_APPLY_RULE   "iptables -I OUTPUT -p tcp -m tcp --sport 5900 -m conntrack --ctstate ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"
#define IPTABLE_INPUT_DELETE_RULE   "iptables -D INPUT -p tcp -m tcp --dport 5900 -m conntrack --ctstate NEW,ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"
#define IPTABLE_OUTPUT_DELETE_RULE  "iptables -D OUTPUT -p tcp -m tcp --sport 5900 -m conntrack --ctstate ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"

#define VNCSERVER_PORT                          RDK_WINDOW_MANAGER_VNC_SERVER_PORT
#define VNCSERVER_FRIENDLYNAME                  "Friendly name"
#define VNCSERVER_DEFAULT_WAIT_TIME_MS          200 //200ms
#define VNCSERVER_MAX_SOUPTCPSERVER_WAIT_LOOP   10  // 10 * 200ms = 2s
#define VNCSERVER_MAX_SOUPSUBSERVER_WAIT_LOOP   3   // 3 * 200ms = 600mss
#define VNCSERVER_MAX_CLEANUP_TIME_MS           200 //200ms

std::mutex mVNCServerContextLock;

namespace RdkWindowManager
{
    VncServer::VncServer()
          : mPort(0),
            mWidth(0),
            mHeight(0),
            mIsRunning(false),
            mFrameBufferUpdateInProgress(false),
            mVncSoupTcpServer(nullptr),
            mGMainLoop(nullptr),
            mReadyToSendFrameBufer(false),
            mPixelFormat(VncClient::ClientCaptureFormat::InvalidFormat)
    {
        Logger::log(LogLevel::Information, "In VncServer constructor %s", __func__);
    }

    VncServer& VncServer::getInstance()
    {
        static VncServer instance;
        return instance;
    }

    bool VncServer::start(uint32_t width, uint32_t height)
    {
        bool status = false;
        std::lock_guard<std::mutex> contextLock(mVNCServerContextLock);

        if(mIsRunning)
        {
            Logger::log(LogLevel::Error, "%s: VncServer is already started", __func__);
            return status;
        }

        if (!applyIptableRule())
        {
            Logger::log(LogLevel::Error, "%s: VncServer applyIptableRule Failed", __func__);
            return status;
        }

        mWidth = width;
        mHeight = height;
        mPort = VNCSERVER_PORT;
        mGMainLoop = g_main_loop_new(nullptr, FALSE);

        mVncSoupTcpServer = new VncSoupTcpServer(mPort);
        status = mVncSoupTcpServer->start();
        if(status)
        {
            mIsRunning = true;
            Logger::log(LogLevel::Information, "%s: VncSoupTcpServer started", __func__);
            mGMainLoopThread = std::thread(mainLoopThread, mGMainLoop);
        }
        else
        {
            Logger::log(LogLevel::Error, "%s: VncSoupTcpServer start failed", __func__);
        }

        return status;
    }

    void VncServer::stop()
    {
        uint8_t waitCounter = 0;

        Logger::log(LogLevel::Information, "In stop %s", __func__);
        if(!mIsRunning)
        {
            Logger::log(LogLevel::Error, "%s: VncServer is already in stop state", __func__);
            return;
        }
        // If we are sending framebuffer to VNC Client, wait for the g_cancellable_cancel to cancel the operation
        while((mFrameBufferUpdateInProgress) && (waitCounter < VNCSERVER_MAX_SOUPTCPSERVER_WAIT_LOOP))
        {
            Logger::log(LogLevel::Information, "%s:: Check VncSocket state %d waitCounter :%d mFrameBufferUpdateInProgress:%d", __func__,
                    mVncSocket->state(),
                    waitCounter,
                    mFrameBufferUpdateInProgress.load());
            std::this_thread::sleep_for(std::chrono::milliseconds(VNCSERVER_DEFAULT_WAIT_TIME_MS));
            waitCounter++;
        }

        mIsRunning = false;
        mReadyToSendFrameBufer = false;
        mFrameBufferUpdateInProgress = false;
        mPixelFormat = VncClient::ClientCaptureFormat::InvalidFormat;

        mVncSoupTcpServer->stop();

        waitCounter = 0;
        while((mVncSoupTcpServer->state() != IVncSoupSubServer::State::Stopped) && (waitCounter < VNCSERVER_MAX_SOUPSUBSERVER_WAIT_LOOP))
        {
            Logger::log(LogLevel::Information, "%s:: Check VncSoupTcpServer state %d waitCounter :%d", __func__, mVncSoupTcpServer->state(), waitCounter);
            std::this_thread::sleep_for(std::chrono::milliseconds(VNCSERVER_DEFAULT_WAIT_TIME_MS));
            waitCounter++;
        }

        g_main_loop_quit(mGMainLoop);
        if (mGMainLoopThread.joinable())
        {
            mGMainLoopThread.join();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(VNCSERVER_MAX_CLEANUP_TIME_MS)); // Give some time to clean GLib
        g_main_loop_unref(mGMainLoop);

        deleteIptableRule();

        delete mVncSoupTcpServer;
    }

    VncServer::~VncServer()
    {
        Logger::log(LogLevel::Information, "In destructor %s", __func__);
        stop();
    }

    bool VncServer::applyIptableRule()
    {
        bool status = false;
        int systemStatus = 0;

        systemStatus = v_secure_system(IPTABLE_INPUT_APPLY_RULE);
        if(systemStatus == 0)
        {
            Logger::log(LogLevel::Information, "%s INPUT chain Rule applied", __func__);
            systemStatus = v_secure_system(IPTABLE_OUTPUT_APPLY_RULE);
            if(systemStatus == 0)
            {
                Logger::log(LogLevel::Information, "%s OUTPUT chain Rule applied", __func__);
                status = true;
            }
        }
        return status;
    }

    bool VncServer::deleteIptableRule()
    {
        bool status = false;
        int systemStatus = 0;

        systemStatus = v_secure_system(IPTABLE_INPUT_DELETE_RULE);
        if(systemStatus == 0)
        {
            Logger::log(LogLevel::Information, "%s INPUT chain Rule deleted", __func__);
            systemStatus = v_secure_system(IPTABLE_OUTPUT_DELETE_RULE);
            if(systemStatus == 0)
            {
                Logger::log(LogLevel::Information, "%s OUTPUT chain Rule deleted", __func__);
                status = true;
            }
        }
        return status;
    }

    uint32_t VncServer::getFrameBufferWidth()
    {
        return mWidth;
    }

    uint32_t VncServer::getFrameBufferHeight()
    {
        return mHeight;
    }
    std::string VncServer::getFriendlyName()
    {
        return VNCSERVER_FRIENDLYNAME;
    }

    void VncServer::mainLoopThread(GMainLoop* loop)
    {
        Logger::log(LogLevel::Information, "%s: VncServer Starting GLib main loop in thread", __func__);
        g_main_loop_run(loop);
        Logger::log(LogLevel::Information, "%s: VncServer Starting GLib main loop exited", __func__);
    }

    std::shared_ptr<IVncSocket> VncServer::getVncSocket()
    {
        return mVncSocket;
    }

    void VncServer::setVncSocket(const std::shared_ptr<IVncSocket>&   vncSocket)
    {
        std::lock_guard<std::mutex> contextLock(mVNCServerContextLock);
        if (vncSocket != mVncSocket)
        {
            mVncSocket = vncSocket;
        }
    }

    void VncServer::setVncFrameUpdateRequestFlag(bool flag)
    {
        std::lock_guard<std::mutex> contextLock(mVNCServerContextLock);
        if(mReadyToSendFrameBufer != flag)
        {
            mReadyToSendFrameBufer = flag;
            Logger::log(LogLevel::Information, " %s mReadyToSendFrameBufer %d", __func__, flag);
        }
    }

    bool VncServer::getVncFrameUpdateRequestFlag()
    {
        return mReadyToSendFrameBufer;
    }

    void VncServer::setVncFrameUpdatePixelFormat(VncClient::ClientCaptureFormat pixelFormat)
    {
        std::lock_guard<std::mutex> contextLock(mVNCServerContextLock);
        if (pixelFormat != mPixelFormat)
        {
            mPixelFormat = pixelFormat;
            Logger::log(LogLevel::Information, " %s pixelFormat - %d", __func__, mPixelFormat);
        }
    }

    VncClient::ClientCaptureFormat VncServer::getVncFrameUpdatePixelFormat()
    {
        return mPixelFormat;
    }

    void VncServer::setVncFrameBufferProgressState(bool sendInProgress)
    {
        std::lock_guard<std::mutex> contextLock(mVNCServerContextLock);
        if (sendInProgress != mFrameBufferUpdateInProgress)
        {
            mFrameBufferUpdateInProgress = sendInProgress;
            Logger::log(LogLevel::Information, " %s mFrameBufferUpdateInProgress - %d", __func__, mFrameBufferUpdateInProgress.load());
        }
    }

    bool VncServer::getVncFrameBufferProgressState()
    {
        return mFrameBufferUpdateInProgress;
    }


}

