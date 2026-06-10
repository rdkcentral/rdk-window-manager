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

#include "rdkcompositor.h"
#include "compositorcontroller.h"

#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "linuxkeys.h"
#include "rdkwindowmanager.h"
#include "framebuffer.h"
#include "framebufferrenderer.h"
#include "logger.h"

extern bool gForce720;

namespace RdkWindowManager
{
    #define RDK_WINDOW_MANAGER_INITIAL_INPUT_LISTENER_TAG 1001
    #define RDK_WINDOW_MANAGER_INITIAL_STATE_CHANGE_LISTENER_TAG 2001

    void launchApplicationThreadCallback(RdkCompositor* compositor)
    {
        if (compositor != nullptr)
        {
            compositor->launchApplication();
        }
    }

    RdkCompositor::RdkCompositor() : mDisplayName(), mWstContext(NULL), 
        mWidth(1920), mHeight(1080), mPositionX(0), mPositionY(0), mMatrix(), mOpacity(1.0),
        mVisible(true), mAnimating(false), mHolePunch(true), mScaleX(1.0), mScaleY(1.0), mInputListenerTags(RDK_WINDOW_MANAGER_INITIAL_INPUT_LISTENER_TAG), mInputLock(), mInputListeners(),
        mStateChangeListenerTags(RDK_WINDOW_MANAGER_INITIAL_STATE_CHANGE_LISTENER_TAG), mStateChangeLock(), mStateChangeListeners(),
        mApplicationName(), mApplicationThread(), mApplicationState(RdkWindowManager::ApplicationState::Unknown),
        mApplicationPid(-1), mApplicationThreadStarted(false), mApplicationClosedByCompositor(false), mApplicationMutex(), mReceivedKeyPress(false),
        mVirtualDisplayEnabled(false), mVirtualWidth(0), mVirtualHeight(0), mSizeChangeRequestPresent(false), 
        mInputEventsEnabled(true), mSuspendedBeforeStart(false), mFocused(false), mFireboltSurfaces(), mCropX(0), mCropY(0), mCropWidth(0), mCropHeight(0), mOwnerId(-1),
        mRendererEnabled(true), mFirstFrameRendered(false), mApplicationConnectionCount(0)
    {
        if (gForce720)
        {
            RdkWindowManager::Logger::log(LogLevel::Information,  "forcing 720 for rdkc");
            mWidth = 1280;
            mHeight = 720;
        }
        float* matrixPointer = mMatrix;
        float matrix[16] = 
        {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
        
        memcpy(matrixPointer, matrix, sizeof(matrix));
    }
    
    RdkCompositor::~RdkCompositor()
    {
        if ( mWstContext )
        {
            WstCompositorSetInvalidateCallback(mWstContext, NULL, NULL);
            WstCompositorSetClientStatusCallback(mWstContext, NULL, NULL);
            WstCompositorSetDispatchCallback( mWstContext, NULL, NULL);
            closeApplication();
            for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
            {
                WstCompositorDestroy(fireboltSurface->westerosCompositor);
                fireboltSurface->westerosCompositor = NULL;
            }
            WstCompositorDestroy(mWstContext);

            //shutdownApplication();
        }
        mWstContext = NULL;

        mInputListeners.clear();
        mStateChangeListeners.clear();
        mReceivedKeyPress = false;
    }

    void RdkCompositor::invalidate(WstCompositor */*context*/, void *userData)
    {
        RdkCompositor *rdkCompositor= (RdkCompositor*)userData;
        if (rdkCompositor != NULL)
        {
            rdkCompositor->onInvalidate();
        }
    }

    void RdkCompositor::clientStatus(WstCompositor */*context*/, int status, int pid, int detail, void *userData)
    {
        RdkCompositor *rdkCompositor= (RdkCompositor*)userData;
        if (rdkCompositor != NULL)
        {
            rdkCompositor->onClientStatus(status, pid, detail);
        }
    }

    void RdkCompositor::dispatch( WstCompositor *wctx, void *userData )
    {
         RdkCompositor *rdkCompositor= (RdkCompositor*)userData;
         if (rdkCompositor != NULL)
         {
             rdkCompositor->onSizeChangeComplete();
         }
    }

    void RdkCompositor::onInvalidate()
    {
        //todo
    }

    void RdkCompositor::onClientStatus(int status, int pid, int detail)
    {
        if (mApplicationPid < 0)
        {
            if ((status == WstClient_stoppedAbnormal) || (status == WstClient_stoppedNormal))
            {
                mApplicationPid = -1;
            }
            else
            {
                mApplicationPid = pid;
            }
        }
        bool eventFound = true;
        std::string eventName = "";

        switch ( status )
        {
             case WstClient_stoppedNormal:
                 RdkWindowManager::Logger::log(LogLevel::Information,  "client stopped normal");
                 eventName = RDK_WINDOW_MANAGER_EVENT_APPLICATION_TERMINATED;
                 break;
             case WstClient_stoppedAbnormal:
                 RdkWindowManager::Logger::log(LogLevel::Information,  "client stopped abnormal");
                 eventName = RDK_WINDOW_MANAGER_EVENT_APPLICATION_TERMINATED;
                 break;
             case WstClient_connected:
                 RdkWindowManager::Logger::log(LogLevel::Information,  "client connected");
                 eventName = RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED;
                 break;
             case WstClient_disconnected:
                 RdkWindowManager::Logger::log(LogLevel::Information,  "client disconnected");
                 eventName = RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED;
                 break;
             case WstClient_firstFrame:
                 RdkWindowManager::Logger::log(LogLevel::Information,  "client first frame received");
                 eventName = RDK_WINDOW_MANAGER_EVENT_APPLICATION_FIRST_FRAME;
                 break;
             default:
                 RdkWindowManager::Logger::log(LogLevel::Information,  "unknown client status state");
                 eventFound = false;
                 break;
        }

        if (eventFound)
        {
            if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED) == 0)
            {
                if(mApplicationConnectionCount.fetch_add(1) == 0)
                {
                    RdkWindowManager::Logger::log(LogLevel::Information,  "sending event %s", eventName.c_str());
                    CompositorController::onEvent(this, eventName);
                }
            }
            else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED) == 0)
            {
                if(mApplicationConnectionCount.fetch_sub(1) == 1)
                {
                    RdkWindowManager::Logger::log(LogLevel::Information,  "sending event %s", eventName.c_str());
                    CompositorController::onEvent(this, eventName);
                }
            }
            else
            {
                RdkWindowManager::Logger::log(LogLevel::Information,  "sending event %s", eventName.c_str());
                CompositorController::onEvent(this, eventName);
            }
        }
    }

    void RdkCompositor::onSizeChangeComplete()
    {
        if (mSizeChangeRequestPresent)
        {		
            mSizeChangeRequestPresent = false;
            CompositorController::onEvent(this, RDK_WINDOW_MANAGER_EVENT_SIZE_CHANGE_COMPLETE);
        }
    }

    bool RdkCompositor::loadfireboltExtensions(WstCompositor *compositor)
    {
        Logger::log(LogLevel::Information,  "loadfireboltExtensions WstCompositor:%p", compositor);
        bool success = true;

#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
        if (compositor)
        {
            std::vector<std::string> extensions;

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
            extensions.push_back("libwstplugin_rdkwmfireboltsurface.so");
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
            extensions.push_back("libwstplugin_rdkwmfireboltshell.so");
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
            extensions.push_back("libwstplugin_rdkwmfireboltwm.so");
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
            for (int i = 0; i < extensions.size(); ++i)
            {
                const std::string extensionPath = RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY + extensions[i];
                Logger::log(LogLevel::Information,  "Attempting to load extension: %s", extensionPath.c_str());
                if (!WstCompositorAddModule(compositor, extensionPath.c_str()))
                {
                    Logger::log(LogLevel::Warn,  "Failed to load plugin:: %s, westeros error: %s", extensionPath.c_str(), WstCompositorGetLastErrorDetail(compositor));
                }
            }
        }
        else
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */
        {
            success = false;
        }

        return success;
    }

    bool RdkCompositor::loadExtensions(WstCompositor *compositor, const std::string& clientName)
    {
        Logger::log(LogLevel::Information,  "loadExtensions clientName: %s", clientName.c_str());

        bool success = true;
        if (compositor)
        {
            std::vector<std::string> extensions;

            const char* enableRdkWindowManagerExtendedInput = getenv("RDK_WINDOW_MANAGER_EXTENDED_INPUT_ENABLED");
            if (enableRdkWindowManagerExtendedInput)
            {
                std::string extensionInputPath = std::string(RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY) + "libwesteros_plugin_rdkwindowmanager_extended_input.so";
                extensions.push_back(extensionInputPath);
            }

            for (int i = 0; i < extensions.size(); ++i)
            {
                const std::string extensionInputPath = RDK_WINDOW_MANAGER_WESTEROS_PLUGIN_DIRECTORY + extensions[i];
                Logger::log(LogLevel::Information,  "attempting to load extension: %s", extensionInputPath.c_str());
                if (!WstCompositorAddModule(compositor, extensionInputPath.c_str()))
                {
                    Logger::log(LogLevel::Error,  "Failed to load plugin:: %s, westeros error: %s", extensionInputPath.c_str(), WstCompositorGetLastErrorDetail(compositor));
                    success = false;
                }
            }
        }
        else
        {
            success = false;
        }

        return success;
    }

    void RdkCompositor::prepareHolePunchRects(std::vector<WstRect> rects, RdkWindowManagerRect& rect)
    {
        uint32_t x=20000, y=20000, w=0, h=0;
        for (int i=0; i<rects.size(); i++)
        {
            WstRect& wstRect = rects[i];
            x = (wstRect.x < x)?wstRect.x:x;
            y = (wstRect.y < y)?wstRect.y:y;
            int xBoundary = wstRect.x + wstRect.width;
            int yBoundary = wstRect.y + wstRect.height;
            int tempWidth = xBoundary-x;
            int tempHeight = yBoundary-y;
            w = (tempWidth>w)?tempWidth:w;
            h = (tempHeight>h)?tempHeight:h;
        }
        rect.x = x;
        rect.y = y;
        rect.width = w;
        rect.height = h;
        RdkWindowManager::Logger::log(LogLevel::Debug,  "hole punch rectangle: x %d y %d w %d h %d", x, y, w, h);
    }

    void RdkCompositor::draw(bool &needsHolePunch, RdkWindowManagerRect& rect, bool drawOverlays)
    {
        #ifndef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
        if (!mVisible)
        {
            return;
        }
        #endif //!RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
        if (!mRendererEnabled)
        {
            return;
        }

        if (mVirtualDisplayEnabled)
        {
            drawFbo(needsHolePunch, rect, drawOverlays);
        }
        else
        {
            drawDirect(needsHolePunch, rect, drawOverlays);
        }
    }

    void RdkCompositor::drawFbo(bool &needsHolePunch, RdkWindowManagerRect& rect,  bool drawOverlays)
    {
        // create the FBO if it's not created yet or its size was changed
        if (!mFbo ||
            mFbo->width() != mVirtualWidth ||
            mFbo->height() != mVirtualHeight)
        {
            RdkWindowManager::Logger::log(LogLevel::Information,  "creating FBO resolution: %d x %d", mVirtualWidth, mVirtualHeight);
            mFbo = std::make_shared<FrameBuffer>(mVirtualWidth, mVirtualHeight);
        }

        unsigned int outputWidth, outputHeight;
        WstCompositorGetOutputSize(mWstContext, &outputWidth, &outputHeight);
        if ((mVirtualWidth != (uint32_t)outputWidth) || (mVirtualHeight != (uint32_t)outputHeight))
        {
            WstCompositorSetOutputSize(mWstContext, mVirtualWidth, mVirtualHeight);
        }

        mFbo->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glViewport(0, 0, mVirtualWidth, mVirtualHeight);

        int hints = WstHints_none;//WstHints_fboTarget;
        #ifdef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
        if (!mVisible)
        {
            hints |= WstHints_hidden;
        }
        #endif /* RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT */
        std::vector<WstRect> rects;
        float matrix[16] = {
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        };

        if(!drawOverlays)
        {
            if(mFireboltSurfaces.empty())
            {
                if (mCropWidth > 0 || mCropHeight > 0)
                {
                    WstCompositorComposeEmbedded(mWstContext, mCropX, mCropY, mCropWidth, mCropHeight,
                    mMatrix, mOpacity, hints, &needsHolePunch, rects);
                }
                else
                {
                    WstCompositorComposeEmbedded(mWstContext, 0, 0, mVirtualWidth, mVirtualHeight,
                    mMatrix, mOpacity, hints, &needsHolePunch, rects);
                }
            }
            else
            {
                for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
                {
                    if (!fireboltSurface->visible)
                    {
                    #ifdef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
                        hints |= WstHints_hidden;
                    #else
                        continue;
                    #endif /* RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT */
                    }
                    if (fireboltSurface->surfaceType == SurfaceType::Standard || fireboltSurface->surfaceType == SurfaceType::Video)
                    {
                        if(fireboltSurface->westerosCompositor != NULL)
                        {
                            bool clearVideoRegion = (fireboltSurface->surfaceType == SurfaceType::Video);
#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
                            // In bridge mode, attempt to compose video surfaces into the capture path.
                            clearVideoRegion = false;
#endif
                            if (clearVideoRegion)
                            {
                                GLenum error;
                                glEnable( GL_SCISSOR_TEST );
                                glClearColor(0.0f,0.0f,0.0f,0.0f);
                                if (fireboltSurface->swidth > 0 || fireboltSurface->sheight > 0)
                                {
                                    glScissor(fireboltSurface->sx, fireboltSurface->sy, fireboltSurface->swidth, fireboltSurface->sheight);
                                }
                                else
                                {
                                    glScissor(fireboltSurface->x, fireboltSurface->y, fireboltSurface->width, fireboltSurface->height);
                                }
                                error = glGetError();
                                if (error != GL_NO_ERROR)
                                {
                                    Logger::log(LogLevel::Error, "glScissor: glGetError() = %X Co-Ordinates X: %d Y: %d width: %d  height:%d", error,fireboltSurface->x,fireboltSurface->y,fireboltSurface->width,fireboltSurface->height );
                                }
                                glClear(GL_COLOR_BUFFER_BIT);
                                glDisable(GL_SCISSOR_TEST);
                            }
                            else
                            {
                                if (fireboltSurface->swidth > 0 || fireboltSurface->sheight > 0)
                                {
                                    float lMatrix[16];

                                    memcpy(lMatrix, matrix, sizeof(matrix));

                                    /* copy of default matrix*/
                                    lMatrix[0] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->width, fireboltSurface->swidth, 1.f);
                                    lMatrix[5] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->height, fireboltSurface->sheight, 1.f);
                                    lMatrix[12] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->x, fireboltSurface->sx, 0.f);
                                    lMatrix[13] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->y, fireboltSurface->sy, 0.f);

                                    WstCompositorComposeEmbedded(fireboltSurface->westerosCompositor, fireboltSurface->sx, fireboltSurface->sy, fireboltSurface->swidth, fireboltSurface->sheight,
                                    lMatrix, fireboltSurface->opacity, ((fireboltSurface->opacity != 1.f) ? (hints|WstHints_applyTransform) : hints), &needsHolePunch, rects);
                                }
                                else
                                {
                                    WstCompositorComposeEmbedded(fireboltSurface->westerosCompositor, fireboltSurface->x, fireboltSurface->y, fireboltSurface->width, fireboltSurface->height,
                                    matrix, fireboltSurface->opacity, ((fireboltSurface->opacity != 1.f) ? (hints|WstHints_applyTransform) : hints), &needsHolePunch, rects);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if(!mFireboltSurfaces.empty())
            {
                for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
                {
                    if (!fireboltSurface->visible)
                    {
                        #ifdef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
                        hints |= WstHints_hidden;
                        #else
                            continue;
                        #endif /* RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT */
                    }
                    if (fireboltSurface->surfaceType == SurfaceType::Popup || fireboltSurface->surfaceType == SurfaceType::Notification)
                    {
                        if(fireboltSurface->westerosCompositor != NULL)
                        {
                            if (fireboltSurface->swidth > 0 || fireboltSurface->sheight > 0)
                            {
                                float lMatrix[16];

                                memcpy(lMatrix, matrix, sizeof(matrix));

                                /* copy of default matrix*/
                                lMatrix[0] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->width, fireboltSurface->swidth, 1.f);
                                lMatrix[5] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->height, fireboltSurface->sheight, 1.f);
                                lMatrix[12] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->x, fireboltSurface->sx, 0.f);
                                lMatrix[13] = CONVERT_GL_FLOAT_SCALE(fireboltSurface->y, fireboltSurface->sy, 0.f);

                                WstCompositorComposeEmbedded(fireboltSurface->westerosCompositor, fireboltSurface->sx, fireboltSurface->sy, fireboltSurface->swidth, fireboltSurface->sheight,
                                lMatrix, fireboltSurface->opacity, ((fireboltSurface->opacity != 1.f) ? (hints|WstHints_applyTransform) : hints), &needsHolePunch, rects);
                            }
                            else
                            {
                                WstCompositorComposeEmbedded(fireboltSurface->westerosCompositor, 0, 0, fireboltSurface->width, fireboltSurface->height,
                                matrix, fireboltSurface->opacity, ((fireboltSurface->opacity != 1.f) ? (hints|WstHints_applyTransform) : hints), &needsHolePunch, rects);
                            }
                        }
                    }
                }
            }
        }

        if (needsHolePunch)
        {
            prepareHolePunchRects(std::move(rects), rect);
        }

        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        mFbo->unbind();

        uint32_t screenWidth, screenHeight;
        CompositorController::getScreenResolution(screenWidth, screenHeight);

        FrameBufferRenderer::instance()->draw(mFbo, screenWidth, screenHeight, mMatrix,
                                              mPositionX, mPositionY, mWidth, mHeight,
                                              mCropX, mCropY, mCropWidth, mCropHeight, mOpacity);
    }

    void RdkCompositor::drawDirect(bool &needsHolePunch, RdkWindowManagerRect& rect, bool drawOverlays)
    {
        int hints = WstHints_none;
        hints |= WstHints_applyTransform;
        if (mHolePunch)
        {
            hints |= WstHints_holePunch;
        }
        hints |= WstHints_noRotation;
        if (mAnimating)
        {
            hints |= WstHints_animating;
        }
        #ifdef RDK_WINDOW_MANAGER_ENABLE_FORCE_ANIMATE
        hints |= WstHints_animating;
        #endif //RDK_WINDOW_MANAGER_ENABLE_FORCE_ANIMATE
        #ifdef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
        if (!mVisible)
        {
            hints |= WstHints_hidden;
        }
        #endif //RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
        std::vector<WstRect> rects;

        unsigned int width, height;
        WstCompositorGetOutputSize(mWstContext, &width, &height);
        if ( (mWidth != (uint32_t)width) || (mHeight != (uint32_t)height) )
        {
            WstCompositorSetOutputSize(mWstContext, mWidth, mHeight);
        }

        if(!drawOverlays)
        {
            if(mFireboltSurfaces.empty())
            {
                WstCompositorComposeEmbedded( mWstContext, 0, 0, mWidth, mHeight,
                mMatrix, mOpacity, hints, &needsHolePunch, rects );
            }
            else
            {
                for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
                {
                    if (!fireboltSurface->visible)
                    {
                    #ifdef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
                        hints |= WstHints_hidden;
                    #else
                        continue;
                    #endif /* RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT */
                    }
                    if (fireboltSurface->surfaceType == SurfaceType::Standard || fireboltSurface->surfaceType == SurfaceType::Video)
                    {
                        if(fireboltSurface->westerosCompositor != NULL)
                        {
                            bool clearVideoRegion = (fireboltSurface->surfaceType == SurfaceType::Video);
#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
                            // In bridge mode, attempt to compose video surfaces into the capture path.
                            clearVideoRegion = false;
#endif
                            if (clearVideoRegion)
                            {
                                    GLenum error;
                                    glEnable( GL_SCISSOR_TEST );
                                    glClearColor(0.0f,0.0f,0.0f,0.0f);
                                    glScissor(fireboltSurface->x, fireboltSurface->y, fireboltSurface->width, fireboltSurface->height);
                                    error = glGetError();
                                    if (error != GL_NO_ERROR)
                                    {
                                        Logger::log(LogLevel::Error, "glScissor: glGetError() = %X Co-Ordinates X: %d Y: %d width: %d  height:%d", error,fireboltSurface->x,fireboltSurface->y,fireboltSurface->width,fireboltSurface->height );
                                    }
                                    glClear(GL_COLOR_BUFFER_BIT); // Clear with transparency
                                    glDisable(GL_SCISSOR_TEST);
                            }
                            else
                            {
                                WstCompositorComposeEmbedded( fireboltSurface->westerosCompositor, fireboltSurface->x, fireboltSurface->y, fireboltSurface->width, fireboltSurface->height,
                                mMatrix, fireboltSurface->opacity, hints, &needsHolePunch, rects );
                            }
                        }
                    }
                }

            }
        }
        else
        {
            if(!mFireboltSurfaces.empty())
            {
                for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
                {
                    if (!fireboltSurface->visible)
                    {
                    #ifdef RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT
                        hints |= WstHints_hidden;
                    #else
                        continue;
                    #endif /* RDK_WINDOW_MANAGER_ENABLE_HIDDEN_SUPPORT */
                    }
                    if (fireboltSurface->surfaceType == SurfaceType::Popup || fireboltSurface->surfaceType == SurfaceType::Notification )
                    {
                        if(fireboltSurface->westerosCompositor != NULL)
                        {
                                WstCompositorComposeEmbedded( fireboltSurface->westerosCompositor, fireboltSurface->x, fireboltSurface->y, fireboltSurface->width, fireboltSurface->height,
                                mMatrix, fireboltSurface->opacity, hints, &needsHolePunch, rects );
                        }
                    }
                }
            }
        }

        if (needsHolePunch)
        {
            prepareHolePunchRects(std::move(rects), rect);
        }
    }

    void RdkCompositor::processKeyEvent(bool keyPressed, uint32_t keycode, uint32_t flags, uint64_t metadata)
    {
        if (!mInputEventsEnabled)
        {
            RdkWindowManager::Logger::log(LogLevel::Information, "processKeyEvent input event blocked display: %s, keyCode: %d",
                mDisplayName.c_str(), keycode);
            return;
        }

        uint32_t modifiers = 0;

        if ( flags & RDK_WINDOW_MANAGER_FLAGS_SHIFT )
        {
            modifiers |= WstKeyboard_shift;
        }
        if ( flags & RDK_WINDOW_MANAGER_FLAGS_CONTROL )
        {
            modifiers |= WstKeyboard_ctrl;
        }
        if ( flags & RDK_WINDOW_MANAGER_FLAGS_ALT )
        {
            modifiers |= WstKeyboard_alt;
        }

        int32_t waylandKeyCode = (int32_t)keyCodeToWayland(keycode);
        WstCompositorKeyEvent( mWstContext, waylandKeyCode, keyPressed ? WstKeyboard_keyState_depressed : WstKeyboard_keyState_released, (int32_t)modifiers );
    }


    void RdkCompositor::onKeyPress(uint32_t keycode, uint32_t flags, uint64_t metadata)
    {
        processKeyEvent(true, keycode, flags, metadata);
        mReceivedKeyPress = true;
    }

    void RdkCompositor::onKeyRelease(uint32_t keycode, uint32_t flags, uint64_t metadata)
    {
        if (mReceivedKeyPress)
        {
            processKeyEvent(false, keycode, flags, metadata);
        }
        mReceivedKeyPress = false;
    }

    void RdkCompositor::onPointerMotion(uint32_t x, uint32_t y)
    {
        WstCompositorPointerMoveEvent(mWstContext, x, y);
    }

    void RdkCompositor::onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y)
    {
        WstCompositorPointerButtonEvent(mWstContext, keyCode, WstKeyboard_keyState_depressed);
    }

    void RdkCompositor::onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y)
    {
        WstCompositorPointerButtonEvent(mWstContext, keyCode, WstKeyboard_keyState_released);
    }

    void RdkCompositor::setPosition(int32_t x, int32_t y)
    {
        mPositionX = x;
        mPositionY = y;
        mMatrix[12] = static_cast<float>(mPositionX);
        mMatrix[13] = static_cast<float>(mPositionY);
    }

    void RdkCompositor::position(int32_t &x, int32_t &y)
    {
        x = mPositionX;
        y = mPositionY;
    }

    void RdkCompositor::setOpacity(double opacity)
    {
        mOpacity = opacity;
    }

    void RdkCompositor::scale(double &scaleX, double &scaleY)
    {
        scaleX = mScaleX;
        scaleY = mScaleY;
    }

    void RdkCompositor::setScale(double scaleX, double scaleY)
    {
        if (scaleX >= 0)
        {
            mScaleX = scaleX;
        }
        if (scaleY >= 0)
        {
            mScaleY = scaleY;
        }

        mMatrix[0] = 1 * mScaleX;
        mMatrix[5] = 1 * mScaleY;
    }

    void RdkCompositor::setSize(uint32_t width, uint32_t height)
    {
        if (gForce720)
        {
            width = 1280;
            height = 720;
        }
        if ( (mWstContext != NULL) && !mVirtualDisplayEnabled && ((mWidth != width) || (mHeight != height)) )
        {
            mSizeChangeRequestPresent = true;
            WstCompositorSetOutputSize(mWstContext, width, height);
        }
        mWidth = width;
        mHeight = height;
    }

    void RdkCompositor::size(uint32_t &width, uint32_t &height)
    {
        width = mWidth;
        height = mHeight;
    }

    void RdkCompositor::opacity(double& opacity)
    {
        opacity = mOpacity;
    }

    void RdkCompositor::setVisible(bool visible)
    {
        if (visible && (RdkWindowManager::ApplicationState::Suspended == RdkCompositor::getApplicationState()))
        {
            Logger::log(LogLevel::Information,  "application not made visible because of suspended state");
            return;
        }

        /* Send onVisible event if new is true and old was false,
            send onHidden event if new is false and old was true */
        if (visible != mVisible)
        {
            if (visible)
            {
                Logger::log(LogLevel::Information, "sending onVisible event");
                CompositorController::onEvent(this, RDK_WINDOW_MANAGER_EVENT_APPLICATION_VISIBLE);
            }
            else
            {
                Logger::log(LogLevel::Information, "sending onHidden event");
                CompositorController::onEvent(this, RDK_WINDOW_MANAGER_EVENT_APPLICATION_HIDDEN);
            }
            mVisible = visible;
            updateWaylandState();
        }
    }
    
    void RdkCompositor::visible(bool &visible)
    {
        visible = mVisible;
    }

    void RdkCompositor::setAnimating(bool animating)
    {
        mAnimating = animating;
    }

    void RdkCompositor::setHolePunch(bool holePunchEnabled)
    {
        mHolePunch = holePunchEnabled;
    }

    void RdkCompositor::holePunch(bool &holePunchEnabled)
    {
        holePunchEnabled = mHolePunch;
    }

    void RdkCompositor::setCrop(int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight)
    {
        mCropX = cropX;
        mCropY = cropY;
        mCropWidth = cropWidth;
        mCropHeight = cropHeight;

        if (cropWidth > 0 || cropHeight > 0)
        {
            Logger::log(LogLevel::Information,  "setCrop cropX:%d cropY:%d cropWidth:%d cropHeight:%d", cropX, cropY, cropWidth, cropHeight);

            mMatrix[0] = CONVERT_GL_FLOAT_SCALE(mWidth, cropWidth, 1.f);
            mMatrix[5] = CONVERT_GL_FLOAT_SCALE(mHeight, cropHeight, 1.f);
            mMatrix[12] = CONVERT_GL_FLOAT_SCALE(mPositionX, cropX, 0.f);
            mMatrix[13] = CONVERT_GL_FLOAT_SCALE(mPositionY, cropY, 0.f);
        }
        else
        {
            Logger::log(LogLevel::Information,  "setCrop mWidth:%d mHeight:%d mPositionX:%d mPositionY:%d", mWidth, mHeight, mPositionX, mPositionY);

            mMatrix[0] = 1.f;
            mMatrix[5] = 1.f;
            mMatrix[12] = 0.f;
            mMatrix[13] = 0.f;
        }
    }

    bool RdkCompositor::setOwner(int32_t ownerId, int32_t groupId)
    {
        bool status = false;

        if (mDisplayName.empty())
        {
            Logger::log(LogLevel::Error,"display name is empty");
        }
        else if (mOwnerId == ownerId)
        {
            status = true;
        }
        else
        {
            const char* runtimeDir = getenv("XDG_RUNTIME_DIR");

            if (NULL != runtimeDir)
            {
                std::string displaySocket = std::string(runtimeDir) + "/" + mDisplayName;

                Logger::log(LogLevel::Information,"change owner of %s with ownerId : %d", displaySocket.c_str(), ownerId);
                // POSIX standard: gid value of -1 means "do not change group ownership"
                // Coverity flags this as INTEGER_OVERFLOW, but it's documented behavior
                // coverity[INTEGER_OVERFLOW : FALSE]
                if (0 != chown(displaySocket.c_str(), ownerId, (groupId>0)?groupId:static_cast<gid_t>(-1)))
                {
                    Logger::log(LogLevel::Error,"failed to change ownership for ownerId : %d errno: %s", ownerId, strerror(errno));
                }
                else
                {
                    mOwnerId = ownerId;
                    status = true;
                }
            }
            else
            {
                Logger::log(LogLevel::Error,"failed to get runtime directory");
            }
        }

        return status;
    }

    void RdkCompositor::crop(int32_t &cropX, int32_t &cropY, int32_t &cropWidth, int32_t &cropHeight)
    {
        cropX = mCropX;
        cropY = mCropY;
        cropWidth = mCropWidth;
        cropHeight = mCropHeight;
    }

    void RdkCompositor::ownerId(int32_t& ownerId)
    {
        ownerId = mOwnerId;
    }

    int RdkCompositor::registerInputEventListener(std::function<void(const RdkWindowManager::InputEvent&)> listener)
    {
        std::lock_guard<std::mutex> locker(mInputLock);
        const int tag = mInputListenerTags++;
        mInputListeners.emplace(tag, std::move(listener));
        return tag;
    }

    void RdkCompositor::unregisterInputEventListener(int tag)
    {
        std::lock_guard<std::mutex> locker(mInputLock);
        mInputListeners.erase(tag);
    }

    void RdkCompositor::broadcastInputEvent(const RdkWindowManager::InputEvent &inputEvent)
    {
        RdkWindowManager::Logger::log(LogLevel::Information,  "sending input metadata for device: %d", inputEvent.deviceId);
        std::lock_guard<std::mutex> locker(mInputLock);
        for (const auto &listener : mInputListeners)
        {
            if (listener.second)
                listener.second(inputEvent);
        }
    }

    int RdkCompositor::registerStateChangeEventListener(std::function<void(uint32_t)> listener)
    {
        std::lock_guard<std::mutex> locker(mStateChangeLock);
        if (true == mSuspendedBeforeStart)
        {
           // suspendApplication();
        }
        const int tag = mStateChangeListenerTags++;
        if (true == mSuspendedBeforeStart)
        {
            if (listener)
            {
               listener(3);
            }
            mSuspendedBeforeStart = false;
        }
        mStateChangeListeners.emplace(tag, std::move(listener));
        return tag;
    }

    void RdkCompositor::unregisterStateChangeEventListener(int tag)
    {
        std::lock_guard<std::mutex> locker(mStateChangeLock);
        mStateChangeListeners.erase(tag);
    }

    void RdkCompositor::broadcastStateChangeEvent(uint32_t state)
    {
        Logger::log(LogLevel::Information, "sending state event %d for %s", state, mDisplayName.c_str());
        // std::vector<std::map<std::string, RdkWindowManagerData>> eventData(1);
        // eventData[0] = std::map<std::string, RdkWindowManagerData>();
        // eventData[0]["state"] = state;
        // eventData[0]["display"] = mDisplayName;
        // CompositorController::sendEvent(RDK_WINDOW_MANAGER_EVENT_APPLICATION_STATE_CHANGED, eventData);
        std::lock_guard<std::mutex> locker(mStateChangeLock);
        for (const auto &listener : mStateChangeListeners)
        {
            if (listener.second)
                listener.second(state);
        }
    }

    void RdkCompositor::displayName(std::string& name) const
    {
        name = mDisplayName;
    }

    void RdkCompositor::launchApplicationInBackground()
    {
        mApplicationThreadStarted = true;
        mApplicationThread = std::thread{launchApplicationThreadCallback, this};
    }

    void RdkCompositor::launchApplication()
    {
        std::string applicationName;
        {
          std::lock_guard<std::recursive_mutex> lock{mApplicationMutex};
          mApplicationState = RdkWindowManager::ApplicationState::Running;
          applicationName = mApplicationName;
        }
        if (!WstCompositorLaunchClient(mWstContext, applicationName.c_str()))
        {
            RdkWindowManager::Logger::log(LogLevel::Information,  "RdkCompositor failed to launch %s", applicationName.c_str());
            const char *detail = WstCompositorGetLastErrorDetail( mWstContext );
            RdkWindowManager::Logger::log(LogLevel::Information,  "westeros error: %s", detail);
        }
        RdkWindowManager::Logger::log(LogLevel::Information,  "application close: %s", applicationName.c_str());
        {
            std::lock_guard<std::recursive_mutex> lock{mApplicationMutex};
            mApplicationState = RdkWindowManager::ApplicationState::Running;
        }
    }

    void RdkCompositor::closeApplication()
    {
        {
            std::lock_guard<std::recursive_mutex> lock{mApplicationMutex};
            if (mApplicationPid > 0 &&
                mApplicationState != RdkWindowManager::ApplicationState::Stopped &&
                mApplicationState != RdkWindowManager::ApplicationState::Unknown)
            {
                RdkWindowManager::Logger::log(LogLevel::Information,  "about to terminate process id %d", mApplicationPid);
                kill( mApplicationPid, SIGKILL);
                RdkWindowManager::Logger::log(LogLevel::Information,  "process with id %d has been terminated", mApplicationPid);
                mApplicationPid = 0;
                mApplicationState = RdkWindowManager::ApplicationState::Stopped;
            }
        }
        if (mApplicationThreadStarted)
        {
            mApplicationThread.join();
        }
    }

    void RdkCompositor::shutdownApplication()
    {
        std::lock_guard<std::recursive_mutex> lock{mApplicationMutex};
        if (mApplicationClosedByCompositor && (mApplicationPid > 0) && (0 == kill(mApplicationPid, 0)))
        {
            RdkWindowManager::Logger::log(LogLevel::Information,  "sending SIGKILL to application with pid %d", mApplicationPid);
            kill(mApplicationPid, SIGKILL);
            mApplicationClosedByCompositor = false;
        }
        mApplicationPid= -1;
    }

    void RdkCompositor::setApplication(const std::string& application)
    {
        mApplicationName = application;
    }

    RdkWindowManager::ApplicationState RdkCompositor::getApplicationState(void)
    {
        std::lock_guard<std::recursive_mutex> lock{mApplicationMutex};
        return mApplicationState;
    }

    bool RdkCompositor::isKeyPressed()
    {
        return mReceivedKeyPress;
    }

    void RdkCompositor::getVirtualResolution(uint32_t &virtualWidth, uint32_t &virtualHeight)
    {
        virtualWidth = mVirtualWidth;
        virtualHeight = mVirtualHeight;
    }

    void RdkCompositor::setVirtualResolution(uint32_t virtualWidth, uint32_t virtualHeight)
    {
        mVirtualWidth = (virtualWidth > 0) ? virtualWidth : mWidth;
        mVirtualHeight = (virtualHeight > 0) ? virtualHeight : mHeight;
    }

    void RdkCompositor::enableVirtualDisplay(bool enable)
    {
        mVirtualDisplayEnabled = enable;   
    }

    bool RdkCompositor::getVirtualDisplayEnabled()
    {
        return mVirtualDisplayEnabled;
    }

    void RdkCompositor::enableInputEvents(bool enable)
    {
        Logger::log(LogLevel::Information, "enableInputEvents display: %s, oldVal: %d, newVal: %d",
            mDisplayName.c_str(), mInputEventsEnabled, enable);
        mInputEventsEnabled = enable;
    }

    bool RdkCompositor::getInputEventsEnabled() const
    {
        return mInputEventsEnabled;
    }

    void RdkCompositor::updateWaylandState()
    {
        const uint32_t _ACTIVE = 0;
        const uint32_t _INACTIVE = 1;
        const uint32_t _HIDDEN = 2;
        const uint32_t _SUSPENDED = 3;

        std::lock_guard<std::recursive_mutex> lock{mApplicationMutex};
        if (mApplicationState == ApplicationState::Suspended)
        {
            broadcastStateChangeEvent(_SUSPENDED);
        }
        else if (!mVisible)
        {
            broadcastStateChangeEvent(_HIDDEN);
        }
        else if (mFocused)
        {
            broadcastStateChangeEvent(_ACTIVE);
        }
        else
        {
            broadcastStateChangeEvent(_INACTIVE);
        }
    }

    void RdkCompositor::setFocused(bool focused)
    {
        /* Send onFocus event if new is true and old was false,
            send onBlur event if new is false and old was true */
        if (focused != mFocused)
        {
            if (focused)
            {
                Logger::log(LogLevel::Information, "sending onFocus event");
                CompositorController::onEvent(this, RDK_WINDOW_MANAGER_EVENT_APPLICATION_FOCUS);
            }
            else
            {
                Logger::log(LogLevel::Information, "sending onBlur event");
                CompositorController::onEvent(this, RDK_WINDOW_MANAGER_EVENT_APPLICATION_BLUR);
            }
        }
        mFocused = focused;
        updateWaylandState();
    }

    bool RdkCompositor::convertToFireboltSurface(int surfaceId, SurfaceType surfaceType)
    {
        bool result = true;
        FireboltSurfaceInfo surfaceInfo;
        surfaceInfo.surfaceId = surfaceId;
        surfaceInfo.surfaceType = surfaceType;
        std::vector<int> surfaceIds;

        if(mFireboltSurfaces.empty())
        {
            WstCompositorGetSurfaceIds(mWstContext, surfaceIds);

            for (std::vector<int>::iterator id = surfaceIds.begin(); id != surfaceIds.end(); id++)
            {
                if((surfaceType == SurfaceType::Notification || surfaceType == SurfaceType::Popup) && *id == surfaceId)
                {
                    WstCompositor* overlayCompositor = NULL;
                    overlayCompositor = WstCompositorCreateVirtualEmbedded(mWstContext);
                    result = WstCompositorVirtualEmbeddedSetSurfaceOwner(overlayCompositor, surfaceId );
                    surfaceInfo.westerosCompositor = overlayCompositor;
                    mFireboltSurfaces.push_back(surfaceInfo);
                }
                else
                {
                    WstCompositor* westerosCompositor = NULL;
                    westerosCompositor = WstCompositorCreateVirtualEmbedded(mWstContext);
                    result = WstCompositorVirtualEmbeddedSetSurfaceOwner( westerosCompositor, surfaceId );
                    FireboltSurfaceInfo mainSurfaceInfo;
                    mainSurfaceInfo.surfaceId = surfaceId;
                    mainSurfaceInfo.surfaceType = surfaceType;
                    mainSurfaceInfo.westerosCompositor = westerosCompositor;
                    mFireboltSurfaces.push_back(mainSurfaceInfo);
                }
            }
        }
        else
        {
            for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
            {
                if (fireboltSurface->surfaceId == surfaceId)
                {
                    fireboltSurface->surfaceType = surfaceType;
                    return true;
                }
            }

            WstCompositor* westerosCompositor = NULL;
            westerosCompositor = WstCompositorCreateVirtualEmbedded(mWstContext);
            result = WstCompositorVirtualEmbeddedSetSurfaceOwner( westerosCompositor, surfaceId );
            if (result)
            {
                surfaceInfo.westerosCompositor = westerosCompositor;
                for (auto reverseIterator = mFireboltSurfaces.rbegin(); reverseIterator != mFireboltSurfaces.rend(); reverseIterator++)
                {
                    if((surfaceType == SurfaceType::Notification || surfaceType == SurfaceType::Popup) &&
                     (reverseIterator->surfaceType == SurfaceType::Notification || reverseIterator->surfaceType == SurfaceType::Popup))
                    {
                        surfaceInfo.zOrder = reverseIterator->zOrder + 1;
                        break;
                    }
                    else if ((surfaceType == SurfaceType::Standard || surfaceType == SurfaceType::Video) &&
                     (reverseIterator->surfaceType == SurfaceType::Standard || reverseIterator->surfaceType == SurfaceType::Video))
                    {
                        surfaceInfo.zOrder = reverseIterator->zOrder + 1;
                        break;
                    }
                }
                std::vector<FireboltSurfaceInfo>::iterator fireboltSurfaceIt;
                for (fireboltSurfaceIt = mFireboltSurfaces.begin(); fireboltSurfaceIt != mFireboltSurfaces.end(); ++fireboltSurfaceIt)
                {
                    if (fireboltSurfaceIt->zOrder > surfaceInfo.zOrder)
                    {
                        break;
                    }
                }
                if (fireboltSurfaceIt == mFireboltSurfaces.end())
                {
                    mFireboltSurfaces.push_back(surfaceInfo);
                }
                else
                {
                    mFireboltSurfaces.insert(fireboltSurfaceIt, surfaceInfo);
                }
            }
            else
            {
                WstCompositorDestroy(westerosCompositor);
            }
        }
        return result;
    }

    bool RdkCompositor::hasCompositor(WstCompositor* compositor)
    {
        if (mWstContext == compositor) {
            return true;
        }

        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->westerosCompositor == compositor)
            {
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::hasOverlays()
    {
        if(mFireboltSurfaces.empty())
        {
            return false;
        }
        else
        {
            for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
            {
                if (fireboltSurface->surfaceType == SurfaceType::Popup || fireboltSurface->surfaceType == SurfaceType::Notification )
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool RdkCompositor::setFireboltSurfaceOpacity(int surfaceId, double opacity)
    {
        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->surfaceId == surfaceId )
            {
                fireboltSurface->opacity = opacity;
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::setFireboltSurfaceBounds(int surfaceId, int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->surfaceId == surfaceId )
            {
                fireboltSurface->x = x;
                fireboltSurface->y = y;
                fireboltSurface->width = width;
                fireboltSurface->height = height;
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::setFireboltSurfaceCrop(int surfaceId, int32_t sx, int32_t sy, uint32_t swidth, uint32_t sheight)
    {
        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->surfaceId == surfaceId )
            {
                fireboltSurface->sx = sx;
                fireboltSurface->sy = sy;
                fireboltSurface->swidth = swidth;
                fireboltSurface->sheight = sheight;
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::setFireboltSurfaceVisibility(int surfaceId, bool visible)
    {
        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->surfaceId == surfaceId )
            {
                fireboltSurface->visible = visible;
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::setFireboltSurfaceZOrder(int surfaceId, int zOrder)
    {
        FireboltSurfaceInfo tempFireboltSurface;
        bool surfaceFound = false;
        std::vector<FireboltSurfaceInfo>::iterator fireboltSurfaceIt;

        for (fireboltSurfaceIt = mFireboltSurfaces.begin(); fireboltSurfaceIt != mFireboltSurfaces.end(); ++fireboltSurfaceIt)
        {
            if (fireboltSurfaceIt->surfaceId == surfaceId )
            {
                tempFireboltSurface = *fireboltSurfaceIt;
                mFireboltSurfaces.erase(fireboltSurfaceIt);
                tempFireboltSurface.zOrder = zOrder;
                surfaceFound = true;
                break;
            }
        }
        if (!surfaceFound)
        {
            return false;
        }

        for (fireboltSurfaceIt = mFireboltSurfaces.begin(); fireboltSurfaceIt != mFireboltSurfaces.end(); ++fireboltSurfaceIt)
        {
            if (fireboltSurfaceIt->zOrder > zOrder)
            {
                break;
            }
        }
        if (fireboltSurfaceIt == mFireboltSurfaces.end())
        {
            mFireboltSurfaces.push_back(tempFireboltSurface);
        }
        else
        {
            mFireboltSurfaces.insert(fireboltSurfaceIt, tempFireboltSurface);
        }
        return true;
    }

    bool RdkCompositor::setFireboltSurfaceName(int surfaceId, const std::string& surfaceName)
    {
        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->surfaceId == surfaceId )
            {
                fireboltSurface->name = surfaceName;
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::fireboltSurfaceDestroy(int surfaceId)
    {
        std::vector<FireboltSurfaceInfo>::iterator fireboltSurfaceIt;

        for (fireboltSurfaceIt = mFireboltSurfaces.begin(); fireboltSurfaceIt != mFireboltSurfaces.end(); ++fireboltSurfaceIt)
        {
            if (fireboltSurfaceIt->surfaceId == surfaceId )
            {
                mFireboltSurfaces.erase(fireboltSurfaceIt);
                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::getSurfaceInfo(int surfaceId, FireboltSurfaceInfo& surfaceInfo)
    {
        for (std::vector<FireboltSurfaceInfo>::iterator fireboltSurface = mFireboltSurfaces.begin(); fireboltSurface != mFireboltSurfaces.end(); fireboltSurface++)
        {
            if (fireboltSurface->surfaceId == surfaceId)
            {
                surfaceInfo.opacity = fireboltSurface->opacity;
                surfaceInfo.x = fireboltSurface->x;
                surfaceInfo.y = fireboltSurface->y;
                surfaceInfo.width = fireboltSurface->width;
                surfaceInfo.height = fireboltSurface->height;
                surfaceInfo.sx = fireboltSurface->sx;
                surfaceInfo.sy = fireboltSurface->sy;
                surfaceInfo.swidth = fireboltSurface->swidth;
                surfaceInfo.sheight = fireboltSurface->sheight;
                surfaceInfo.visible = fireboltSurface->visible;
                surfaceInfo.zOrder = fireboltSurface->zOrder;
                surfaceInfo.name = fireboltSurface->name;
                surfaceInfo.surfaceType= fireboltSurface->surfaceType;
                surfaceInfo.westerosCompositor = fireboltSurface->westerosCompositor;

                return true;
            }
        }
        return false;
    }

    bool RdkCompositor::enableDisplayRender(bool enable)
    {
        if (mRendererEnabled != enable)
        {
            mRendererEnabled = enable;

            if(enable == false)
            {
                WstCompositorResetFirstFrame(mWstContext);
                mFirstFrameRendered = false;
            }
        }
        return true;
    }

    void RdkCompositor::setFirstFrameRendered(bool enable)
    {
        mFirstFrameRendered = enable;
    }

    bool RdkCompositor::renderReady()
    {
        return mFirstFrameRendered;
    }
}
