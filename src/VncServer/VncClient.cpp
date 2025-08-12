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

/*
    Runs the state machine / protocol for an individual connected VNC remote client.

    This is expected to be event driven from an external server, ie. it doesn't
    run it's own thread or event loop.
*/

#include "VncClient.h"
#include "VncBuffer.h"
#include "MemFd.h"

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>

#include "VncServer.h"
#include "logger.h"
using namespace RdkWindowManager;


#define ROUND_UP(N, S)      ((((N) + (S) - 1) / (S)) * (S))

using namespace std::chrono_literals;

/* Static list of encodings supported. */
const std::set<VncEncoding> VncClient::mSupportedEncodings =
    {
        VncEncoding::Raw,
    };


/*
    We support palette mode - in a rubbish way - we take the RGB values and
    convert to 3_3_2 mode (3 bits of red and green and 2 bits of blue), this is
    our look-up into the palette.

    We could be better by using something like the websafe color set (6 possible
    values per channel), but even that basic transform requires more cycles than
    worth dedicating to what should be a dead format.
*/
const uint16_t VncClient::mPalette[256 * 3] =
    {
            0,     0,     0,       0,     0, 21845,       0,     0, 43690,       0,     0, 65535,
            0, 37412,     0,       0, 37412, 21845,       0, 37412, 43690,       0, 37412, 65535,
            0,  9289,     0,       0,  9289, 21845,       0,  9289, 43690,       0,  9289, 65535,
            0, 46701,     0,       0, 46701, 21845,       0, 46701, 43690,       0, 46701, 65535,
            0, 18578,     0,       0, 18578, 21845,       0, 18578, 43690,       0, 18578, 65535,
            0, 55990,     0,       0, 55990, 21845,       0, 55990, 43690,       0, 55990, 65535,
            0, 27867,     0,       0, 27867, 21845,       0, 27867, 43690,       0, 27867, 65535,
            0, 65535,     0,       0, 65535, 21845,       0, 65535, 43690,       0, 65535, 65535,
        37412,     0,     0,   37412,     0, 21845,   37412,     0, 43690,   37412,     0, 65535,
        37412, 37412,     0,   37412, 37412, 21845,   37412, 37412, 43690,   37412, 37412, 65535,
        37412,  9289,     0,   37412,  9289, 21845,   37412,  9289, 43690,   37412,  9289, 65535,
        37412, 46701,     0,   37412, 46701, 21845,   37412, 46701, 43690,   37412, 46701, 65535,
        37412, 18578,     0,   37412, 18578, 21845,   37412, 18578, 43690,   37412, 18578, 65535,
        37412, 55990,     0,   37412, 55990, 21845,   37412, 55990, 43690,   37412, 55990, 65535,
        37412, 27867,     0,   37412, 27867, 21845,   37412, 27867, 43690,   37412, 27867, 65535,
        37412, 65535,     0,   37412, 65535, 21845,   37412, 65535, 43690,   37412, 65535, 65535,
         9289,     0,     0,    9289,     0, 21845,    9289,     0, 43690,    9289,     0, 65535,
         9289, 37412,     0,    9289, 37412, 21845,    9289, 37412, 43690,    9289, 37412, 65535,
         9289,  9289,     0,    9289,  9289, 21845,    9289,  9289, 43690,    9289,  9289, 65535,
         9289, 46701,     0,    9289, 46701, 21845,    9289, 46701, 43690,    9289, 46701, 65535,
         9289, 18578,     0,    9289, 18578, 21845,    9289, 18578, 43690,    9289, 18578, 65535,
         9289, 55990,     0,    9289, 55990, 21845,    9289, 55990, 43690,    9289, 55990, 65535,
         9289, 27867,     0,    9289, 27867, 21845,    9289, 27867, 43690,    9289, 27867, 65535,
         9289, 65535,     0,    9289, 65535, 21845,    9289, 65535, 43690,    9289, 65535, 65535,
        46701,     0,     0,   46701,     0, 21845,   46701,     0, 43690,   46701,     0, 65535,
        46701, 37412,     0,   46701, 37412, 21845,   46701, 37412, 43690,   46701, 37412, 65535,
        46701,  9289,     0,   46701,  9289, 21845,   46701,  9289, 43690,   46701,  9289, 65535,
        46701, 46701,     0,   46701, 46701, 21845,   46701, 46701, 43690,   46701, 46701, 65535,
        46701, 18578,     0,   46701, 18578, 21845,   46701, 18578, 43690,   46701, 18578, 65535,
        46701, 55990,     0,   46701, 55990, 21845,   46701, 55990, 43690,   46701, 55990, 65535,
        46701, 27867,     0,   46701, 27867, 21845,   46701, 27867, 43690,   46701, 27867, 65535,
        46701, 65535,     0,   46701, 65535, 21845,   46701, 65535, 43690,   46701, 65535, 65535,
        18578,     0,     0,   18578,     0, 21845,   18578,     0, 43690,   18578,     0, 65535,
        18578, 37412,     0,   18578, 37412, 21845,   18578, 37412, 43690,   18578, 37412, 65535,
        18578,  9289,     0,   18578,  9289, 21845,   18578,  9289, 43690,   18578,  9289, 65535,
        18578, 46701,     0,   18578, 46701, 21845,   18578, 46701, 43690,   18578, 46701, 65535,
        18578, 18578,     0,   18578, 18578, 21845,   18578, 18578, 43690,   18578, 18578, 65535,
        18578, 55990,     0,   18578, 55990, 21845,   18578, 55990, 43690,   18578, 55990, 65535,
        18578, 27867,     0,   18578, 27867, 21845,   18578, 27867, 43690,   18578, 27867, 65535,
        18578, 65535,     0,   18578, 65535, 21845,   18578, 65535, 43690,   18578, 65535, 65535,
        55990,     0,     0,   55990,     0, 21845,   55990,     0, 43690,   55990,     0, 65535,
        55990, 37412,     0,   55990, 37412, 21845,   55990, 37412, 43690,   55990, 37412, 65535,
        55990,  9289,     0,   55990,  9289, 21845,   55990,  9289, 43690,   55990,  9289, 65535,
        55990, 46701,     0,   55990, 46701, 21845,   55990, 46701, 43690,   55990, 46701, 65535,
        55990, 18578,     0,   55990, 18578, 21845,   55990, 18578, 43690,   55990, 18578, 65535,
        55990, 55990,     0,   55990, 55990, 21845,   55990, 55990, 43690,   55990, 55990, 65535,
        55990, 27867,     0,   55990, 27867, 21845,   55990, 27867, 43690,   55990, 27867, 65535,
        55990, 65535,     0,   55990, 65535, 21845,   55990, 65535, 43690,   55990, 65535, 65535,
        27867,     0,     0,   27867,     0, 21845,   27867,     0, 43690,   27867,     0, 65535,
        27867, 37412,     0,   27867, 37412, 21845,   27867, 37412, 43690,   27867, 37412, 65535,
        27867,  9289,     0,   27867,  9289, 21845,   27867,  9289, 43690,   27867,  9289, 65535,
        27867, 46701,     0,   27867, 46701, 21845,   27867, 46701, 43690,   27867, 46701, 65535,
        27867, 18578,     0,   27867, 18578, 21845,   27867, 18578, 43690,   27867, 18578, 65535,
        27867, 55990,     0,   27867, 55990, 21845,   27867, 55990, 43690,   27867, 55990, 65535,
        27867, 27867,     0,   27867, 27867, 21845,   27867, 27867, 43690,   27867, 27867, 65535,
        27867, 65535,     0,   27867, 65535, 21845,   27867, 65535, 43690,   27867, 65535, 65535,
        65535,     0,     0,   65535,     0, 21845,   65535,     0, 43690,   65535,     0, 65535,
        65535, 37412,     0,   65535, 37412, 21845,   65535, 37412, 43690,   65535, 37412, 65535,
        65535,  9289,     0,   65535,  9289, 21845,   65535,  9289, 43690,   65535,  9289, 65535,
        65535, 46701,     0,   65535, 46701, 21845,   65535, 46701, 43690,   65535, 46701, 65535,
        65535, 18578,     0,   65535, 18578, 21845,   65535, 18578, 43690,   65535, 18578, 65535,
        65535, 55990,     0,   65535, 55990, 21845,   65535, 55990, 43690,   65535, 55990, 65535,
        65535, 27867,     0,   65535, 27867, 21845,   65535, 27867, 43690,   65535, 27867, 65535,
        65535, 65535,     0,   65535, 65535, 21845,   65535, 65535, 43690,   65535, 65535, 65535
    };

