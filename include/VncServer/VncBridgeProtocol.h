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

// VncBridgeProtocol.h
//
// Plain binary framing protocol used between rdk-window-manager (server) and
// VNCServer2/appsservice-vncd (client) when built with ENABLE_RDKWINDOWMANAGER_VNCSERVER2.
//
// The matching definitions on the VNCServer2 side are in:
//   appinfrastructure/RDK/AppManager/Tools/VncServer2/source/VncRdkWmBridgeConn.cpp
//
// KEEP BOTH SIDES IN SYNC.
//
// Wire format for every exchange:
//   [MsgHeader (8 bytes)] [payload (payloadLen bytes)]
//   File descriptors are passed via SCM_RIGHTS ancillary data.
//
// Socket: abstract Unix domain socket at path "\0/tmp/rdkwindowmanager-vnc-bridge"

#pragma once

#include <cstdint>

namespace VncBridgeProtocol {

// ── Wire header ──────────────────────────────────────────────────────────────

struct MsgHeader
{
    uint32_t type;       // MsgType enum value
    uint32_t payloadLen; // byte length of the payload following this header
};

// ── Message types ─────────────────────────────────────────────────────────────

enum MsgType : uint32_t
{
    MSG_GET_DETAILS_REQ     =  1,   // client → server  (no payload)
    MSG_GET_DETAILS_RESP    =  2,   // server → client  (GetDetailsResp + name string)
    MSG_SET_BUFFER_REQ      =  3,   // client → server  (SetBufferReq  + SCM_RIGHTS fd)
    MSG_SET_BUFFER_RESP     =  4,   // server → client  (SetBufferResp)
    MSG_FRAME_UPDATE_REQ    =  5,   // client → server  (FrameUpdateReq)
    MSG_FRAME_UPDATE_RESP   =  6,   // server → client  (FrameUpdateResp)
    MSG_SCREENSHOT_REQ      =  7,   // client → server  (ScreenshotReq)
    MSG_SCREENSHOT_RESP     =  8,   // server → client  (ScreenshotResp + SCM_RIGHTS fd on success)
    MSG_APP_SCREENSHOT_REQ  =  9,   // client → server  (AppScreenshotReq + appId string)
    MSG_APP_SCREENSHOT_RESP = 10,   // server → client  (ScreenshotResp + SCM_RIGHTS fd on success)
    MSG_ERROR               = 255,  // bidirectional    (ErrorMsg + message string)
};

// ── Payload structures (packed / explicit sizes to avoid ABI surprises) ───────

struct GetDetailsResp
{
    uint32_t width;
    uint32_t height;
    uint32_t pixelFormat;  // IVncBridgeConn::CaptureFormat value
    uint32_t nameLen;      // byte length of the name string that follows
    // char name[nameLen] immediately follows this struct
};

struct SetBufferReq
{
    uint32_t size;       // total size of the shared buffer in bytes
    uint8_t  wrap;       // non-zero if the server should wrap around
    uint8_t  _pad[3];
};

struct SetBufferResp
{
    uint8_t success;     // non-zero on success
    uint8_t _pad[3];
};

struct FrameUpdateReq
{
    uint64_t offset;    // byte offset into the shared buffer to write pixel data
    uint64_t maxSize;   // maximum bytes to write
    uint32_t format;    // requested CaptureFormat
    uint32_t _pad;
};

struct FrameUpdateResp
{
    int64_t bytesWritten;  // >= 0 on success; negative errno on failure
};

struct ScreenshotReq
{
    uint32_t format;   // ImageFormat (PNG_RGBA8888, …)
    uint32_t width;    // requested width  (0 = native)
    uint32_t height;   // requested height (0 = native)
};

struct AppScreenshotReq
{
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t appIdLen; // byte length of the appId string that follows
    // char appId[appIdLen] immediately follows this struct
};

struct ScreenshotResp
{
    uint8_t success;   // non-zero on success; on success an fd is passed via SCM_RIGHTS
    uint8_t _pad[3];
};

struct ErrorMsg
{
    uint32_t code;     // errno or application-defined error code
    uint32_t msgLen;   // byte length of the human-readable message that follows
    // char msg[msgLen] immediately follows this struct
};

} // namespace VncBridgeProtocol
