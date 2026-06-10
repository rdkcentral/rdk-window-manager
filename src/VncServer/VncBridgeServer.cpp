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

#include "VncBridgeServer.h"
#include "VncBridgeProtocol.h"
#include "VncServer.h"
#include "VncClient.h"
#include "logger.h"
#include "MemFd.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cassert>

// Abstract Unix socket path (the leading '\0' makes it abstract)
#define BRIDGE_SOCKET_PATH  "/tmp/rdkwindowmanager-vnc-bridge"

namespace RdkWindowManager {

// ── singleton ─────────────────────────────────────────────────────────────────

VncBridgeServer& VncBridgeServer::getInstance()
{
    static VncBridgeServer instance;
    return instance;
}

VncBridgeServer::VncBridgeServer()
    : mListenFd(-1)
    , mRunning(false)
    , mClientFd(-1)
    , mBridgeMemPtr(nullptr)
    , mBridgeMemSize(0)
    , mBridgeMemFd(-1)
    , mFrameUpdatePending(false)
    , mFrameOffset(0)
    , mFrameMaxSize(0)
    , mFrameFormat(0)
    , mFrameResult(0)
    , mFrameReady(false)
{
}

VncBridgeServer::~VncBridgeServer()
{
    stop();
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

bool VncBridgeServer::start()
{
    if (mRunning.load())
    {
        Logger::log(LogLevel::Warning, "%s: VncBridgeServer already running", __func__);
        return true;
    }

    // Create abstract Unix domain socket
    mListenFd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (mListenFd < 0)
    {
        Logger::log(LogLevel::Error, "%s: socket() failed: %s", __func__, ::strerror(errno));
        return false;
    }

    struct sockaddr_un addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    // Abstract socket: sun_path[0] = '\0', rest is the name
    ::strncpy(addr.sun_path + 1, BRIDGE_SOCKET_PATH, sizeof(addr.sun_path) - 2);
    socklen_t addrLen = 1 + 1 + ::strlen(BRIDGE_SOCKET_PATH); // offsetof + '\0' + name

    if (::bind(mListenFd, reinterpret_cast<struct sockaddr*>(&addr), addrLen) < 0)
    {
        Logger::log(LogLevel::Error, "%s: bind() failed: %s", __func__, ::strerror(errno));
        ::close(mListenFd);
        mListenFd = -1;
        return false;
    }

    if (::listen(mListenFd, 1) < 0)
    {
        Logger::log(LogLevel::Error, "%s: listen() failed: %s", __func__, ::strerror(errno));
        ::close(mListenFd);
        mListenFd = -1;
        return false;
    }

    mRunning.store(true);
    mAcceptThread = std::thread(&VncBridgeServer::acceptLoop, this);
    Logger::log(LogLevel::Information, "%s: VncBridgeServer started on abstract socket %s",
                __func__, BRIDGE_SOCKET_PATH);
    return true;
}

void VncBridgeServer::stop()
{
    if (!mRunning.exchange(false))
        return;

    // Close listen fd to unblock accept()
    if (mListenFd >= 0)
    {
        ::close(mListenFd);
        mListenFd = -1;
    }

    // Close current client fd to unblock recv()
    int cfd = mClientFd.exchange(-1);
    if (cfd >= 0)
        ::close(cfd);

    if (mAcceptThread.joinable())
        mAcceptThread.join();
    if (mClientThread.joinable())
        mClientThread.join();

    // Release shared buffer
    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        if (mBridgeMemPtr != nullptr)
        {
            ::munmap(mBridgeMemPtr, mBridgeMemSize);
            mBridgeMemPtr  = nullptr;
            mBridgeMemSize = 0;
        }
        if (mBridgeMemFd >= 0)
        {
            ::close(mBridgeMemFd);
            mBridgeMemFd = -1;
        }
    }

    // Wake any thread blocked waiting for a frame
    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mFrameReady = true;
        mFrameResult = -EIO;
        mFrameUpdatePending.store(false);
    }
    mFrameCv.notify_all();

    Logger::log(LogLevel::Information, "%s: VncBridgeServer stopped", __func__);
}

bool VncBridgeServer::isRunning() const
{
    return mRunning.load();
}

// ── Accept loop ───────────────────────────────────────────────────────────────