VncClient::VncClient(ClientMode mode,
                     std::shared_ptr<IVncSocket> vncSocket)
    : mMode(mode)
    , mProtocolVersion(mode == ClientMode::RFC6143 ? "RFB 003.008\n" : "")
    , mVncSocket(std::move(vncSocket))
    , mFrameBufferWidth(VncServer::getInstance().getFrameBufferWidth())
    , mFrameBufferHeight(VncServer::getInstance().getFrameBufferHeight())
    , mSupportContinuousUpdates(false)
    , mState(ProtocolVersionHandshake)
    , mRfbVersion(RFB_3_8)
    , mEncoding(VncEncoding::Invalid)
    , mPixelFormat(ClientCaptureFormat::BGR0_8_8_8_8)
    , mContinuousUpdatesEnabled(false)
    , mSendPalettePending(false)
{
    Logger::log(LogLevel::Information, "mMode : %d, mProtocolVersion : %s", mMode, mProtocolVersion.c_str());

    // create two ring buffers for the input and output messages
    mReadBuffer = std::make_shared<VncBuffer>(4096);
    if (!mReadBuffer->isValid())
    {
        Logger::log(LogLevel::Information, "failed creating vnc transfer buffers");
        terminate();
        return;
    }

    VncServer::getInstance().setVncSocket(mVncSocket);
    mVncSocket->readReady.connect(std::bind(&VncClient::onRecvData, this));
    mVncSocket->closed.connect(std::bind(&VncClient::onSocketClosed, this));

    writeProtocolVersion(); // the first thing we do is send our protocol version
}

