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

#include "VncTypes.h"
#include "IVncSocket.h"

#include <boost/signals2.hpp>

#include <set>
#include <map>
#include <string>
#include <memory>
#include <bitset>
#include <chrono>
#include <optional>


class VncBuffer;


class VncClient
{
public:
    enum class ClientMode
    {
        RFC6143,
    };
    enum ClientState
    {
        Connected,
        Disconnecting,
        Disconnected
    };

    enum ClientCaptureFormat : uint32_t
    {
        InvalidFormat = 0,

        RGBA_8_8_8_8 = 1,   // red, green, blue, alpha
        ABGR_8_8_8_8 = 2,   // alpha, blue, green, red
        ARGB_8_8_8_8 = 3,   // red, green, blue, alpha
        BGR0_8_8_8_8 = 4,   // blue, green, red, 0
        RGB0_8_8_8_8 = 5,   // red, green, blue, 0

        RGB_5_6_5 = 6,      // 5-bits of red, 6-bits of green and 5-bits of blue

        RGB_3_3_2 = 7,      // 3-bits of red and green, 2-bits of blue
        RGB_2_2_2 = 8,      // 2-bits of red, green, blue in a single byte
        RGB_1_1_1 = 9,      // 1-bit of red, green, blue in a single byte

        GREY_8 = 10,         // 8-bits per pixel of greyscale
        MONO_1 = 11,         // 1-bit per pixel of white or black

    };

public:
    VncClient(ClientMode mode, std::shared_ptr<IVncSocket> socket);
    ~VncClient();

public:
    ClientMode mode() const;
    void terminate();
    bool isTerminated() const;
    boost::signals2::signal<void()> terminated;

private:
    void onSocketClosed();
    void onRecvData();
    void onTimeout();
    bool processNextMessage();
    bool checkProtocolVersion(const char version[12]);
    void writeProtocolVersion();
    void writeSecurityTypes();
    void writeInvalidVersionError();
    void writeSecurityResult(bool result);
    void writeServerInit();
    void onSetPixelFormat(const VncSetPixelFormat *format);
    void onSetEncodings(const VncSetEncoding *encoding);
    void onFrameUpdateRequest(bool incremental);
    void onKeyEvent(const VncKeyEvent *keyEvent);
    void onEnableContinuousUpdates(const VncEnableContinuousUpdates *enable);

private:
    const ClientMode mMode;
    const std::string mProtocolVersion;
    std::shared_ptr<IVncSocket> mVncSocket;
    const uint16_t mFrameBufferWidth;
    const uint16_t mFrameBufferHeight;
    const bool mSupportContinuousUpdates;

    enum RFBVersion
    {
        RFB_3_3,
        RFB_3_7,
        RFB_3_8,
    };

    enum State : int
    {
        ProtocolVersionHandshake,
        SecurityHandshake,
        AwaitingClientInit,

        Running,

        Terminating,
        Terminated,

    };

    enum SecurityType : uint8_t
    {
        Invalid = 0x00,
        None = 0x01,
        VncAuthentication = 0x02,
    };

    /// \see https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst#client-to-server-messages
    enum ClientMessageType : uint8_t
    {
        SetPixelFormat = 0,
        SetEncodings = 2,
        FramebufferUpdateRequest = 3,
        KeyEvent = 4,
        PointerEvent = 5,
        ClientCutText = 6,

        ContinuousUpdates = 150,

        GiiClient = 253,
    };

    State mState;
    RFBVersion mRfbVersion;
    std::shared_ptr<VncBuffer> mReadBuffer;
    std::chrono::steady_clock::time_point mLastValidClientMessage;
    VncEncoding mEncoding;
    ClientCaptureFormat mPixelFormat;
    bool mContinuousUpdatesEnabled;
    bool mSendPalettePending;
    static const std::set<VncEncoding> mSupportedEncodings;
    static const uint16_t mPalette[256 * 3];
};