void VncBridgeServer::acceptLoop()
{
    while (mRunning.load())
    {
        struct sockaddr_un peerAddr;
        socklen_t peerLen = sizeof(peerAddr);

        int clientFd = ::accept(mListenFd,
                                reinterpret_cast<struct sockaddr*>(&peerAddr),
                                &peerLen);
        if (clientFd < 0)
        {
            if (mRunning.load())
                Logger::log(LogLevel::Error, "%s: accept() failed: %s", __func__, ::strerror(errno));
            break;
        }

        Logger::log(LogLevel::Information, "%s: VNCServer2 client connected fd=%d", __func__, clientFd);

        // If there is already a client, close the old one
        int old = mClientFd.exchange(clientFd);
        if (old >= 0)
        {
            ::close(old);
            if (mClientThread.joinable())
                mClientThread.join();
        }

        mClientThread = std::thread(&VncBridgeServer::clientLoop, this, clientFd);
    }
}

// ── Client loop ───────────────────────────────────────────────────────────────

void VncBridgeServer::clientLoop(int clientFd)
{
    Logger::log(LogLevel::Information, "%s: client loop start fd=%d", __func__, clientFd);

    while (mRunning.load() && clientFd == mClientFd.load())
    {
        uint32_t             type      = 0;
        std::vector<uint8_t> payload;
        int                  passedFd  = -1;

        if (!recvMsg(clientFd, type, payload, passedFd))
        {
            Logger::log(LogLevel::Information, "%s: client disconnected fd=%d", __func__, clientFd);
            break;
        }

        switch (static_cast<VncBridgeProtocol::MsgType>(type))
        {
            case VncBridgeProtocol::MSG_GET_DETAILS_REQ:
                handleGetDetails(clientFd);
                break;
            case VncBridgeProtocol::MSG_SET_BUFFER_REQ:
                handleSetBuffer(clientFd, payload, passedFd);
                break;
            case VncBridgeProtocol::MSG_FRAME_UPDATE_REQ:
                handleFrameUpdateRequest(clientFd, payload);
                break;
            case VncBridgeProtocol::MSG_SCREENSHOT_REQ:
                handleScreenshotRequest(clientFd, payload);
                break;
            case VncBridgeProtocol::MSG_APP_SCREENSHOT_REQ:
                handleAppScreenshotRequest(clientFd, payload);
                break;
            default:
                Logger::log(LogLevel::Warning, "%s: unknown message type %u", __func__, type);
                sendError(clientFd, EINVAL, "Unknown message type");
                break;
        }

        if (passedFd >= 0 && type != static_cast<uint32_t>(VncBridgeProtocol::MSG_SET_BUFFER_REQ))
        {
            // Unexpected fd – close it to avoid fd leak
            ::close(passedFd);
        }
    }

    // Client gone – clean up bridge buffer ownership
    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        if (mBridgeMemPtr != nullptr)
        {
            ::munmap(mBridgeMemPtr, mBridgeMemSize);
            mBridgeMemPtr  = nullptr;
            mBridgeMemSize = 0;
        }
        if (mBridgeMemFd >= 0)
        {
            ::close(mBridgeMemFd);
            mBridgeMemFd = -1;
        }
    }

    // Unblock any pending frame waiter
    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mFrameReady  = true;
        mFrameResult = -ECONNRESET;
        mFrameUpdatePending.store(false);
    }
    mFrameCv.notify_all();

    if (mClientFd.compare_exchange_strong(clientFd, -1))
        ::close(clientFd);

    Logger::log(LogLevel::Information, "%s: client loop end fd=%d", __func__, clientFd);
}

// ── Message handlers ──────────────────────────────────────────────────────────

void VncBridgeServer::handleGetDetails(int clientFd)
{
    const std::string name       = VncServer::getInstance().getFriendlyName();
    const uint32_t    width      = VncServer::getInstance().getFrameBufferWidth();
    const uint32_t    height     = VncServer::getInstance().getFrameBufferHeight();

    // Report BGR0_8_8_8_8 (matching the default VncClient capture format)
    const uint32_t    pixFmt     = 4; // IVncBridgeConn::CaptureFormat::BGR0_8_8_8_8

    VncBridgeProtocol::GetDetailsResp resp;
    resp.width       = width;
    resp.height      = height;
    resp.pixelFormat = pixFmt;
    resp.nameLen     = static_cast<uint32_t>(name.size());

    // Build payload = struct + name bytes
    std::vector<uint8_t> payload(sizeof(resp) + name.size());
    ::memcpy(payload.data(), &resp, sizeof(resp));
    ::memcpy(payload.data() + sizeof(resp), name.data(), name.size());

    sendMsg(clientFd, VncBridgeProtocol::MSG_GET_DETAILS_RESP,
            payload.data(), payload.size());
}

