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

#ifndef RDK_WINDOW_MANAGER_RDK_COMPOSITOR_H
#define RDK_WINDOW_MANAGER_RDK_COMPOSITOR_H

#include <string>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <memory>
#include "westeros-compositor.h"
#include "inputevent.h"
#include "application.h"
#include "rdkwindowmanagerrect.h"
#include "rdkwindowmanagertypes.h"

namespace RdkWindowManager
{

    struct FireboltSurfaceInfo
    {
        WstCompositor* westerosCompositor;
        int surfaceId;
        SurfaceType surfaceType;
        int32_t x;
        int32_t y;
        uint32_t width;
        uint32_t height;
        int32_t sx;
        int32_t sy;
        uint32_t swidth;
        uint32_t sheight;
        double opacity;
        bool visible;
        int zOrder;
        std::string name;
        FireboltSurfaceInfo():westerosCompositor(NULL),surfaceId(0),surfaceType(SurfaceType::Standard),x(0),y(0),
            width(1920),height(1080),sx(0),sy(0),swidth(1920),sheight(1080),opacity(1.0),visible(true),zOrder(0),name(){}
    };

    class FrameBuffer;

    class RdkCompositor
    {
        public:

            RdkCompositor();
            virtual ~RdkCompositor();
            virtual bool createDisplay(const std::string& displayName, const std::string& clientName,
                uint32_t width, uint32_t height, bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight) = 0;
            void draw(bool &needsHolePunch, RdkWindowManagerRect& rect, bool drawOverlays);
            void onKeyPress(uint32_t keycode, uint32_t flags, uint64_t metadata);
            void onKeyRelease(uint32_t keycode, uint32_t flags, uint64_t metadata);
            void onPointerMotion(uint32_t x, uint32_t y);
            void onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y);
            void onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y);
            void setPosition(int32_t x, int32_t y);
            void position(int32_t &x, int32_t &y);
            void setSize(uint32_t width, uint32_t height);
            void size(uint32_t &width, uint32_t &height);
            void setOpacity(double opacity);
            void scale(double &scaleX, double &scaleY);
            void setScale(double scaleX, double scaleY);
            void opacity(double& opacity);
            void setVisible(bool visible);
            void visible(bool &visible);
            void setAnimating(bool animating);
            void setHolePunch(bool holePunchEnabled);
            void holePunch(bool &holePunchEnabled);
            void setCrop(int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight);
            void crop(int32_t &cropX, int32_t &cropY, int32_t &cropWidth, int32_t &cropHeight);
            void keyMetadataEnabled(bool &enabled);
            void setKeyMetadataEnabled(bool enable);
            int registerInputEventListener(std::function<void(const RdkWindowManager::InputEvent&)> listener);
            void unregisterInputEventListener(int tag);
            int registerStateChangeEventListener(std::function<void(uint32_t)> listener);
            void unregisterStateChangeEventListener(int tag);
            void displayName(std::string& name) const;
            void closeApplication();
            void launchApplication();
            void setApplication(const std::string& application);
            bool isKeyPressed();
            void getVirtualResolution(uint32_t &virtualWidth, uint32_t &virtualHeight);
            void setVirtualResolution(uint32_t virtualWidth, uint32_t virtualHeight);
            void enableVirtualDisplay(bool enable);
            bool getVirtualDisplayEnabled();
            void enableInputEvents(bool enable);
            bool getInputEventsEnabled() const;
            void setFocused(bool focused);
            bool convertToFireboltSurface(int surfaceId, SurfaceType surfaceType);
            bool setFireboltSurfaceZOrder(int surfaceId, int zOrder);
            bool setFireboltSurfaceOpacity(int surfaceId, double opacity);
            bool setFireboltSurfaceBounds(int surfaceId, int32_t x, int32_t y, uint32_t width, uint32_t height);
            bool setFireboltSurfaceCrop(int surfaceId, int32_t sx, int32_t sy, uint32_t swidth, uint32_t sheight);
            bool setFireboltSurfaceVisibility(int surfaceId, bool visible);
            bool setFireboltSurfaceName(int surfaceId, const std::string& surfaceName);
            bool fireboltSurfaceDestroy(int surfaceId);
            bool hasOverlays();
            bool hasCompositor(WstCompositor* compositor);

        private:
            void prepareHolePunchRects(std::vector<WstRect> wstrects, RdkWindowManagerRect& rect);
        protected:
            static void invalidate(WstCompositor *context, void *userData);
            static void clientStatus(WstCompositor *context, int status, int pid, int detail, void *userData);
            static void dispatch( WstCompositor *wctx, void *userData );
            void onInvalidate();
            void onClientStatus(int status, int pid, int detail);
            void onSizeChangeComplete();
            void processKeyEvent(bool keyPressed, uint32_t keycode, uint32_t flags, uint64_t metadata);
            void broadcastInputEvent(const RdkWindowManager::InputEvent &inputEvent);
            void broadcastStateChangeEvent(uint32_t state);
            void launchApplicationInBackground();
            void shutdownApplication();
            static bool loadExtensions(WstCompositor *compositor, const std::string& clientName);
            static bool loadfireboltExtensions(WstCompositor *compositor);
            void drawDirect(bool &needsHolePunch, RdkWindowManagerRect& rect, bool drawOverlays);
            void drawFbo(bool &needsHolePunch, RdkWindowManagerRect& rect, bool drawOverlays);
            void updateWaylandState();
            
            std::string mDisplayName;
            WstCompositor *mWstContext;
            uint32_t mWidth;
            uint32_t mHeight;
            int32_t mPositionX;
            int32_t mPositionY;
            float mMatrix[16];
            double mOpacity;
            bool mVisible;
            bool mAnimating;
            bool mHolePunch;
            double mScaleX;
            double mScaleY;
            int32_t mCropX;
            int32_t mCropY;
            int32_t mCropWidth;
            int32_t mCropHeight;
            bool mEnableKeyMetadata;
            int mInputListenerTags;
            std::mutex mInputLock;
            std::unordered_map<int, std::function<void(const RdkWindowManager::InputEvent&)>> mInputListeners;
            int mStateChangeListenerTags;
            std::mutex mStateChangeLock;
            std::unordered_map<int, std::function<void(uint32_t)>> mStateChangeListeners;
            std::string mApplicationName;
            std::thread mApplicationThread;
            RdkWindowManager::ApplicationState mApplicationState;
            int32_t mApplicationPid;
            bool mApplicationThreadStarted;
            bool mApplicationClosedByCompositor;
            std::recursive_mutex mApplicationMutex;
            bool mReceivedKeyPress;
            bool mVirtualDisplayEnabled;
            uint32_t mVirtualWidth;
            uint32_t mVirtualHeight;
            std::shared_ptr<FrameBuffer> mFbo;
            bool mSizeChangeRequestPresent;
            bool mInputEventsEnabled;
            bool mSuspendedBeforeStart;
            bool mFocused;
            std::vector<FireboltSurfaceInfo> mFireboltSurfaces;
    };
}

#endif //RDK_WINDOW_MANAGER_RDK_COMPOSITOR_H
