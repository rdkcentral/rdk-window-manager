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

#include <cstdint>

struct VncPixelFormat
{
    uint8_t bitsPerPixel = 0;
    uint8_t depth = 0;
    uint8_t bigEndianEncoding = 0;
    uint8_t trueColorFlag = 0;
    uint16_t redMax = 0;
    uint16_t greenMax = 0;
    uint16_t blueMax = 0;
    uint8_t redShift = 0;
    uint8_t greenShift = 0;
    uint8_t blueShift = 0;
    uint8_t padding[3] = { 0, 0, 0 };
}  __attribute__((packed));

static_assert(sizeof(VncPixelFormat) == 16, "invalid PixelFormat packing");


struct VncServerInit
{
    uint16_t frameBufferWidth = 0;
    uint16_t frameBufferHeight = 0;
    VncPixelFormat pixelFormat;
    uint32_t nameLength = 0;
}  __attribute__((packed));

static_assert(sizeof(VncServerInit) == 24, "invalid ServerInit packing");


struct VncSetPixelFormat
{
    uint8_t messageType;
    uint8_t padding[3];
    VncPixelFormat pixelFormat;
}  __attribute__((packed));

static_assert(sizeof(VncSetPixelFormat) == 20, "invalid SetPixelFormat packing");


struct VncSetEncoding
{
    uint8_t messageType;
    uint8_t padding;
    uint16_t numberOfEncodings;
    int32_t encodings[];
}  __attribute__((packed));

static_assert(sizeof(VncSetEncoding) == 4, "invalid SetEncoding packing");


enum class VncEncoding : int32_t
{
    Invalid = -1,

    Raw = 0,
    CopyRect = 1,
    RRE = 2,
    CoRRE = 4,
    Hextile = 5,
    Zlib = 6,
    Tight = 7,
    ZLibHex = 8,
    TRLE = 15,
    ZRLE = 16,
    ZYWRLE = 17,
    TightPNG = -260,

    CursorPseudoEncoding = -239,
    DesktopSizePseudoEncoding = -223,
    GIIPseudoEncoding = -305,
    ContinuousUpdatesPseudoEncoding = -313,
};


// [RFC 6143] 7.5.3.  FramebufferUpdateRequest
struct VncFramebufferUpdateRequest
{
    uint8_t messageType;
    uint8_t increment;
    uint16_t xPosition;
    uint16_t yPosition;
    uint16_t width;
    uint16_t height;
}  __attribute__((packed));

static_assert(sizeof(VncFramebufferUpdateRequest) == 10, "invalid VncFramebufferUpdateRequest packing");


// [RFC 6143] 7.6.1.  FramebufferUpdate
struct VncFrameBufferRectangle
{
    uint16_t xPosition;
    uint16_t yPosition;
    uint16_t width;
    uint16_t height;
    int32_t encodingType;
}  __attribute__((packed));

static_assert(sizeof(VncFrameBufferRectangle) == 12, "invalid VncFrameBufferRectangle packing");


struct VncFrameBufferUpdate
{
    uint8_t messageType;
    uint8_t padding = 0;
    uint16_t numberOfRectangles;
    VncFrameBufferRectangle rectangles[];
}  __attribute__((packed));

static_assert(sizeof(VncFrameBufferUpdate) == 4, "invalid VncFrameBufferUpdate packing");


struct VncColorMapEntry
{
    uint16_t red;
    uint16_t green;
    uint16_t blue;
}  __attribute__((packed));

struct VncSetColorMapEntries
{
    uint8_t messageType;
    uint8_t padding = 0;
    uint16_t firstColor;
    uint16_t numberOfColors;
    VncColorMapEntry colors[];
}  __attribute__((packed));

static_assert(sizeof(VncSetColorMapEntries) == 6, "invalid VncSetColorMapEntries packing");


struct VncKeyEvent
{
    uint8_t messageType;
    uint8_t downFlag;
    uint8_t padding[2] = { 0, 0 };
    uint32_t keyCode;
}  __attribute__((packed));

static_assert(sizeof(VncKeyEvent) == 8, "invalid KeyEvent packing");


// [RFC 6143] 7.5.5.  PointerEvent
struct VncPointerEvent
{
    uint8_t messageType;
    uint8_t buttonMask;
    uint16_t xPosition;
    uint16_t yPosition;
}  __attribute__((packed));

static_assert(sizeof(VncPointerEvent) == 6, "invalid VncPointerEvent packing");


// [RFC 6143] 7.5.6.  ClientCutText
struct VncClientCutText
{
    uint8_t messageType;
    uint8_t padding[3];
    uint32_t length;
    uint8_t text[];
}  __attribute__((packed));

static_assert(sizeof(VncClientCutText) == 8, "invalid VncClientCutText packing");

// https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst#enablecontinuousupdates
struct VncEnableContinuousUpdates
{
    uint8_t messageType;
    uint8_t enableFlag;
    uint16_t xPosition;
    uint16_t yPosition;
    uint16_t width;
    uint16_t height;
}  __attribute__((packed));

static_assert(sizeof(VncEnableContinuousUpdates) == 10, "invalid VncEnableContinuousUpdates packing");

// https://github.com/rfbproto/rfbproto/blob/master/rfbproto.rst#endofcontinuousupdates
struct VncEndOfContinuousUpdates
{
    uint8_t messageType;
}  __attribute__((packed));

static_assert(sizeof(VncEndOfContinuousUpdates) == 1, "invalid VncEndOfContinuousUpdates packing");