VncClient::~VncClient()
{
    // manually reset the socket
    mVncSocket.reset();
}

void VncClient::terminate()
{
    // if already terminated then don't do anything
    if (mState == Terminated)
        return;

    // move to the terminating state
    mState = Terminating;

    // close the socket (can be async operation)
    if (mVncSocket->state() == IVncSocket::State::Open)
    {
        mVncSocket->close();
    }

    // emit the terminated signal if both the bridge and socket are closed
    if (mVncSocket->state() == IVncSocket::State::Closed)
    {
        mState = Terminated;
        terminated();
    }
}

void VncClient::onRecvData()
{
    // copy the data into the read buffer
    if (mReadBuffer->full())
    {
        Logger::log(LogLevel::Information, "receive buffer full, clearing and processing new data");
        mReadBuffer->clear();
    }

    // read as much data from the socket as will fit in the buffer
    ssize_t read = mVncSocket->read(mReadBuffer->head<void>(), mReadBuffer->space());
    if (read > 0)
    {
        mReadBuffer->advanceHead(read);

        // process the read buffer, looping while successfully processed a message
        // and the read buffer is not empty
        while (!mReadBuffer->empty() && (mState != Terminated) && processNextMessage())
        {
            continue;
        }
    }
}

void VncClient::onSocketClosed()
{
    Logger::log(LogLevel::Information, "received notification client socket has closed");

    terminate();
}

bool VncClient::isTerminated() const
{
    return (mState == Terminated);
}

/* Returns the mode of the client, either Sky legacy mode, or RFC6143 mode.  */
VncClient::ClientMode VncClient::mode() const
{
    return mMode;
}

/* Writes our protocol string into the write buffer. */
void VncClient::writeProtocolVersion()
{
    Logger::log(LogLevel::Information, "writing protocol version");

    mVncSocket->write(g_bytes_new_static(mProtocolVersion.data(), mProtocolVersion.length()));
}

