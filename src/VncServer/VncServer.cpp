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

#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include "logger.h"
#include "VncSoupTcpServer.h"
#include "VncServer.h"
#include "secure_wrapper.h"

#define IPTABLE_INPUT_APPLY_RULE    "iptables -I INPUT -p tcp -m tcp --dport 5900 -m conntrack --ctstate NEW,ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"
#define IPTABLE_OUTPUT_APPLY_RULE  "iptables -I OUTPUT -p tcp -m tcp --sport 5900 -m conntrack --ctstate ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"
#define IPTABLE_INPUT_DELETE_RULE   "iptables -D INPUT -p tcp -m tcp --dport 5900 -m conntrack --ctstate NEW,ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"
#define IPTABLE_OUTPUT_DELETE_RULE  "iptables -D OUTPUT -p tcp -m tcp --sport 5900 -m conntrack --ctstate ESTABLISHED -m comment --comment \"VNC (RFC6143)\" -j ACCEPT"

#define VNCSERVER_PORT          RDK_WINDOW_MANAGER_VNC_SERVER_PORT
#define VNCSERVER_FRIENDLYNAME  "Friendly name"

std::mutex mVNCServerContextLock;

namespace RdkWindowManager
{
    VncServer::VncServer()
          : mPort(0),
            mWidth(0),
            mHeight(0),
            mIsRunning(false),
            mVncSoupTcpServer(nullptr),
            mGMainLoop(nullptr)
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
            Logger::log(LogLevel::Error, "VncServer is already started %s", __func__);
            return status;
        }

        if (!applyIptableRule())
        {
            Logger::log(LogLevel::Error, "VncServer applyIptableRule Failed %s", __func__);
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
            Logger::log(LogLevel::Information, "VncSoupTcpServer started %s", __func__);
            mGMainLoopThread = std::thread(mainLoopThread, mGMainLoop);
        }
        else
        {
            Logger::log(LogLevel::Error, "VncSoupTcpServer start failed %s", __func__);
        }

        return status;
    }

    void VncServer::stop()
    {
        std::lock_guard<std::mutex> contextLock(mVNCServerContextLock);

        Logger::log(LogLevel::Information, "In stop %s", __func__);
        if(!mIsRunning)
        {
            Logger::log(LogLevel::Error, "VncServer is already in stop state %s", __func__);
            return;
        }
        mIsRunning = false;

        mVncSoupTcpServer->stop();

        g_main_loop_quit(mGMainLoop);
        if (mGMainLoopThread.joinable())
        {
            mGMainLoopThread.join();
        }
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
        // This is Temporary code, will be moved to libnftnl or libiptc
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
        // This is Temporary code, will be moved to libnftnl or libiptc

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
        Logger::log(LogLevel::Information, "VncServer Starting GLib main loop in thread %s", __func__);
        g_main_loop_run(loop);
        Logger::log(LogLevel::Information, "VncServer Starting GLib main loop exited %s", __func__);
    }

}

