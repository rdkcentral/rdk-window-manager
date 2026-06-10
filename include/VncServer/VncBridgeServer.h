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

// VncBridgeServer.h
//
// Plain-socket (no DBus) bridge server used when ENABLE_RDKWINDOWMANAGER_VNCSERVER2 is set.
// VNCServer2 (appsservice-vncd) connects to this server to get screen details, set a shared
// frame buffer and request frame / screenshot captures.
//
// The server listens on abstract Unix socket:  \0/tmp/rdkwindowmanager-vnc-bridge

#pragma once

#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>

namespace RdkWindowManager {

class VncBridgeServer
{
public:
    static VncBridgeServer& getInstance();

    // ── Lifecycle ────────────────────────────────────────────────────────────

    bool start();
    void stop();
    bool isRunning() const;

    // ── Called from the GL rendering thread (VncFrameBuffer::publish) ────────

    /// Returns true when VNCServer2 has issued a pending RequestFrameUpdate.
    bool isBridgeFrameUpdatePending() const;

    /// Deliver a freshly captured RGBA frame.  The server converts the pixels,
    /// writes them into the shared buffer the client set via SetBuffer and then
    /// wakes the waiting VNCServer2 request.
    void deliverFrame(const uint8_t* rgbaData, uint32_t width, uint32_t height);

private:
    VncBridgeServer();
    ~VncBridgeServer();
    VncBridgeServer(const VncBridgeServer&) = delete;
    VncBridgeServer& operator=(const VncBridgeServer&) = delete;

    // ── Server threads ───────────────────────────────────────────────────────

    void acceptLoop();
    void clientLoop(int clientFd);

    // ── Message handlers ─────────────────────────────────────────────────────

    void handleGetDetails(int clientFd);
    void handleSetBuffer(int clientFd, const std::vector<uint8_t>& payload, int memFd);
    void handleFrameUpdateRequest(int clientFd, const std::vector<uint8_t>& payload);
    void handleScreenshotRequest(int clientFd, const std::vector<uint8_t>& payload);
    void handleAppScreenshotRequest(int clientFd, const std::vector<uint8_t>& payload);

    // ── Low-level I/O helpers ────────────────────────────────────────────────

    static bool sendMsg(int fd, uint32_t type,
                        const void* payload, size_t payloadLen,
                        int passFd = -1);

    static bool recvMsg(int fd,
                        uint32_t& outType,
                        std::vector<uint8_t>& outPayload,
                        int& outPassedFd);

    static void sendError(int fd, uint32_t code, const std::string& msg);

    // ── State ────────────────────────────────────────────────────────────────

    int                    mListenFd;
    std::atomic<bool>      mRunning;
    std::thread            mAcceptThread;
    std::thread            mClientThread;
    std::atomic<int>       mClientFd;

    // Shared frame buffer set by the client via SetBuffer
    std::mutex             mBufferMutex;
    uint8_t*               mBridgeMemPtr;
    size_t                 mBridgeMemSize;
    int                    mBridgeMemFd;

    // Pending RequestFrameUpdate state
    std::mutex             mFrameMutex;
    std::condition_variable mFrameCv;
    std::atomic<bool>      mFrameUpdatePending;
    size_t                 mFrameOffset;
    size_t                 mFrameMaxSize;
    uint32_t               mFrameFormat;
    int64_t                mFrameResult;
    bool                   mFrameReady;
};

} // namespace RdkWindowManager