/*  Writes our supported security types into the write buffer.  Currently don't
    support any security modes. */
void VncClient::writeSecurityTypes()
{
    Logger::log(LogLevel::Information, "sending security types");

    if (mRfbVersion == RFB_3_3)
    {
        uint32_t securityType = SecurityType::None;
        securityType = htonl(securityType);

        mVncSocket->write(g_bytes_new(&securityType, 4));
    }
    else
    {
        uint8_t securityTypes[2] = { 0x01, SecurityType::None };

        mVncSocket->write(g_bytes_new(&securityTypes, 2));
    }
}

void VncClient::writeInvalidVersionError()
{
    static const char error[] = "\0Invalid version\n";

    mVncSocket->write(g_bytes_new_static(error, sizeof(error) - 1));
}

void VncClient::writeSecurityResult(bool result)
{
    Logger::log(LogLevel::Information, "sending security result");

    uint32_t status = result ? 0 : 1;
    status = htonl(status);

    mVncSocket->write(g_bytes_new(&status, sizeof(status)));
}

void VncClient::writeServerInit()
{
    VncServerInit serverInit;

    const std::string friendlyName = VncServer::getInstance().getFriendlyName();

    const uint16_t width = mFrameBufferWidth;
    const uint16_t height = mFrameBufferHeight;

    serverInit.frameBufferWidth = htons(width);
    serverInit.frameBufferHeight = htons(height);

    serverInit.pixelFormat.bitsPerPixel = 32;
    serverInit.pixelFormat.depth = 24;
    serverInit.pixelFormat.bigEndianEncoding = 0;
    serverInit.pixelFormat.trueColorFlag = 1;
    serverInit.pixelFormat.redMax = htons(255);
    serverInit.pixelFormat.greenMax = htons(255);
    serverInit.pixelFormat.blueMax = htons(255);
    serverInit.pixelFormat.redShift = 16;
    serverInit.pixelFormat.greenShift = 8;
    serverInit.pixelFormat.blueShift = 0;

    serverInit.nameLength = htonl(friendlyName.size());

    GByteArray *message = g_byte_array_new();

    // write the serviceInit struct
    g_byte_array_append(message, (const guint8*)&serverInit, sizeof(serverInit));

    // write the friendly name
    g_byte_array_append(message, (const guint8*)friendlyName.data(), friendlyName.size());

    Logger::log(LogLevel::Information, "sending ServerInit message - friendly name '%s', size = %u",
           friendlyName.c_str(), message->len);

    mVncSocket->write(g_byte_array_free_to_bytes(message));
}

/* Called when the poll loop times-out while in the handshaking phase. */
void VncClient::onTimeout()
{
    if (mState != Running)
    {
        std::chrono::seconds since =
            std::chrono::duration_cast<std::chrono::seconds>
                (std::chrono::steady_clock::now() - mLastValidClientMessage);

        if (since > std::chrono::seconds(10))
        {
            Logger::log(LogLevel::Information, "vnc client hasn't sent handshaking message in 10 seconds, terminating");
            terminate();
        }
    }
}

/* Checks the protocol version supplied by the client. */
bool VncClient::checkProtocolVersion(const char version[12])
{
    Logger::log(LogLevel::Information, "vnc client using version '%.*s'", 12, version);

    if (mMode == ClientMode::RFC6143)
    {
        if (memcmp(version, "RFB 003.008\n", 12) == 0)
            mRfbVersion = RFB_3_8;
        else if (memcmp(version, "RFB 003.007\n", 12) == 0)
            mRfbVersion = RFB_3_7;
        else if (memcmp(version, "RFB 003.003\n", 12) == 0)
            mRfbVersion = RFB_3_3;
        else
            return false;
    }

    return true;
}