void VncBridgeServer::handleSetBuffer(int clientFd,
                                      const std::vector<uint8_t>& payload,
                                      int memFd)
{
    if (payload.size() < sizeof(VncBridgeProtocol::SetBufferReq))
    {
        sendError(clientFd, EINVAL, "SetBuffer payload too short");
        if (memFd >= 0) ::close(memFd);
        return;
    }
    if (memFd < 0)
    {
        sendError(clientFd, EINVAL, "SetBuffer: no fd received");
        return;
    }

    VncBridgeProtocol::SetBufferReq req;
    ::memcpy(&req, payload.data(), sizeof(req));

    // Map the client-provided memfd into our address space
    void* ptr = ::mmap(nullptr, req.size, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
    if (ptr == MAP_FAILED)
    {
        Logger::log(LogLevel::Error, "%s: mmap failed: %s", __func__, ::strerror(errno));
        sendError(clientFd, errno, "SetBuffer: mmap failed");
        ::close(memFd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        if (mBridgeMemPtr != nullptr)
            ::munmap(mBridgeMemPtr, mBridgeMemSize);
        if (mBridgeMemFd >= 0)
            ::close(mBridgeMemFd);
        mBridgeMemPtr  = static_cast<uint8_t*>(ptr);
        mBridgeMemSize = req.size;
        mBridgeMemFd   = memFd;
    }

    VncBridgeProtocol::SetBufferResp resp;
    resp.success   = 1;
    resp._pad[0] = resp._pad[1] = resp._pad[2] = 0;
    sendMsg(clientFd, VncBridgeProtocol::MSG_SET_BUFFER_RESP, &resp, sizeof(resp));
}

void VncBridgeServer::handleFrameUpdateRequest(int clientFd,
                                               const std::vector<uint8_t>& payload)
{
    if (payload.size() < sizeof(VncBridgeProtocol::FrameUpdateReq))
    {
        sendError(clientFd, EINVAL, "FrameUpdateReq payload too short");
        return;
    }

    VncBridgeProtocol::FrameUpdateReq req;
    ::memcpy(&req, payload.data(), sizeof(req));

    // Check shared buffer is available
    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        if (mBridgeMemPtr == nullptr)
        {
            sendError(clientFd, ENOMEM, "FrameUpdateReq: no buffer set");
            return;
        }
    }

    // Arm the pending frame update – the GL thread (VncFrameBuffer::publish)
    // will see this flag and call deliverFrame()
    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mFrameOffset        = static_cast<size_t>(req.offset);
        mFrameMaxSize       = static_cast<size_t>(req.maxSize);
        mFrameFormat        = req.format;
        mFrameReady         = false;
        mFrameResult        = 0;
    }
    mFrameUpdatePending.store(true);

    // Notify the GL thread that a VNC frame update is needed
    VncServer::getInstance().setVncFrameUpdateRequestFlag(true);

    // Block until the GL thread delivers the frame
    {
        std::unique_lock<std::mutex> lock(mFrameMutex);
        mFrameCv.wait(lock, [this]{ return mFrameReady; });
    }
    mFrameUpdatePending.store(false);

    VncBridgeProtocol::FrameUpdateResp resp;
    resp.bytesWritten = mFrameResult;
    sendMsg(clientFd, VncBridgeProtocol::MSG_FRAME_UPDATE_RESP, &resp, sizeof(resp));
}