/*  Called when data has been added to the read buffer.  This attempts to
    process any messages in the buffer.

    Returns \c true if a message was processed, otherwise \c false.
*/
bool VncClient::processNextMessage()
{
    const size_t initialSize = mReadBuffer->size();

    // process the data based on state
    State state = mState;
    Logger::log(LogLevel::Information, "processNextMessage mState: %d", mState);
    if (state == ProtocolVersionHandshake)
    {
        // RFC6143 : 7.1.1. ProtocolVersion Handshake

        // expect a 12 character version string response from the client
        if (mReadBuffer->size() < 12)
        {
            return false;
        }

        // check the version supplied by the client, if not valid then
        // terminate the connection
        if (!checkProtocolVersion(mReadBuffer->tail<char>()))
        {
            Logger::log(LogLevel::Information, "vnc client is using unknown version '%.*s'",
                      12, mReadBuffer->tail<char>());

            writeInvalidVersionError();
            terminate();
        }
        else
        {
            // send the security type(s) we support - currently none
            mLastValidClientMessage = std::chrono::steady_clock::now();
            writeSecurityTypes();

            // for RFB version 3.7 and 3.8 we perform the security handshake,
            // otherwise skip it
            if (mRfbVersion == RFB_3_3)
                mState = AwaitingClientInit;
            else
                mState = SecurityHandshake;
        }

        mReadBuffer->clear();
    }
    else if (state == SecurityHandshake)
    {
        // RFC6143 : 7.1.2. Security Handshake

        const uint8_t securityType = *(mReadBuffer->tail<uint8_t>());
        if (securityType != SecurityType::None)
        {
            writeSecurityResult(false);
            terminate();
        }
        else
        {
            mLastValidClientMessage = std::chrono::steady_clock::now();
            if (mRfbVersion == RFB_3_8)
                writeSecurityResult(true);
            mState = AwaitingClientInit;
        }

        mReadBuffer->clear();
    }
    else if (state == AwaitingClientInit)
    {
        // RFC6143 : 7.3.1. ClientInit

        const uint8_t sharedFlag = *(mReadBuffer->tail<uint8_t>());
        if (sharedFlag == 0x00)
        {
            Logger::log(LogLevel::Information, "FIXME: don't support vnc exclusive mode");
        }

        mLastValidClientMessage = std::chrono::steady_clock::now();

        writeServerInit();
        mState = Running;

        mReadBuffer->clear();
    }
    else if (state == Running)
    {
        // RFC6143 : 7.5.  Client-to-Server Messages

        const uint8_t type = *(mReadBuffer->tail<uint8_t>());
        Logger::log(LogLevel::Information, "processNextMessage type: %d", type);
        switch (type)
        {
            case SetPixelFormat:
                if (mReadBuffer->size() >= sizeof(VncSetPixelFormat))
                {
                    onSetPixelFormat(mReadBuffer->tail<VncSetPixelFormat>());
                    mReadBuffer->advanceTail(sizeof(VncSetPixelFormat));
                    VncServer::getInstance().setVncFrameUpdatePixelFormat(mPixelFormat);
                }
                break;

            case SetEncodings:
                if (mReadBuffer->size() >= sizeof(VncSetEncoding))
                {
                    const VncSetEncoding *setEncoding = mReadBuffer->tail<VncSetEncoding>();
                    const uint16_t count = ntohs(setEncoding->numberOfEncodings);
                    if (count > 32)
                    {
                        Logger::log(LogLevel::Information, "invalid number of encodings");
                        mReadBuffer->clear();
                        terminate();
                    }
                    else
                    {
                        const size_t totalRequireSize = sizeof(VncSetEncoding)
                                                        + (count * sizeof(uint32_t));
                        if (mReadBuffer->size() >= totalRequireSize)
                        {
                            onSetEncodings(mReadBuffer->tail<VncSetEncoding>());
                            mReadBuffer->advanceTail(totalRequireSize);
                        }
                    }
                }
                break;

            case FramebufferUpdateRequest:
                if (mReadBuffer->size() >= sizeof(VncFramebufferUpdateRequest))
                {
                    auto request = mReadBuffer->tail<VncFramebufferUpdateRequest>();
                    Logger::log(LogLevel::Information, "FrameUpdateRequest: { incremental:%s, x:%hu, y:%hu, width:%hu, height:%hu }",
                           request->increment ? "yes" : "no",
                           ntohs(request->xPosition), ntohs(request->yPosition),
                           ntohs(request->width), ntohs(request->height));

                    onFrameUpdateRequest(request->increment != 0);
                    mReadBuffer->advanceTail(sizeof(VncFramebufferUpdateRequest));
                }
                else
                {
                    Logger::log(LogLevel::Information, "waiting for more data to complete FramebufferUpdateRequest request");
                }
                break;

            case KeyEvent:
                if (mReadBuffer->size() >= sizeof(VncKeyEvent))
                {
                    onKeyEvent(mReadBuffer->tail<VncKeyEvent>());
                    mReadBuffer->advanceTail(sizeof(VncKeyEvent));
                }
                else
                {
                    Logger::log(LogLevel::Information, "waiting for more data to complete KeyEvent request");
                }
                break;

            case PointerEvent:
                if (mReadBuffer->size() >= sizeof(VncPointerEvent))
                {
                    //Logger::log(LogLevel::Information, "ignoring pointer event");
                    mReadBuffer->advanceTail(sizeof(VncPointerEvent));
                }
                break;

            case ClientCutText:
                if (mReadBuffer->size() >= sizeof(VncClientCutText))
                {
                    const VncClientCutText *header = mReadBuffer->tail<VncClientCutText>();
                    const uint32_t len = ntohl(header->length);
                    const size_t totalRequiredSize = sizeof(VncClientCutText) + len;

                    if (mReadBuffer->size() >= totalRequiredSize)
                    {
                        Logger::log(LogLevel::Information, "ignoring clipboard cut text message");
                        mReadBuffer->advanceTail(totalRequiredSize);
                    }
                }
                break;

            case ContinuousUpdates:
                if (mReadBuffer->size() >= sizeof(VncEnableContinuousUpdates))
                {
                    onEnableContinuousUpdates(mReadBuffer->tail<VncEnableContinuousUpdates>());
                    mReadBuffer->advanceTail(sizeof(VncEnableContinuousUpdates));
                }
                else
                {
                    Logger::log(LogLevel::Information, "waiting for more data to complete EnableContinuousUpdates request");
                }
                break;

            default:
                Logger::log(LogLevel::Information, "unknown vnc message %d", type);
                mReadBuffer->clear();
                break;
        }
    }

    // return true if some of the buffer was consumed
    return (initialSize != mReadBuffer->size());
}

/* Called when a KeyEvent message has been received. */
void VncClient::onKeyEvent(const VncKeyEvent *keyEvent)
{
    const uint32_t vncKeyCode = ntohl(keyEvent->keyCode);
    const bool keyDown = (keyEvent->downFlag != 0);

    Logger::log(LogLevel::Information, "Ignore received vnc key event (key 0x%04x %s)", vncKeyCode, keyDown ? "down" : "up");
}

/* Called when a SetPixelFormat message has been received from the client. */
void VncClient::onSetPixelFormat(const VncSetPixelFormat *format)
{
    Logger::log(LogLevel::Information, "VncSetPixelFormat: { bpp:%hhu, depth:%hhu, %s-endian, trueColor:%s, rgbMax:%hu:%hu:%hu rgbShift:%hhu:%hhu:%hhu }",
           format->pixelFormat.bitsPerPixel,
           format->pixelFormat.depth,
           format->pixelFormat.bigEndianEncoding ? "big" : "little",
           format->pixelFormat.trueColorFlag ? "yes" : "no",
           ntohs(format->pixelFormat.redMax),
           ntohs(format->pixelFormat.greenMax),
           ntohs(format->pixelFormat.blueMax),
           format->pixelFormat.redShift,
           format->pixelFormat.greenShift,
           format->pixelFormat.blueShift);

    const VncPixelFormat &pixelFormat = format->pixelFormat;

    if (pixelFormat.trueColorFlag == 0)
    {
        // for our simplistic palette implementation, we just use 3_3_2 format
        // and supply a palette with those matches
        mPixelFormat = ClientCaptureFormat::RGB_3_3_2;
        mSendPalettePending = true;
    }
    else if ((pixelFormat.bitsPerPixel == 32) && (pixelFormat.depth == 24))
    {
        if (pixelFormat.bigEndianEncoding)
        {
            mPixelFormat = ClientCaptureFormat::ARGB_8_8_8_8;
        }
        else
        {
            if ((format->pixelFormat.redShift == 0) &&
                (format->pixelFormat.greenShift == 8) &&
                (format->pixelFormat.blueShift == 16))
                mPixelFormat = ClientCaptureFormat::RGB0_8_8_8_8;
            else
                mPixelFormat = ClientCaptureFormat::BGR0_8_8_8_8;
        }
    }
    else if ((pixelFormat.bitsPerPixel == 8) && (pixelFormat.depth == 8) &&
             (pixelFormat.redShift == 5) && (pixelFormat.greenShift == 2) &&
             (pixelFormat.blueShift == 0))
    {
        // 8-bit mode used by tigerVNC
        mPixelFormat = ClientCaptureFormat::RGB_3_3_2;
    }
    else if ((format->pixelFormat.bitsPerPixel == 8) && (format->pixelFormat.depth == 6))
    {
        mPixelFormat = ClientCaptureFormat::RGB_2_2_2;
    }
    else if ((format->pixelFormat.bitsPerPixel == 8) && (format->pixelFormat.depth == 3))
    {
        mPixelFormat = ClientCaptureFormat::RGB_1_1_1;
    }
    else
    {
        Logger::log(LogLevel::Information, "requested pixel format not supported, terminating client connection");
        terminate();
        return;
    }

    Logger::log(LogLevel::Information, "using pixel format %s %s",
           (mPixelFormat == RGB_3_3_2)    ? "RGB_3_3_2" :
           (mPixelFormat == ARGB_8_8_8_8) ? "ARGB_8_8_8_8" :
           (mPixelFormat == BGR0_8_8_8_8) ? "BGR0_8_8_8_8" :
           (mPixelFormat == RGB0_8_8_8_8) ? "RGB0_8_8_8_8" :
           (mPixelFormat == RGB_2_2_2)    ? "RGB_2_2_2" :
           (mPixelFormat == RGB_1_1_1)    ? "RGB_1_1_1" :  "Unknown",
           mSendPalettePending ? "(palette)" : "");
}