void VncBridgeServer::handleScreenshotRequest(int clientFd,
                                              const std::vector<uint8_t>& payload)
{
    if (payload.size() < sizeof(VncBridgeProtocol::ScreenshotReq))
    {
        sendError(clientFd, EINVAL, "ScreenshotReq payload too short");
        return;
    }

    // Capture the current frame into a fresh memfd and return it.
    // We reuse the bridge frame update path: arm the update, wait, then
    // dup the mapped buffer into a new memfd for the caller.
    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        if (mBridgeMemPtr == nullptr)
        {
            VncBridgeProtocol::ScreenshotResp resp;
            resp.success = 0;
            resp._pad[0] = resp._pad[1] = resp._pad[2] = 0;
            sendMsg(clientFd, VncBridgeProtocol::MSG_SCREENSHOT_RESP, &resp, sizeof(resp));
            return;
        }
    }

    // Use native width × height × 4 bytes (BGRA raw)
    const uint32_t w    = VncServer::getInstance().getFrameBufferWidth();
    const uint32_t h    = VncServer::getInstance().getFrameBufferHeight();
    const size_t   size = static_cast<size_t>(w) * h * 4;

    // Arm a frame update into the shared buffer at offset 0
    {
        std::lock_guard<std::mutex> lock(mFrameMutex);
        mFrameOffset  = 0;
        mFrameMaxSize = size;
        mFrameFormat  = 4; // BGR0_8_8_8_8
        mFrameReady   = false;
        mFrameResult  = 0;
    }
    mFrameUpdatePending.store(true);
    VncServer::getInstance().setVncFrameUpdateRequestFlag(true);

    {
        std::unique_lock<std::mutex> lock(mFrameMutex);
        mFrameCv.wait(lock, [this]{ return mFrameReady; });
    }
    mFrameUpdatePending.store(false);

    if (mFrameResult <= 0)
    {
        VncBridgeProtocol::ScreenshotResp resp;
        resp.success = 0;
        resp._pad[0] = resp._pad[1] = resp._pad[2] = 0;
        sendMsg(clientFd, VncBridgeProtocol::MSG_SCREENSHOT_RESP, &resp, sizeof(resp));
        return;
    }

    // Create a new memfd containing just the captured pixels and pass it
    int snapFd = RdkWindowManager::memfd_create("/vnc-screenshot", MFD_CLOEXEC);
    if (snapFd < 0 || ::ftruncate(snapFd, static_cast<off_t>(mFrameResult)) != 0)
    {
        if (snapFd >= 0) ::close(snapFd);
        VncBridgeProtocol::ScreenshotResp resp;
        resp.success = 0;
        resp._pad[0] = resp._pad[1] = resp._pad[2] = 0;
        sendMsg(clientFd, VncBridgeProtocol::MSG_SCREENSHOT_RESP, &resp, sizeof(resp));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mBufferMutex);
        void* snapPtr = ::mmap(nullptr, static_cast<size_t>(mFrameResult),
                               PROT_WRITE, MAP_SHARED, snapFd, 0);
        if (snapPtr != MAP_FAILED)
        {
            ::memcpy(snapPtr, mBridgeMemPtr, static_cast<size_t>(mFrameResult));
            ::munmap(snapPtr, static_cast<size_t>(mFrameResult));
        }
    }

    VncBridgeProtocol::ScreenshotResp resp;
    resp.success = 1;
    resp._pad[0] = resp._pad[1] = resp._pad[2] = 0;
    sendMsg(clientFd, VncBridgeProtocol::MSG_SCREENSHOT_RESP, &resp, sizeof(resp), snapFd);
    ::close(snapFd);
}

void VncBridgeServer::handleAppScreenshotRequest(int clientFd,
                                                 const std::vector<uint8_t>& /*payload*/)
{
    // Per-app screenshots are not supported via the bridge path
    sendError(clientFd, ENOSYS, "App screenshots not supported via RDKWindowManager bridge");
}

// ── GL-thread integration ─────────────────────────────────────────────────────

bool VncBridgeServer::isBridgeFrameUpdatePending() const
{
    return mFrameUpdatePending.load();
}

void VncBridgeServer::deliverFrame(const uint8_t* rgbaData,
                                   uint32_t width,
                                   uint32_t height)
{
    if (!mFrameUpdatePending.load())
        return;

    std::lock_guard<std::mutex> bufLock(mBufferMutex);
    if (mBridgeMemPtr == nullptr)
    {
        std::lock_guard<std::mutex> frameLock(mFrameMutex);
        mFrameResult = -ENOMEM;
        mFrameReady  = true;
        mFrameCv.notify_all();
        return;
    }

    uint8_t* dest     = mBridgeMemPtr + mFrameOffset;
    size_t   maxBytes = mBridgeMemSize > mFrameOffset
                        ? mBridgeMemSize - mFrameOffset
                        : 0;

    // Respect the maxSize hint from the client
    if (mFrameMaxSize > 0 && mFrameMaxSize < maxBytes)
        maxBytes = mFrameMaxSize;

    // Convert RGBA → BGR0 (the format reported in GetDetails / default capture format)
    const uint32_t pixels        = width * height;
    const size_t   requiredBytes = pixels * 4;

    if (requiredBytes > maxBytes)
    {
        Logger::log(LogLevel::Error, "%s: bridge buffer too small (%zu < %zu)",
                    __func__, maxBytes, requiredBytes);
        std::lock_guard<std::mutex> frameLock(mFrameMutex);
        mFrameResult = -ENOBUFS;
        mFrameReady  = true;
        mFrameCv.notify_all();
        return;
    }

    // RGBA → BGR0 conversion + vertical flip (OpenGL origin is bottom-left)
    const int rowStride = static_cast<int>(width) * 4;
    for (uint32_t y = 0; y < height; ++y)
    {
        uint32_t        srcRow  = (height - 1 - y);   // flip vertically
        const uint8_t*  srcPtr  = rgbaData + srcRow * rowStride;
        uint8_t*        dstPtr  = dest + y * rowStride;
        for (uint32_t x = 0; x < width; ++x)
        {
            dstPtr[0] = srcPtr[2]; // B
            dstPtr[1] = srcPtr[1]; // G
            dstPtr[2] = srcPtr[0]; // R
            dstPtr[3] = 0;         // 0
            srcPtr += 4;
            dstPtr += 4;
        }
    }

    {
        std::lock_guard<std::mutex> frameLock(mFrameMutex);
        mFrameResult = static_cast<int64_t>(requiredBytes);
        mFrameReady  = true;
    }
    mFrameCv.notify_all();
    Logger::log(LogLevel::Information, "%s: delivered %u×%u frame (%zu bytes)",
                __func__, width, height, requiredBytes);
}