/* Called when a SetEncoding message has been received from the client. */
void VncClient::onSetEncodings(const VncSetEncoding *encoding)
{
    static const std::map<VncEncoding, const char*> encodingTypes =
    {
        { VncEncoding::Raw,      "Raw"       },
        { VncEncoding::CopyRect, "CopyRect"  },
        { VncEncoding::RRE,      "RRE"       },
        { VncEncoding::CoRRE,    "CoRRE"     },
        { VncEncoding::Hextile,  "Hextile"   },
        { VncEncoding::Zlib,     "Zlib"      },
        { VncEncoding::Tight,    "Tight"     },
        { VncEncoding::TightPNG, "TightPNG"  },
        { VncEncoding::ZLibHex,  "ZLibHex"   },
        { VncEncoding::TRLE,     "TRLE"      },
        { VncEncoding::ZRLE,     "ZRLE"      },
        { VncEncoding::ZYWRLE,   "ZYWRLE"    },
        { VncEncoding::CursorPseudoEncoding,            "Cursor pseudo-encoding" },
        { VncEncoding::DesktopSizePseudoEncoding,       "DesktopSize pseudo-encoding" },
        { VncEncoding::GIIPseudoEncoding,               "gii pseudo-encoding" },
        { VncEncoding::ContinuousUpdatesPseudoEncoding, "Continuous Updates pseudo-encoding" },
    };

    const unsigned n = ntohs(encoding->numberOfEncodings);
    Logger::log(LogLevel::Information, "VncSetEncoding: number of encodings %u", n);

    bool requestedContinuousUpdates = false;

    mEncoding = VncEncoding::Invalid;
    for (unsigned i = 0; i < n; i++)
    {
        // get the type and if supported set it as the encoding type to use
        VncEncoding type = static_cast<VncEncoding>(ntohl(encoding->encodings[i]));
        if ((mEncoding == VncEncoding::Invalid) && (mSupportedEncodings.count(type) > 0))
        {
            mEncoding = type;
        }
        // if continuous updates pseudo-encoding requested then changes how we run
        if (type == VncEncoding::ContinuousUpdatesPseudoEncoding)
        {
            requestedContinuousUpdates = mSupportContinuousUpdates;
        }

        auto it = encodingTypes.find(type);
        if (it != encodingTypes.end())
            Logger::log(LogLevel::Information, "    encoding type %s (%d)", it->second, int(type));
        else
            Logger::log(LogLevel::Information, "    encoding type Unknown (%d)", int(type));
    }

    // if none matched we always support raw
    if (mEncoding == VncEncoding::Invalid)
    {
        mEncoding = VncEncoding::Raw;
    }

    Logger::log(LogLevel::Information, "selected encoding type %s (%d)",
           encodingTypes.at(mEncoding), int(mEncoding));

    // if continuous updates is requested then send a EndOfContinuousUpdates now
    // to tell the client we support it
    if (requestedContinuousUpdates)
    {
        VncEndOfContinuousUpdates message;
        message.messageType = ContinuousUpdates;

        Logger::log(LogLevel::Information, "sending EndOfContinuousUpdates response");

        mVncSocket->write(g_bytes_new(&message, sizeof(message)));
    }
}

/*
    Called when an EnableContinuousUpdates message has been received from the client.

    We reject this if the continuous updates support is disabled, which is the default.
 */
void VncClient::onEnableContinuousUpdates(const VncEnableContinuousUpdates *enable)
{
    Logger::log(LogLevel::Information, "EnableContinuousUpdates: { enable:%hhu, x:%hu, y:%hu, width:%hu, height:%hu }",
           enable->enableFlag,
           ntohs(enable->xPosition), ntohs(enable->yPosition),
           ntohs(enable->width), ntohs(enable->height));

    if (enable->enableFlag == 0)
    {
        if (mContinuousUpdatesEnabled)
        {
            mContinuousUpdatesEnabled = false;
            Logger::log(LogLevel::Information, "disabling continuous update mode");
        }

        VncEndOfContinuousUpdates message;
        message.messageType = ContinuousUpdates;
        mVncSocket->write(g_bytes_new(&message, sizeof(message)));
    }
    else if (!mContinuousUpdatesEnabled)
    {
        if (mSupportContinuousUpdates)
        {
            mContinuousUpdatesEnabled = true;
            Logger::log(LogLevel::Information, "enabling continuous update mode");

            onFrameUpdateRequest(false);
        }
        else
        {
            Logger::log(LogLevel::Information, "received EnableContinuousUpdates request when not supported, ignoring");
        }
    }
}

/*
    Called when a FrameUpdateRequest message has been received from the client,
    or when continuous updates are enabled.
*/
void VncClient::onFrameUpdateRequest(bool incremental)
{
    // ignore requested if an incremental update and continuous updates are enabled
    if (incremental && mContinuousUpdatesEnabled)
    {
        Logger::log(LogLevel::Information, "ignoring incremental frame update request as continuous updates are available");
        return;
    }

    // if the format is palette and we haven't sent the palette do that now
    if (mSendPalettePending)
    {
        GByteArray *message = g_byte_array_new();

        VncSetColorMapEntries colorMap;
        colorMap.messageType = 0x01;
        colorMap.firstColor = htons(0);
        colorMap.numberOfColors = htons(256);

        g_byte_array_append(message, (const guint8*)&colorMap, sizeof(colorMap));

        g_byte_array_append(message, (const guint8*)&mPalette, sizeof(mPalette));

        mSendPalettePending = false;

        Logger::log(LogLevel::Information, "added palette to the send buffer (%zu bytes)",
               sizeof(colorMap) + sizeof(mPalette));

        mVncSocket->write(g_byte_array_free_to_bytes(message));
    }

    VncServer::getInstance().setVncFrameUpdateRequestFlag(true);
}