// ── Low-level I/O ─────────────────────────────────────────────────────────────

bool VncBridgeServer::sendMsg(int fd, uint32_t type,
                              const void* payload, size_t payloadLen,
                              int passFd)
{
    VncBridgeProtocol::MsgHeader hdr;
    hdr.type       = type;
    hdr.payloadLen = static_cast<uint32_t>(payloadLen);

    // Build iovec: [header][payload]
    struct iovec iov[2];
    iov[0].iov_base = &hdr;
    iov[0].iov_len  = sizeof(hdr);
    iov[1].iov_base = const_cast<void*>(payload);
    iov[1].iov_len  = payloadLen;

    struct msghdr msg;
    ::memset(&msg, 0, sizeof(msg));
    msg.msg_iov    = iov;
    msg.msg_iovlen = (payloadLen > 0) ? 2 : 1;

    // Ancillary data for fd passing
    char cmsgBuf[CMSG_SPACE(sizeof(int))];
    if (passFd >= 0)
    {
        ::memset(cmsgBuf, 0, sizeof(cmsgBuf));
        msg.msg_control    = cmsgBuf;
        msg.msg_controllen = sizeof(cmsgBuf);
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type  = SCM_RIGHTS;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
        ::memcpy(CMSG_DATA(cmsg), &passFd, sizeof(int));
    }

    ssize_t ret = ::sendmsg(fd, &msg, MSG_NOSIGNAL);
    if (ret < 0)
    {
        Logger::log(LogLevel::Error, "%s: sendmsg failed: %s", __func__, ::strerror(errno));
        return false;
    }
    return true;
}

bool VncBridgeServer::recvMsg(int fd,
                              uint32_t& outType,
                              std::vector<uint8_t>& outPayload,
                              int& outPassedFd)
{
    outPassedFd = -1;

    // First receive the 8-byte header
    VncBridgeProtocol::MsgHeader hdr;
    char   cmsgBuf[CMSG_SPACE(sizeof(int))];
    struct iovec iov;
    iov.iov_base = &hdr;
    iov.iov_len  = sizeof(hdr);

    struct msghdr msg;
    ::memset(&msg, 0, sizeof(msg));
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = cmsgBuf;
    msg.msg_controllen = sizeof(cmsgBuf);

    ssize_t ret = ::recvmsg(fd, &msg, MSG_WAITALL);
    if (ret <= 0)
        return false;
    if (static_cast<size_t>(ret) < sizeof(hdr))
        return false;

    // Extract any passed fd from the header receive
    for (struct cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm != nullptr; cm = CMSG_NXTHDR(&msg, cm))
    {
        if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS)
        {
            ::memcpy(&outPassedFd, CMSG_DATA(cm), sizeof(int));
            break;
        }
    }

    outType = hdr.type;

    if (hdr.payloadLen == 0)
        return true;

    // Read payload (no more ancillary data expected in payload receive)
    outPayload.resize(hdr.payloadLen);
    ssize_t got = ::recv(fd, outPayload.data(), hdr.payloadLen, MSG_WAITALL);
    if (got != static_cast<ssize_t>(hdr.payloadLen))
        return false;

    return true;
}

void VncBridgeServer::sendError(int fd, uint32_t code, const std::string& msg)
{
    VncBridgeProtocol::ErrorMsg errHdr;
    errHdr.code   = code;
    errHdr.msgLen = static_cast<uint32_t>(msg.size());

    std::vector<uint8_t> payload(sizeof(errHdr) + msg.size());
    ::memcpy(payload.data(), &errHdr, sizeof(errHdr));
    ::memcpy(payload.data() + sizeof(errHdr), msg.data(), msg.size());

    sendMsg(fd, VncBridgeProtocol::MSG_ERROR, payload.data(), payload.size());
}

} // namespace RdkWindowManager
