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

#include "compositorcontroller.h"

#include "essosinstance.h"
#include "rdkwindowmanager.h"
#include "application.h"
#include "logger.h"
#include "linuxkeys.h"
#include "rdkcompositornested.h"
#include "string.h"
#include "rdkwindowmanagerimage.h"
#include "rdkwindowmanagerrect.h"
#include "cursor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <ctime>
#include <mutex>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef RDK_WINDOW_MANAGER_VNC_SERVER
#include "VncServer.h"
#include "src/VncServer/VncServerFactory.h"
#include "VncFrameBuffer.h"
#endif /* RDK_WINDOW_MANAGER_VNC_SERVER */

#define RDK_WINDOW_MANAGER_ANY_KEY 65536
#define RDK_WINDOW_MANAGER_DEFAULT_INACTIVITY_TIMEOUT_IN_SECONDS 15*60
#define RDK_WINDOW_MANAGER_WILDCARD_KEY_CODE 255
#define RDK_WINDOW_MANAGER_WATERMARK_ID 65536

namespace RdkWindowManager
{
    struct KeyListenerInfo
    {
        KeyListenerInfo() : keyCode(-1), flags(0), activate(false), propagate(true) {}
        uint32_t keyCode;
        uint32_t flags;
        bool activate;
        bool propagate;
    };

    struct CompositorInfo
    {
        CompositorInfo() : name(), compositor(nullptr), eventListeners(), mimeType(), zorder(-1), isSuspended(false), previousWidth(0), previousHeight(0), capabilities() {}
        std::string name;
        std::shared_ptr<RdkCompositor> compositor;
        std::map<uint32_t, std::vector<KeyListenerInfo>> keyListenerInfo;
        std::vector<std::shared_ptr<RdkWindowManagerEventListener>> eventListeners;
        std::string mimeType;
        int32_t zorder;
        bool isSuspended;
        uint32_t previousWidth;
        uint32_t previousHeight;
        std::string capabilities;
    };

    struct KeyInterceptInfo
    {
        KeyInterceptInfo() : keyCode(-1), flags(0), focusOnly(false), propagate(false), compositorInfo() {}
        uint32_t keyCode;
        uint32_t flags;
        bool focusOnly;
        bool propagate;
        struct CompositorInfo compositorInfo;
    };

    enum RdkWindowManagerCompositorType
    {
        NESTED,
        SURFACE
    };

    struct WatermarkImage
    {
        WatermarkImage(uint32_t imageId, uint32_t imageZOrder): id(imageId), zorder(imageZOrder), image(nullptr) {}
        uint32_t id;
        uint32_t zorder;
        std::shared_ptr<RdkWindowManager::Image> image;
    };

    struct KeyRepeatConfig
    {
        KeyRepeatConfig() : enabled(false), initialDelay(500), repeatInterval(250) {}
        int initialDelay;
        int repeatInterval;
        bool enabled;
    };

    struct GenerateKeyEvent
    {
        GenerateKeyEvent(const std::string& client, uint32_t keyCode, uint32_t modifiers, double triggerTime) :
            client(client), triggerTime(triggerTime), keyCode(keyCode), modifiers(modifiers) {}
        std::string client;
        double triggerTime;
        uint32_t keyCode;
        uint32_t modifiers;
    };

    typedef std::vector<CompositorInfo> CompositorList;
    typedef CompositorList::iterator CompositorListIterator;

    static bool addCompositor(CompositorList* compositorList, CompositorInfo compositorInfo);

    CompositorList gCompositorList;
    CompositorList gTopmostCompositorList;
    CompositorInfo gFocusedCompositor;
    std::vector<std::shared_ptr<RdkCompositor>> gPendingKeyUpListeners;
    CompositorList gDeletedCompositors;
    std::map<std::string, std::string> gClientAliasMap;
    std::mutex gClientAliasMapMutex;

    static std::map<uint32_t, std::vector<KeyInterceptInfo>> gKeyInterceptInfoMap;
    std::map<std::string, bool> gKeyInterceptedMap;

    bool gEnableInactivityReporting = false;
    double gInactivityIntervalInSeconds = RDK_WINDOW_MANAGER_DEFAULT_INACTIVITY_TIMEOUT_IN_SECONDS;
    double gLastKeyEventTime = RdkWindowManager::seconds();
    double gNextInactiveEventTime = RdkWindowManager::seconds() + gInactivityIntervalInSeconds;
    uint32_t gLastKeyCode = 0;
    uint32_t gLastKeyModifiers = 0;
    uint64_t gLastKeyMetadata = 0;
    std::shared_ptr<RdkWindowManagerEventListener> gRdkWindowManagerEventListener;
    double gLastKeyPressStartTime = 0.0;
    double gLastKeyRepeatTime = 0.0;
    RdkWindowManagerCompositorType gRdkWindowManagerCompositorType = NESTED;
    bool gIgnoreKeyInputEnabled = false;

#ifdef RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN
    std::shared_ptr<RdkWindowManager::Image> gSplashImage = nullptr;
    bool gShowSplashImage = false;
    uint32_t gSplashDisplayTimeInSeconds = 0;
    double gSplashStartTime = 0;
#endif // RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN

    std::shared_ptr<Cursor> gCursor = nullptr;
    KeyRepeatConfig gKeyRepeatConfig;

    // Global hole-punch state: a single rect that applies to all compositors
    // when global hole-punch is enabled (used when textured_video is not set).
    std::mutex            gGlobalHolePunchMutex;
    bool                  gGlobalHolePunchEnabled = false;
    RdkWindowManagerRect  gGlobalHolePunchRect;

    // Z-order threshold separating app-tier compositors (players, apps)
    // from overlay-tier compositors (subtitles z=1000, watermark z=1001).
    // The global hole punch must be applied between these two tiers.
    static constexpr int32_t kOverlayZOrderThreshold = 1000;
    std::vector<GenerateKeyEvent> gGenerateKeyEvents;
    std::unordered_map<std::string, std::shared_ptr<FireboltExtensionEventListener>> gfbExtensionEventListenerMap;
    std::mutex gFireboltExtensionListenerMapMutex;
    const std::unordered_map<std::string, std::string> gFireboltExtensionEventMap = {
            { RDK_WINDOW_MANAGER_EVENT_APPLICATION_FOCUS, RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_FOCUS },
            { RDK_WINDOW_MANAGER_EVENT_APPLICATION_BLUR, RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_BLUR },
            { RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED, RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_CONNECTED},
            { RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED, RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_DISCONNECTED}
        };
    std::unordered_map<int, std::shared_ptr<ExtensionEventListener>> gExtensionEventListenerMap;
    std::mutex gExtensionListenerMapMutex;
    int gExtensionEventListenerTag = 0;
    const std::unordered_map<std::string, std::string> gExtensionEventMap = {
            { RDK_WINDOW_MANAGER_EXTENSION_EVENT_CLIENT_CONFIG_CHANGED, RDK_WINDOW_MANAGER_EXTENSION_EVENT_CLIENT_CONFIG_CHANGED },
            { RDK_WINDOW_MANAGER_EXTENSION_EVENT_OWNER_CHANGED, RDK_WINDOW_MANAGER_EXTENSION_EVENT_OWNER_CHANGED }
        };

#ifdef RDK_WINDOW_MANAGER_VNC_SERVER
    static bool gVncServerEnabled = false;
    static std::shared_ptr<VncFrameBuffer> gVncBuffer;
#endif /* RDK_WINDOW_MANAGER_VNC_SERVER */

    std::string standardizeName(const std::string& clientName)
    {
        std::string displayName = clientName;
        std::transform(displayName.begin(), displayName.end(), displayName.begin(), [](unsigned char c){ return std::tolower(c); });
        return displayName;
    }

    void notifyExtensionClientConfigChanged(const std::string& clientName, const ClientInfo& clientInfo);
    void notifyExtensionOwnerChanged(const std::string& clientName, int ownerId);

    /*
        getCompositorInfo searches gCompositorList and gTopmostCompositoList for compositor info with
        client name equal to clientName parameter.
        Returns true if compositor info was found in any of the lists and false otherwise.

        Iterator for found compositor info is stored in it parameter foundIt.

        If compositorList parameter is not null, it will store the compositor list in which
        compositor info was found.
    */
    bool getCompositorInfo(const std::string& clientName, CompositorListIterator& foundIt,
        CompositorList** compositorList = nullptr)
    {
        std::string stdClientName = standardizeName(clientName);

        for (auto it = gCompositorList.begin(); it != gCompositorList.end(); ++it)
        {
            if (it->name == stdClientName)
            {
                foundIt = it;

                if (compositorList)
                    *compositorList = &gCompositorList;
                return true;
            }
        }

        for (auto it = gTopmostCompositorList.begin(); it != gTopmostCompositorList.end(); ++it)
        {
            if (it->name == stdClientName)
            {
                foundIt = it;

                if (compositorList)
                    *compositorList = &gTopmostCompositorList;
                return true;
            }
        }

        return false;
    }

    /*
        getCompositorInfo searches gCompositorList and gTopmostCompositoList for compositor info with
        RdkCompositor equal to compositor parameter.
        Returns true if compositor info was found in any of the lists and false otherwise.
    */
    bool getCompositorInfo(const RdkCompositor* compositor, CompositorListIterator& foundIt)
    {
        for (auto it = gCompositorList.begin(); it != gCompositorList.end(); ++it)
        {
            if (it->compositor.get() == compositor)
            {
                foundIt = it;
                return true;
            }
        }

        for (auto it = gTopmostCompositorList.begin(); it != gTopmostCompositorList.end(); ++it)
        {
            if (it->compositor.get() == compositor)
            {
                foundIt = it;
                return true;
            }
        }

        return false;
    }

    size_t getNumCompositorInfo()
    {
        return gCompositorList.size() + gTopmostCompositorList.size();
    }

    void sendApplicationEvent(std::shared_ptr<RdkWindowManagerEventListener>& listener, const std::string& eventName, const std::string& client)
    { 
         if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_TERMINATED) == 0)
         {
                 listener->onApplicationTerminated(client);
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED) == 0)
         {
                 listener->onApplicationConnected(client);
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED) == 0)
         {
                 listener->onApplicationDisconnected(client);
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_FIRST_FRAME) == 0)
         {
                 CompositorListIterator it;
                 listener->onReady(client);
                 if (getCompositorInfo(client, it))
                 {
                     it->compositor->setFirstFrameRendered(true);
                 }
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_VISIBLE) == 0)
         {
                 listener->onApplicationVisible(client);
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_HIDDEN) == 0)
         {
                 listener->onApplicationHidden(client);
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_FOCUS) == 0)
         {
                 listener->onApplicationFocus(client);
         }
         else if(eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_BLUR) == 0)
         {
                 listener->onApplicationBlur(client);
         }
    }

    bool interceptKey(uint32_t keycode, uint32_t flags, uint64_t metadata, bool isPressed)
    {
        bool ret = false;

	Logger::log(Debug, "interceptKey called Keycode - %u, flags - %u, metadata -%llu, isPressed- %d", keycode, flags, metadata, isPressed);
        if (gKeyInterceptInfoMap.end() != gKeyInterceptInfoMap.find(keycode))
        {
	    gKeyInterceptedMap.clear();

            for (int i=0; i<gKeyInterceptInfoMap[keycode].size(); i++)
            {
                struct KeyInterceptInfo& info = gKeyInterceptInfoMap[keycode][i];
                bool isFocused = false;
                bool interceptFlag = false;

                if( info.compositorInfo.name == gFocusedCompositor.name)
                {
                    isFocused = true;
                }

		Logger::log(Debug, "inside for loop gKeyInterceptInfoMap and isFocused -%d info.compositorInfo.name - %s", isFocused,info.compositorInfo.name.c_str());
                if (info.flags == flags && info.compositorInfo.compositor->getInputEventsEnabled())
                {
		    if( (true == info.focusOnly))
		    {

                        if(true == isFocused)
			{
			    //focus-only: send intercept to app if its focused.
	                    interceptFlag = true;
			}
			else
			{
			    //won't propagate to any listeners
			    Logger::log(LogLevel::Information, "Key %d is not intercepted by client %s for app", keycode, info.compositorInfo.name.c_str());
			    continue;
			}
		    }
                    else if(false == info.focusOnly)
                    {
			//All - send intercept to app even if its not focused.
			interceptFlag = true;
                    }

		    //good to send key intercept.
                    if (interceptFlag && !gKeyInterceptedMap[info.compositorInfo.name] )
                    {
			Logger::log(LogLevel::Information, "Key %d intercepted by client %s for app", keycode, info.compositorInfo.name.c_str());
                        if (isPressed)
                        {
                            info.compositorInfo.compositor->onKeyPress(keycode, flags, metadata);
                        }
                        else
                        {
                            info.compositorInfo.compositor->onKeyRelease(keycode, flags, metadata);
                        }
			gKeyInterceptedMap[info.compositorInfo.name] = true;
                        ret = true;
                    }

                    if(true == info.propagate)
                    {
                        //Propaget: send intercept to app which comes after focused app though its not focused.
			std::vector<CompositorInfo>::iterator compositorIterator = gCompositorList.begin();
			std::string currentCompositorName = info.compositorInfo.name;
			for (compositorIterator = gCompositorList.begin();  compositorIterator != gCompositorList.end(); compositorIterator++)
			{
                            if (compositorIterator->name == currentCompositorName)
			    {
				//start propagate after current app
				compositorIterator++;
				break;
			    }
			}

			while (compositorIterator != gCompositorList.end())
			{
			    if (!compositorIterator->compositor->getInputEventsEnabled())
			    {
			        compositorIterator++;
				continue;
			    }

			    if(!gKeyInterceptedMap[compositorIterator->name])
			    {
                                Logger::log(LogLevel::Information, "Key %d intercepted by client %s for app with propagate enable", keycode, info.compositorInfo.name.c_str());
                                if (isPressed)
                                {
                                    compositorIterator->compositor->onKeyPress(keycode, flags, metadata);
                                }
                                else
                                {
                                    compositorIterator->compositor->onKeyRelease(keycode, flags, metadata);
                                }
			        gKeyInterceptedMap[compositorIterator->name] = true;
			    }
			    compositorIterator++;
			}

                    }

                }

            }
	    gKeyInterceptedMap.clear();
        }
        return ret;
    }

    void evaluateKeyListeners(struct CompositorInfo& compositor, uint32_t keycode, uint32_t flags, bool& foundlistener, bool& activate, bool& propagate)
    {
        std::map<uint32_t, std::vector<KeyListenerInfo>>& keyListenerInfo = compositor.keyListenerInfo;

        if (keyListenerInfo.end() != keyListenerInfo.find(keycode))
        {
          for (size_t i=0; i<keyListenerInfo[keycode].size(); i++)
          {
            struct KeyListenerInfo& info = keyListenerInfo[keycode][i];

            if (info.flags == flags)
            {
              foundlistener  = true;
              activate = info.activate;
              propagate = info.propagate;
              break;
            }
          }
        }

        // handle wildcard if no listener found
        if ((false == foundlistener) && (keyListenerInfo.find(RDK_WINDOW_MANAGER_ANY_KEY) != keyListenerInfo.end()))
        {
          struct KeyListenerInfo& info = keyListenerInfo[RDK_WINDOW_MANAGER_ANY_KEY][0];
          foundlistener  = true;
          activate = info.activate;
          propagate = info.propagate;
        }
    }

    void bubbleKey(uint32_t keycode, uint32_t flags, uint64_t metadata, bool isPressed)
    {
        std::vector<CompositorInfo>::iterator compositorIterator = gCompositorList.begin();
        std::string focusedCompositorName = gFocusedCompositor.name;
        #ifndef RDK_WINDOW_MANAGER_ENABLE_KEYBUBBING_TOP_MODE
        for (compositorIterator = gCompositorList.begin();  compositorIterator != gCompositorList.end(); compositorIterator++)
        {
          if (compositorIterator->name == gFocusedCompositor.name)
          {
            break;
          }
        }
        #else
        Logger::log(Debug, "Key bubbling is made from top application");
        #endif //RDK_WINDOW_MANAGER_ENABLE_KEYBUBBING_TOP_MODE

        bool activateCompositor = false, propagateKey = true, foundListener = false;
        bool stopPropagation = false;
        bool isFocusedCompositor = true;
        while (compositorIterator != gCompositorList.end())
        {
          if (!compositorIterator->compositor->getInputEventsEnabled())
          {
              compositorIterator++;
              continue;
          }

          #ifdef RDK_WINDOW_MANAGER_ENABLE_KEYBUBBING_TOP_MODE
          if (compositorIterator->name == focusedCompositorName)
          {
              compositorIterator++;
              continue;
          }
          isFocusedCompositor = false;
          #endif //RDK_WINDOW_MANAGER_ENABLE_KEYBUBBING_TOP_MODE
          activateCompositor = false;
          propagateKey = true;
          foundListener = false;
          evaluateKeyListeners(*compositorIterator, keycode, flags, foundListener, activateCompositor, propagateKey);

          if ((false == isFocusedCompositor) && (true == foundListener))
          {
            Logger::log(Debug, "Key %d sent to listener %s", keycode, compositorIterator->name.c_str());
            if (isPressed)
            {
              compositorIterator->compositor->onKeyPress(keycode, flags, metadata);
              gPendingKeyUpListeners.push_back(compositorIterator->compositor);
            }
            else
            {
              compositorIterator->compositor->onKeyRelease(keycode, flags, metadata);
            }
          }
          isFocusedCompositor = false;
          if (activateCompositor)
          {
              if (gFocusedCompositor.name != compositorIterator->name)
              {
                  std::string previousFocusedClient = !gFocusedCompositor.name.empty() ? gFocusedCompositor.name:"none";
                  Logger::log(LogLevel::Information,  "rdkwindowmanager_focus bubbleKey: the focused client is now %s . previous: %s", (*compositorIterator).name.c_str(), previousFocusedClient.c_str());
                  if ((gFocusedCompositor.compositor) && (gFocusedCompositor.compositor->isKeyPressed()))
                  {
                      gPendingKeyUpListeners.push_back(gFocusedCompositor.compositor);
                  }
                  gFocusedCompositor = *compositorIterator;
              }
          }

          //propagate is false, stopping here
          if (false == propagateKey)
          {
            break;
          }
          compositorIterator++;
        }
    }

    void updateKeyRepeat()
    {
        if (gKeyRepeatConfig.enabled && gLastKeyPressStartTime > 0.0) 
        {
            double currentTime = RdkWindowManager::seconds();
            double timeSincePress = (currentTime - gLastKeyPressStartTime);
            double timeSinceRepeat = (currentTime - gLastKeyRepeatTime);

            /* Check if the initial delay has passed or the repeat interval has passed */
            if (((gLastKeyRepeatTime == 0.0) && (timeSincePress * 1000.0 > gKeyRepeatConfig.initialDelay)) || \
                ((gLastKeyRepeatTime != 0.0) && (timeSinceRepeat * 1000.0 > gKeyRepeatConfig.repeatInterval)))
            {
                CompositorController::onKeyPress(gLastKeyCode, gLastKeyModifiers, gLastKeyMetadata);
                gLastKeyRepeatTime = currentTime;
            }
        }
    }

    void updateGenerateKeyEvents()
    {
        double currentTime = RdkWindowManager::seconds();
        auto it = gGenerateKeyEvents.begin();
        while (it != gGenerateKeyEvents.end())
        {
            if (it->triggerTime <= currentTime)
            {
                if (it->client.empty())
                {
                    CompositorController::onKeyRelease(it->keyCode, it->modifiers, 0, false);
                }
                else
                {
                    CompositorListIterator cit;
                    if (getCompositorInfo(it->client, cit))
                    {
                        cit->compositor->onKeyRelease(it->keyCode, it->modifiers, 0);
                    }
                }

                it = gGenerateKeyEvents.erase(it);
            }
            else
                ++it;
        }
    }

    bool addCompositor(CompositorList* compositorList, CompositorInfo compositorInfo)
    {
        if (compositorList->empty())
        {
            compositorList->push_back(compositorInfo);
        }
        else
        {
            CompositorListIterator it = compositorList->begin();
            for (it = compositorList->begin(); it != compositorList->end(); ++it)
            {
                if (compositorInfo.zorder > it->zorder)
                {
                    break;
                }
            }
            if (it == compositorList->end())
            {
                compositorList->push_back(compositorInfo);
            }
            else
            {
                compositorList->insert(it, compositorInfo);
            }
        }
        return true;
    }

    std::shared_ptr<RdkCompositor> CompositorController::getCompositor(const std::string& displayName)
    {
        auto lambda = [displayName](CompositorInfo& info)
        {
            std::string compositorDisplayName;
            info.compositor->displayName(compositorDisplayName);
            return compositorDisplayName == displayName;
        };

        auto it = std::find_if(gCompositorList.begin(), gCompositorList.end(), lambda);

        if (it == gCompositorList.end())
        {
            it = std::find_if(gTopmostCompositorList.begin(), gTopmostCompositorList.end(), std::move(lambda));

            if (it == gTopmostCompositorList.end())
            {
                return nullptr;
            }
        }

        return it->compositor;
    }

    void CompositorController::initialize()
    {
        static bool sCompositorInitialized = false;
        if (sCompositorInitialized)
            return;

        char const *rdkwindowmanagerKeyIgnore = getenv("RDK_WINDOW_MANAGER_ENABLE_KEY_IGNORE");
        Logger::log(LogLevel::Information,  "key ignore feature enabled status [%d]", (NULL==rdkwindowmanagerKeyIgnore));
        if (NULL != rdkwindowmanagerKeyIgnore)
        {
            int keyIgnoreValue = atoi(rdkwindowmanagerKeyIgnore);
            Logger::log(LogLevel::Information,  "key ignore feature [%d]", keyIgnoreValue);
            if (keyIgnoreValue > 0)
            {
                gIgnoreKeyInputEnabled = true;
                Logger::log(LogLevel::Information,  "key ignore feature is enabled");
            }
        }

        char const *rdkwindowmanagerCompositorType = getenv("RDK_WINDOW_MANAGER_COMPOSITOR_TYPE");

        if (NULL == rdkwindowmanagerCompositorType)
        {
            Logger::log(LogLevel::Information,  "compositor type is empty, setting to nested by default");
        }
        else if (strcmp(rdkwindowmanagerCompositorType, "nested") != 0)
        {
            Logger::log(LogLevel::Information,  "invalid compositor type, setting to nested by default ");
        }

        const char* cursorImageName = getenv("RDK_WINDOW_MANAGER_CURSOR_IMAGE");
        if (cursorImageName == nullptr)
        {
            Logger::log(LogLevel::Information,  "cursor image not set");
        }
        else
        {
            gCursor = std::make_shared<Cursor>(std::string(cursorImageName));
        }

        sCompositorInitialized = true;
    }

    bool CompositorController::getFocused(std::string& client)
    {
        client = "";
        if (gFocusedCompositor.compositor)
        {
            client = gFocusedCompositor.name;
        }
        return true;
    }

    bool CompositorController::setFocus(const std::string& client)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            std::string previousFocusedClient = !gFocusedCompositor.name.empty() ? gFocusedCompositor.name:"none";
            Logger::log(LogLevel::Information,  "rdkwindowmanager_focus setFocus: the focused client is now %s.  previous: %s", it->name.c_str(), previousFocusedClient.c_str());
            if ((gFocusedCompositor.compositor) && (gFocusedCompositor.compositor->isKeyPressed()))
            {
                gPendingKeyUpListeners.push_back(gFocusedCompositor.compositor);
            }

            if (gFocusedCompositor.compositor)
            {
                gFocusedCompositor.compositor->setFocused(false);
            }

            gFocusedCompositor = *it;
            gFocusedCompositor.compositor->setFocused(true);

            return true;
        }
        return false;
    }

    bool CompositorController::kill(const std::string& client)
    {
        CompositorListIterator it;
        CompositorList* compositorInfoList;
        if (getCompositorInfo(client, it, &compositorInfoList))
        {
            std::string clientDisplayName = standardizeName(client);

            // cleanup key intercepts
            std::vector<std::map<uint32_t, std::vector<KeyInterceptInfo>>::iterator> emptyKeyCodeEntries;
            std::map<uint32_t, std::vector<KeyInterceptInfo>>::iterator entry = gKeyInterceptInfoMap.begin();
            while(entry != gKeyInterceptInfoMap.end())
            {
                std::vector<KeyInterceptInfo>& interceptMap = entry->second;
                std::vector<KeyInterceptInfo>::iterator interceptMapEntry=interceptMap.begin();
                while (interceptMapEntry != interceptMap.end())
                {
                    if ((*interceptMapEntry).compositorInfo.name == clientDisplayName)
                    {
                        interceptMapEntry = interceptMap.erase(interceptMapEntry);
                    }
                    else
                    {
                        interceptMapEntry++;
                    }
                }
                if (interceptMap.size() == 0)
                {
                    entry = gKeyInterceptInfoMap.erase(entry);
                }
                else
                {
                    entry++;
                }
            }

            {
                std::lock_guard<std::mutex> lock(gClientAliasMapMutex);
                gClientAliasMap.erase(clientDisplayName);
            }

            // cleanup key listeners
            for (std::map<uint32_t, std::vector<KeyListenerInfo>>::iterator iter = it->keyListenerInfo.begin(); iter != it->keyListenerInfo.end(); iter++)
            {
                iter->second.clear();
            }
            it->keyListenerInfo.clear();
            it->eventListeners.clear();
            std::cout << "adding " << clientDisplayName << " to the deleted list\n";
            gDeletedCompositors.push_back(*it);
            compositorInfoList->erase(it);
            if (gFocusedCompositor.name == clientDisplayName)
            {
                // this may be changed to next available compositor
                gFocusedCompositor.name = "";
                gFocusedCompositor.compositor = nullptr;
                Logger::log(LogLevel::Information,  "rdkwindowmanager_focus kill: the focused client has been killed: %s.  there is no focused client.", clientDisplayName.c_str());
            }
            return true;
        }
        return false;
    }

    bool CompositorController::addKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, const bool& focusOnly, const bool& propagate)
    {
        //Logger::log(LogLevel::Information,  "key intercept added " << keyCode << " flags " << flags << std::endl;
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            struct KeyInterceptInfo info;
            info.keyCode = keyCode;
            info.flags = flags;
	    info.focusOnly = focusOnly;
	    info.propagate = propagate;
            info.compositorInfo = *it;
            if (gKeyInterceptInfoMap.end() == gKeyInterceptInfoMap.find(keyCode))
            {
                gKeyInterceptInfoMap[keyCode] = std::vector<KeyInterceptInfo>();
                gKeyInterceptInfoMap[keyCode].push_back(info);
            }
            else
            {
                std::string clientDisplayName = standardizeName(client);
                bool isEntryAvailable = false;
                for (int i=0; i<gKeyInterceptInfoMap[keyCode].size(); i++)
                {
                    struct KeyInterceptInfo& info = gKeyInterceptInfoMap[keyCode][i];
                    if ((info.flags == flags) && (info.compositorInfo.name == clientDisplayName))
                    {
                        isEntryAvailable = true;
                        break;
                    }
                }
                if (false == isEntryAvailable)
                {
                    gKeyInterceptInfoMap[keyCode].push_back(info);
                }
            }
            return true;
        }
        return false;
    }

    bool CompositorController::removeKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
    {
        if (keyCode == RDK_WINDOW_MANAGER_WILDCARD_KEY_CODE)
        {
            std::string clientDisplayName = standardizeName(client);
            for (std::map<uint32_t, std::vector<KeyInterceptInfo>>::iterator keyInterceptIterator = gKeyInterceptInfoMap.begin(); keyInterceptIterator != gKeyInterceptInfoMap.end(); keyInterceptIterator++)
            {
                std::vector<KeyInterceptInfo>& interceptInfo = keyInterceptIterator->second;
                std::vector<KeyInterceptInfo>::iterator interceptInfoIterator = interceptInfo.begin();
                while(interceptInfoIterator != interceptInfo.end())
                {
                    if ((*interceptInfoIterator).compositorInfo.name == clientDisplayName)
                    {
                         interceptInfoIterator = interceptInfo.erase(interceptInfoIterator);
                    }
                    else
                    {
                        interceptInfoIterator++;
                    }
                }
            }
        }
        if (client == "*")
        {
            std::vector<std::vector<KeyInterceptInfo>::iterator> keyMapEntries;
            std::map<uint32_t, std::vector<KeyInterceptInfo>>::iterator it = gKeyInterceptInfoMap.find(keyCode);
            if (it != gKeyInterceptInfoMap.end())
            {
              std::vector<KeyInterceptInfo>::iterator entry = gKeyInterceptInfoMap[keyCode].begin();
              while(entry != gKeyInterceptInfoMap[keyCode].end())
              {
                  if ((*entry).flags == flags)
                  {
                    if (((*entry).compositorInfo.compositor) && ((*entry).compositorInfo.compositor->isKeyPressed()))
                    {
                        gPendingKeyUpListeners.push_back((*entry).compositorInfo.compositor);
                    }
                    entry = gKeyInterceptInfoMap[keyCode].erase(entry);
                  }
                  else
                  {
                    entry++;
                  }
              }
              if ( gKeyInterceptInfoMap[keyCode].size() == 0)
              {
                 gKeyInterceptInfoMap.erase(keyCode);
              }
            }
            return true;
        }

        std::string clientDisplayName = standardizeName(client);
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            if (gKeyInterceptInfoMap.end() != gKeyInterceptInfoMap.find(keyCode))
            {
                bool isEntryAvailable = false;
                std::vector<KeyInterceptInfo>::iterator entryPos = gKeyInterceptInfoMap[keyCode].end();
                for (std::vector<KeyInterceptInfo>::iterator it = gKeyInterceptInfoMap[keyCode].begin() ; it != gKeyInterceptInfoMap[keyCode].end(); ++it)
                {
                    if (((*it).flags == flags) && ((*it).compositorInfo.name == clientDisplayName))
                    {
                        entryPos = it;
                        isEntryAvailable = true;
                        break;
                    }
                }
                if (true == isEntryAvailable)
                {
                    if (((*entryPos).compositorInfo.compositor) && ((*entryPos).compositorInfo.compositor->isKeyPressed()))
                    {
                        gPendingKeyUpListeners.push_back((*entryPos).compositorInfo.compositor);
                    }
                    gKeyInterceptInfoMap[keyCode].erase(entryPos);
                    if (gKeyInterceptInfoMap[keyCode].size() == 0)
                    {
                        gKeyInterceptInfoMap.erase(keyCode);
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool CompositorController::addKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData> &listenerProperties)
    {
        bool activate = false, propagate = true;
        for ( const auto &property : listenerProperties)
        {
          if (property.first == "activate")
          {
            activate = property.second.toBoolean();
          }
          else if (property.first == "propagate")
          {
            propagate = property.second.toBoolean();
          }
        }
        Logger::log(LogLevel::Information,  "key listener added client: %s activate: %d propagate: %d RDKWindowManager keyCode: %d flags: %d", client.c_str(), activate, propagate, keyCode, flags);

        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            struct KeyListenerInfo info;
            info.keyCode = keyCode;
            info.flags = flags;
            info.activate = activate;
            info.propagate = propagate;

            if (it->keyListenerInfo.end() == it->keyListenerInfo.find(keyCode))
            {
                it->keyListenerInfo[keyCode] = std::vector<KeyListenerInfo>();
                it->keyListenerInfo[keyCode].push_back(info);
            }
            else
            {
                std::vector<KeyListenerInfo>& keyListenerEntry = it->keyListenerInfo[keyCode];
                bool isEntryAvailable = false;
                for (int i = 0; i < keyListenerEntry.size(); i++)
                {
                    struct KeyListenerInfo& listenerInfo = keyListenerEntry[i];
                    if (listenerInfo.flags == flags)
                    {
                        listenerInfo.activate = activate;
                        listenerInfo.propagate = propagate;
                        isEntryAvailable = true;
                        break;
                    }
                }
                if (false == isEntryAvailable)
                {
                    keyListenerEntry.push_back(info);
                }
            }
            return true;
        }
        return false;
    }

    bool CompositorController::addNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData> &listenerProperties)
    {
        uint32_t mappedKeyCode = 0, mappedFlags = 0;
        keyCodeFromWayland(keyCode, flags, mappedKeyCode, mappedFlags);

        Logger::log(LogLevel::Information,  "Native keyCode: %d flags: %d converted to RDKWindowManager keyCode: %d flags: %d", keyCode, flags, mappedKeyCode, mappedFlags);

        return CompositorController::addKeyListener(client, mappedKeyCode, mappedFlags, listenerProperties);
    }

    bool CompositorController::removeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
    {
        Logger::log(LogLevel::Information,  "key listener removed client: %s RDKWindowManager keyCode %d flags %d", client.c_str(), keyCode, flags);

        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            if (it->keyListenerInfo.end() != it->keyListenerInfo.find(keyCode))
            {
                bool isEntryAvailable = false;
                std::vector<KeyListenerInfo>::iterator entryPos = it->keyListenerInfo[keyCode].end();
                for (std::vector<KeyListenerInfo>::iterator iter = it->keyListenerInfo[keyCode].begin() ; iter != it->keyListenerInfo[keyCode].end(); ++iter)
                {
                    if ((*iter).flags == flags)
                    {
                        entryPos = iter;
                        isEntryAvailable = true;
                        break;
                    }
                }
                if (true == isEntryAvailable)
                {
                    if ((it->compositor) && (it->compositor->isKeyPressed()))
                    {
                        gPendingKeyUpListeners.push_back(it->compositor);
                    }
                    it->keyListenerInfo[keyCode].erase(entryPos);
                    if (it->keyListenerInfo[keyCode].size() == 0)
                    {
                        it->keyListenerInfo.erase(keyCode);
                    }
                }
            }
            return true;
        }
        return false;
    }

    bool CompositorController::removeNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
    {
        uint32_t mappedKeyCode = 0, mappedFlags = 0;
        keyCodeFromWayland(keyCode, flags, mappedKeyCode, mappedFlags);

        Logger::log(LogLevel::Information,  "Native keyCode: %d flags: %d converted to RDKWindowManager keyCode: %d flags: %d", keyCode, flags, mappedKeyCode, mappedFlags);

        return CompositorController::removeKeyListener(client, mappedKeyCode, mappedFlags);
    }

    bool CompositorController::removeAllKeyIntercepts()
    {
        for (auto it = gKeyInterceptInfoMap.begin(); it != gKeyInterceptInfoMap.end(); ++it)
        {
            it->second.clear();
        }
        gKeyInterceptInfoMap.clear();
        return true;
    }

    bool CompositorController::removeAllKeyListeners()
    {
        for (auto it = gCompositorList.begin(); it != gCompositorList.end(); ++it)
        {
            for (auto keyListener = it->keyListenerInfo.begin(); keyListener != it->keyListenerInfo.end(); ++keyListener)
            {
                keyListener->second.clear();
            }
            it->keyListenerInfo.clear();
        }
        return true;
    }

    bool CompositorController::injectKey(const uint32_t& keyCode, const uint32_t& flags)
    {
        CompositorController::onKeyPress(keyCode, flags, 0, false);
        CompositorController::onKeyRelease(keyCode, flags, 0, false);
        return true;
    }

    bool CompositorController::generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey)
    {
        return generateKey(client, keyCode, flags, std::move(virtualKey), 0.0);
    }

    bool CompositorController::generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey, double duration)
    {
        bool ret = false;
        uint32_t code = keyCode, modifiers = flags;
        if (!virtualKey.empty())
        {
            bool mappingPresent = keyCodeFromVirtual(virtualKey, code, modifiers);
            if (!mappingPresent)
            {
                std::cout << "virtual key mapping not present for " << virtualKey << std::endl;
                return false;
            }
        }

        if (client.empty())
        {
            CompositorController::onKeyPress(code, modifiers, 0, false);
            if (duration == 0.0)
            {
                CompositorController::onKeyRelease(code, modifiers, 0, false);
            }
            else
            {
                GenerateKeyEvent event(client, code, modifiers, RdkWindowManager::seconds() + duration);
                gGenerateKeyEvents.push_back(event);
            }
            ret = true;
        }
        else
        {
            CompositorListIterator it;
            if (getCompositorInfo(client, it))
            {
                if (it->compositor != nullptr)
                {
                    it->compositor->onKeyPress(code, modifiers, 0);
                    if (duration == 0.0)
                    {
                        it->compositor->onKeyRelease(code, modifiers, 0);
                    }
                    else
                    {
                        GenerateKeyEvent event(client, code, modifiers, RdkWindowManager::seconds() + duration);
                        gGenerateKeyEvents.push_back(event);
                    }
                    ret = true;
                }
            }
        }
        return ret;
    }

    bool CompositorController::getScreenResolution(uint32_t &width, uint32_t &height)
    {
        RdkWindowManager::EssosInstance::instance()->resolution(width, height);
        return true;
    }

    bool CompositorController::setScreenResolution(const uint32_t width, const uint32_t height)
    {
        RdkWindowManager::EssosInstance::instance()->setResolution(width, height);
        return true;
    }

    bool CompositorController::getClients(std::vector<std::string>& clients)
    {
        clients.clear();

        for (const auto &client : gTopmostCompositorList)
        {
            std::string clientName = client.name;
            clients.push_back(clientName);
        }

        for ( const auto &client : gCompositorList)
        {
            std::string clientName = client.name;
            clients.push_back(clientName);
        }
        return true;
    }

    bool CompositorController::setAlias(const std::string& clientId, const std::string& alias)
    {
        CompositorListIterator it;
        if (!getCompositorInfo(clientId, it))
        {
            Logger::log(LogLevel::Error, "Client '%s' not found. Cannot set alias", clientId.c_str());
            return false;
        }

        const std::string standardizedClientId = standardizeName(clientId);
        {
            std::lock_guard<std::mutex> lock(gClientAliasMapMutex);
            gClientAliasMap[standardizedClientId] = alias;
        }

        return true;
    }

    std::string CompositorController::getDisplayNameFromAlias(const std::string& alias)
    {
        if (alias.empty())
        {
            return "";
        }

        std::lock_guard<std::mutex> lock(gClientAliasMapMutex);
        for (const auto& clientAliasEntry : gClientAliasMap)
        {
            if (clientAliasEntry.second == alias)
            {
                return clientAliasEntry.first;
            }
        }

        return "";
    }

    std::string CompositorController::getAliasFromDisplayName(const std::string& clientId)
    {
        if (clientId.empty())
        {
            return "";
        }

        const std::string standardizedClientId = standardizeName(clientId);
        std::lock_guard<std::mutex> lock(gClientAliasMapMutex);
        const auto aliasEntry = gClientAliasMap.find(standardizedClientId);
        if (aliasEntry != gClientAliasMap.end())
        {
            return aliasEntry->second;
        }

        return "";
    }

    bool CompositorController::getCapabilities(const std::string& clientId, std::string& capabilities)
    {
        CompositorListIterator it;
        if (!getCompositorInfo(clientId, it))
        {
            Logger::log(LogLevel::Error, "Client '%s' not found. Cannot get capabilities", clientId.c_str());
            return false;
        }
        capabilities = it->capabilities;
        return true;
    }

    bool CompositorController::getZOrder(const std::string& client, int32_t &zorder)
    {
        CompositorListIterator it;

        if (!getCompositorInfo(client, it))
        {
            Logger::log(LogLevel::Error, "Client '%s' not found. Cannot get zorder ", client.c_str());
            return false;
        }

        zorder = it->zorder;

        Logger::log(LogLevel::Information, "Successfully got zorder %d for client '%s'.", zorder, client.c_str());
        return true;
    }

    bool CompositorController::setZorder(const std::string& client, int32_t zorder)
    {
        CompositorListIterator it;
        CompositorList* compositorInfoList = nullptr;

        if (!getCompositorInfo(client, it, &compositorInfoList))
        {
            Logger::log(LogLevel::Error,  "%s not found and cannot set zorder:%d", client.c_str(), zorder);
            return false;
        }

        CompositorInfo compositorInfo = *it;
        if (zorder != compositorInfo.zorder)
        {
            CompositorListIterator listIt = compositorInfoList->begin();
            Logger::log(LogLevel::Information,  "%s compositor:%s zorder:%d -> new zorder:%d",
                        client.c_str(), compositorInfo.name.c_str(), compositorInfo.zorder, zorder);

            /* Updating compositor list based on zorder */
            compositorInfo.zorder = zorder;
            compositorInfoList->erase(it);
            for (listIt = compositorInfoList->begin(); listIt != compositorInfoList->end(); ++listIt)
            {
                if (zorder > listIt->zorder)
                {
                    break;
                }
            }
            if (listIt == compositorInfoList->end())
            {
                compositorInfoList->push_back(compositorInfo);
            }
            else
            {
                compositorInfoList->insert(listIt, compositorInfo);
            }
            Logger::log(LogLevel::Information,  "%s compositor:%s zorder:%d repositioned",
                        client.c_str(), compositorInfo.name.c_str(), compositorInfo.zorder);
        }
        else
        {
            Logger::log(LogLevel::Information,  "%s on the same compositor stack zorder:%d", client.c_str(), zorder);
        }
        return true;
    }

    bool CompositorController::getBounds(const std::string& client, uint32_t &x, uint32_t &y, uint32_t &width, uint32_t &height)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            int32_t xPos = 0;
            int32_t yPos = 0;
            it->compositor->position(xPos, yPos);
            it->compositor->size(width, height);
            x = (uint32_t)xPos;
            y = (uint32_t)yPos;
            return true;
        }
        return false;
    }

    bool CompositorController::setBounds(const std::string& client, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->setPosition(x,y);
            it->compositor->setSize(width, height);
            return true;
        }
        return false;
    }

    bool CompositorController::getVisibility(const std::string& client, bool& visible)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->visible(visible);
            return true;
        }
        return false;
    }

    bool CompositorController::setVisibility(const std::string& client, const bool visible)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool currentVisibility = false;
            it->compositor->visible(currentVisibility);
            it->compositor->setVisible(visible);

            if (currentVisibility != visible)
            {
                ClientInfo updatedInfo{};
                if (CompositorController::getClientInfo(client, updatedInfo))
                {
                    notifyExtensionClientConfigChanged(client, updatedInfo);
                }
            }
            return true;
        }
        return false;
    }

    bool CompositorController::getOpacity(const std::string& client, unsigned int& opacity)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            double o = 1.0;
            it->compositor->opacity(o);
            if (o <= 0.0)
            {
                o = 0.0;
            }
            opacity = (unsigned int)(o * 100);
            if (opacity > 100)
            {
                opacity = 100;
            }
            return true;
        }
        return false;
    }

    bool CompositorController::setOpacity(const std::string& client, const unsigned int opacity)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            double o = (double)opacity / 100.0;
            it->compositor->setOpacity(o);
            return true;
        }
        return true;
    }


    bool CompositorController::getScale(const std::string& client, double &scaleX, double &scaleY)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->scale(scaleX, scaleY);
            return true;
        }
        return false;
    }

    bool CompositorController::setScale(const std::string& client, double scaleX, double scaleY)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->setScale(scaleX, scaleY);
            return true;
        }
        return true;
    }

    bool CompositorController::getHolePunch(const std::string& client, bool& holePunch)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->holePunch(holePunch);
            return true;
        }
        return false;
    }

    bool CompositorController::setHolePunch(const std::string& client, const bool holePunch)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->setHolePunch(holePunch);
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "hole punch for %s set to %s", client.c_str(), holePunch ? "true" : "false");
            return true;
        }
        return false;
    }

    bool CompositorController::setGlobalHolePunch(const RdkWindowManagerRect& rect)
    {
        std::lock_guard<std::mutex> lock(gGlobalHolePunchMutex);
        gGlobalHolePunchRect = rect;
        Logger::log(LogLevel::Information,
                    "setGlobalHolePunch: x=%u y=%u width=%u height=%u",
                    rect.x, rect.y, rect.width, rect.height);
        return true;
    }

    bool CompositorController::getGlobalHolePunch(RdkWindowManagerRect& rect)
    {
        std::lock_guard<std::mutex> lock(gGlobalHolePunchMutex);
        rect = gGlobalHolePunchRect;
        return true;
    }

    bool CompositorController::enableGlobalHolePunch(bool enable)
    {
        std::lock_guard<std::mutex> lock(gGlobalHolePunchMutex);
        gGlobalHolePunchEnabled = enable;
        Logger::log(LogLevel::Information,
                    "enableGlobalHolePunch: %s", enable ? "enabled" : "disabled");
        return true;
    }

    bool CompositorController::getCrop(const std::string& client, int32_t &cropX, int32_t &cropY, int32_t &cropWidth, int32_t &cropHeight)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->crop(cropX, cropY, cropWidth, cropHeight);
            return true;
        }
        return false;
    }

    bool CompositorController::setCrop(const std::string& client, int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->setCrop(cropX, cropY, cropWidth, cropHeight);
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "crop  function for %s set to x=%d,y=%d,w=%d,h=%d", client.c_str(),cropX,cropY,cropWidth,cropHeight);
            return true;
        }
        return false;
    }

    bool CompositorController::scaleToFit(const std::string& client, const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            if (it->compositor != nullptr)
            {
                uint32_t currentWidth = 0;
                uint32_t currentHeight = 0;
                it->compositor->size(currentWidth, currentHeight);

                double scaleX = (double)width / (double)currentWidth;
                double scaleY = (double)height / (double)currentHeight;

                it->compositor->setPosition(x, y);
                it->compositor->setScale(scaleX, scaleY);
            }
        }
        return true;
    }

    void CompositorController::onKeyPress(uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress)
    {
        //Logger::log(LogLevel::Information,  "key press code " << keycode << " flags " << flags << std::endl;
        double currentTime = RdkWindowManager::seconds();
        if ((true == physicalKeyPress) && (0.0 == gLastKeyPressStartTime))
        {
            gLastKeyPressStartTime = currentTime;
        }
        gLastKeyCode = keycode;
        gLastKeyModifiers = flags;
        gLastKeyMetadata = metadata;
        gLastKeyEventTime = currentTime;
        gNextInactiveEventTime = gLastKeyEventTime + gInactivityIntervalInSeconds;
        gLastKeyRepeatTime = 0.0;
        bool isInterceptAvailable = false;

        isInterceptAvailable = interceptKey(keycode, flags, metadata, true);

        if (false == isInterceptAvailable && gFocusedCompositor.compositor)
        {
            gFocusedCompositor.compositor->onKeyPress(keycode, flags, metadata);
            bubbleKey(keycode, flags, metadata, true);
        }
        else
        {
            std::string focusedClientName = !gFocusedCompositor.name.empty() ? gFocusedCompositor.name : "none";
            Logger::log(LogLevel::Information,  "rdkwindowmanager_focus key intercepted: %d focused client: %s", isInterceptAvailable, focusedClientName.c_str());
        }
        if (gRdkWindowManagerEventListener && physicalKeyPress)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "sending the keyevent for key press");
            gRdkWindowManagerEventListener->onKeyEvent(keycode, flags, true);
        }
    }

    void CompositorController::onKeyRelease(uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress)
    {
        //Logger::log(LogLevel::Information,  "key release code " << keycode << " flags " << flags << std::endl;

        if (true == physicalKeyPress)
        {
            double keyPressTime = RdkWindowManager::seconds() - gLastKeyPressStartTime;
            gLastKeyPressStartTime = 0.0;
        }
        gLastKeyCode = keycode;
        gLastKeyModifiers = flags;
        gLastKeyEventTime = RdkWindowManager::seconds();
        gNextInactiveEventTime = gLastKeyEventTime + gInactivityIntervalInSeconds;

        bool isInterceptAvailable = false;
        isInterceptAvailable = interceptKey(keycode, flags, metadata, false);

        if (false == isInterceptAvailable)
        {
            if (gFocusedCompositor.compositor)
            {
                gFocusedCompositor.compositor->onKeyRelease(keycode, flags, metadata);
                bubbleKey(keycode, flags, metadata, false);
            }
        }
        for ( const auto &compositor : gPendingKeyUpListeners )
        {
            compositor->onKeyRelease(keycode, flags, metadata);
        }
        gPendingKeyUpListeners.clear();

        if (gRdkWindowManagerEventListener && physicalKeyPress)
        {
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "sending the keyevent for key release");
            gRdkWindowManagerEventListener->onKeyEvent(keycode, flags, false);
        }
    }

    void CompositorController::onPointerMotion(uint32_t x, uint32_t y)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "%s, x: %d, y: %d", __func__, x, y);

        if (gCursor)
        {
            gCursor->setPosition(x, y);
        }

        if (gFocusedCompositor.compositor)
        {
            gFocusedCompositor.compositor->onPointerMotion(x, y);
        }
    }

    void CompositorController::onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "%s, keycode: %d, x: %d, y: %d", __func__, keyCode, x, y);

        if (gCursor)
        {
            gCursor->setPosition(x, y);
        }

        if (gFocusedCompositor.compositor)
        {
            gFocusedCompositor.compositor->onPointerButtonPress(keyCode, x, y);
        }
    }

    void CompositorController::onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y)
    {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "%s, keycode: %d, x: %d, y: %d", __func__, keyCode, x, y);

        if (gCursor)
        {
            gCursor->setPosition(x, y);
        }

        if (gFocusedCompositor.compositor)
        {
            gFocusedCompositor.compositor->onPointerButtonRelease(keyCode, x, y);
        }
    }

    bool CompositorController::createDisplay(const std::string& client, const std::string& displayName,
        uint32_t displayWidth, uint32_t displayHeight, bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight,
        bool topmost, bool focus , int32_t ownerId, int32_t groupId, const std::string& capabilities)
    {
        Logger::log(LogLevel::Information,
            "rdkwindowmanager createDisplay client: %s, displayName: %s, res: %d x %d, virtualDisplayEnabled: %d, virtualRes: %d x %d, topmost: %d, focus: %d\n",
            client.c_str(), displayName.c_str(), displayWidth, displayHeight, virtualDisplayEnabled, virtualWidth, virtualHeight,
            topmost, focus);

        std::string clientDisplayName = standardizeName(client);
        std::string compositorDisplayName = displayName;
        if (displayName.empty())
        {
            compositorDisplayName = clientDisplayName;
        }

        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            Logger::log(LogLevel::Information,  "display with name %s already exists", client.c_str());
            return false;
        }
        CompositorInfo compositorInfo;
        compositorInfo.name = clientDisplayName;
        compositorInfo.compositor = std::make_shared<RdkCompositorNested>();
        compositorInfo.capabilities = capabilities;

        uint32_t width = 0;
        uint32_t height = 0;
        RdkWindowManager::EssosInstance::instance()->resolution(width, height);
        Logger::log(LogLevel::Information, "EssosInstance resolution: %d x %d", width, height);
        if (displayWidth > 0)
        {
            width = displayWidth;
        }
        if (displayHeight > 0)
        {
            height = displayHeight;
        }

        if (virtualDisplayEnabled)
        {
            if (virtualWidth == 0)
            {
                virtualWidth = width;
            }
            if (virtualHeight == 0)
            {
                virtualHeight = height;
            }
        }
        Logger::log(LogLevel::Information,
            "Compositor createDisplay client: %s, displayName: %s, res: %d x %d, virtualDisplayEnabled: %d, virtualRes: %d x %d, topmost: %d, focus: %d\n",
            clientDisplayName.c_str(), compositorDisplayName.c_str(), width, height, virtualDisplayEnabled, virtualWidth, virtualHeight,
            topmost, focus);

        bool ret = compositorInfo.compositor->createDisplay(compositorDisplayName, clientDisplayName, width, height,
            virtualDisplayEnabled, virtualWidth, virtualHeight, ownerId, groupId, capabilities);

        if (ret)
        {
            bool bNotifyFocusEvent = false;
            CompositorInfo prevFocusedCompositor = gFocusedCompositor;

            if ((!topmost && getNumCompositorInfo() == 0) || (topmost && focus))
            {
                gFocusedCompositor = compositorInfo;
                bNotifyFocusEvent = true;
                Logger::log(LogLevel::Information,  "rdkwindowmanager_focus create: setting focus of first application created %s", gFocusedCompositor.name.c_str());
            }
            else if (focus)
            {
                gFocusedCompositor = compositorInfo;
                bNotifyFocusEvent = true;
            }

            /* Updating compositor list based on topmost+1 zorder */
            if (topmost)
            {
                compositorInfo.zorder = (gTopmostCompositorList.empty() == true) ? 0 : (gTopmostCompositorList.begin()->zorder + 1);
                addCompositor(&gTopmostCompositorList, std::move(compositorInfo));
            }
            else
            {
                compositorInfo.zorder = (gCompositorList.empty() == true) ? 0 : (gCompositorList.begin()->zorder + 1);
                addCompositor(&gCompositorList, std::move(compositorInfo));
            }

            if (bNotifyFocusEvent)
            {
                if (prevFocusedCompositor.compositor)
                {
                    onFireboltExtensionEvent(prevFocusedCompositor.compositor.get(), RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_BLUR);
                }
                onFireboltExtensionEvent(gFocusedCompositor.compositor.get(), RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_FOCUS);
            }
        }
        return ret;
    }

    bool CompositorController::draw()
    {
        //first render deleted compositors to ensure there is no memory leak
        //mfnote: todo - come back and revisit this approach to prevent a memory leak

        for (auto reverseIterator = gDeletedCompositors.rbegin(); reverseIterator != gDeletedCompositors.rend(); reverseIterator++)
        {
            bool needsHolePunch = false;
            RdkWindowManagerRect rect;
            std::string compositorName = "unknown";
            reverseIterator->compositor->displayName(compositorName);
            std::cout << "rendering deleted compositor " << compositorName << std::endl;
            reverseIterator->compositor->draw(needsHolePunch, rect, false);
            reverseIterator->compositor->draw(needsHolePunch, rect, true);
        }

        gDeletedCompositors.clear();

#ifdef RDK_WINDOW_MANAGER_VNC_SERVER
#ifndef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
        if (gVncServerEnabled && gVncBuffer)
        {
            gVncBuffer->begin();
        }
#endif
#endif /* RDK_WINDOW_MANAGER_VNC_SERVER */

        // Base pass — draw app-tier compositors (players + apps, z < kOverlayZOrderThreshold)
        // in ascending z-order (background → foreground).
        for (auto reverseIterator = gCompositorList.rbegin(); reverseIterator != gCompositorList.rend(); reverseIterator++)
        {
            if (reverseIterator->zorder >= kOverlayZOrderThreshold)
                continue;
            bool needsHolePunch = false;
            RdkWindowManagerRect rect;
            reverseIterator->compositor->draw(needsHolePunch, rect, false);
        }

        // Global hole punch: after apps but before subtitles/watermark, punch a
        // transparent hole to expose the HW video layer beneath the composited
        // output — mirrors the per-player hole punch in WesterosWindowManager.
        {
            std::lock_guard<std::mutex> lock(gGlobalHolePunchMutex);
            if (gGlobalHolePunchEnabled)
            {
                glEnable(GL_SCISSOR_TEST);
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glScissor(gGlobalHolePunchRect.x, gGlobalHolePunchRect.y,
                          gGlobalHolePunchRect.width, gGlobalHolePunchRect.height);
                glClear(GL_COLOR_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);
            }
        }

        // Base pass — draw overlay-tier compositors (subtitles, watermark,
        // z >= kOverlayZOrderThreshold) on top of the hole-punched frame.
        for (auto reverseIterator = gCompositorList.rbegin(); reverseIterator != gCompositorList.rend(); reverseIterator++)
        {
            if (reverseIterator->zorder < kOverlayZOrderThreshold)
                continue;
            bool needsHolePunch = false;
            RdkWindowManagerRect rect;
            reverseIterator->compositor->draw(needsHolePunch, rect, false);
        }

        for (auto reverseIterator = gCompositorList.rbegin(); reverseIterator != gCompositorList.rend(); reverseIterator++)
        {
            if (reverseIterator->compositor->hasOverlays())
            {
                bool needsHolePunch = false;
                RdkWindowManagerRect rect;
                reverseIterator->compositor->draw(needsHolePunch, rect, true);
            }
        }


        if (gCursor)
        {
            gCursor->draw();
        }

#ifdef RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN
        if (gShowSplashImage && gSplashImage != nullptr)
        {
            if (gSplashDisplayTimeInSeconds > 0)
            {
                uint32_t splashShownTime = (uint32_t)(RdkWindowManager::seconds() - gSplashStartTime);
                if (splashShownTime >= gSplashDisplayTimeInSeconds)
                {
                    Logger::log(LogLevel::Information, "hiding splash screen after timeout: %u s", gSplashDisplayTimeInSeconds);
                    gShowSplashImage = false;
                    gSplashImage = nullptr;
                }
                else
                {
                    gSplashImage->draw();
                }
            }
            else
            {
                gSplashImage->draw();
            }
        }
#endif // RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN

#ifdef RDK_WINDOW_MANAGER_VNC_SERVER
        if (gVncServerEnabled && gVncBuffer)
        {
            gVncBuffer->publish();
#ifndef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
            // Extra draw call is disabled for now as it leads to TV Blank issue RDKEMW-6814 gVncBuffer->draw();
            gVncBuffer->end();
#endif
        }
#endif /* RDK_WINDOW_MANAGER_VNC_SERVER */
        return true;
    }

    bool CompositorController::update()
    {
        updateKeyRepeat();
        updateGenerateKeyEvents();

        if (gEnableInactivityReporting)
        {
            double currentTime = RdkWindowManager::seconds();
            if (currentTime > gNextInactiveEventTime)
            {
                if (gRdkWindowManagerEventListener)
                {
                    gRdkWindowManagerEventListener->onUserInactive(getInactivityTimeInMinutes());
                }
              gNextInactiveEventTime = currentTime + gInactivityIntervalInSeconds;
            }
        }
        return true;
    }

    bool CompositorController::addListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->eventListeners.push_back(listener);
        }
        return true;
    }

    bool CompositorController::removeListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            std::vector<std::shared_ptr<RdkWindowManagerEventListener>>::iterator entryToRemove = it->eventListeners.end();
            for (std::vector<std::shared_ptr<RdkWindowManagerEventListener>>::iterator iter = it->eventListeners.begin() ; iter != it->eventListeners.end(); ++iter)
            {
                if ((*iter) == listener)
                {
                    entryToRemove = iter;
                    break;
                }
            }
            if (entryToRemove != it->eventListeners.end())
            {
                it->eventListeners.erase(entryToRemove);
            }
        }
        return true;
    }

    bool CompositorController::onEvent(RdkCompositor* eventCompositor, const std::string& eventName)
    {
        bool killClient = false;
        std::string clientToKill("");

        CompositorListIterator it;
        if (getCompositorInfo(eventCompositor, it))
        {
            for (int i=0; i< it->eventListeners.size(); i++)
            {
                sendApplicationEvent(it->eventListeners[i], eventName, it->name);
            }
            if (eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED) == 0)
            {
                clientToKill = it->name;
                killClient = true;
            }
        }

        /* Firebolt Extension Events */
        auto eventIt = gFireboltExtensionEventMap.find(eventName);
        if (eventIt != gFireboltExtensionEventMap.end())
        {
            onFireboltExtensionEvent(eventCompositor, eventIt->second);
        }

        if (true == killClient)
        {
            CompositorController::kill(clientToKill);
        }
        return true;
    }

    void sendFireboltExtensionEvent(const std::shared_ptr<FireboltExtensionEventListener>& listener, const std::string& eventName, const std::string& client)
    {
        Logger::log(LogLevel::Information, "sendFireboltExtensionEvent - client:%s eventName:%s", client.c_str(), eventName.c_str());
        if (eventName.compare(RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_FOCUS) == 0)
        {
            listener->on_focus(client.c_str());
        }
        else if (eventName.compare(RDK_WINDOW_MANAGER_FIREBOLT_EXTENTION_EVENT_ON_BLUR) == 0)
        {
            listener->on_blur(client.c_str());
        }
        else if (eventName.compare(RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_CONNECTED) == 0)
        {
            listener->client_connected(client.c_str());
        }
        else if (eventName.compare(RDK_WINDOW_MANAGER_FIREBOLT_EXTENSION_EVENT_CLIENT_DISCONNECTED) == 0)
        {
            listener->client_disconnected(client.c_str());
        }
    }

    void sendExtensionEvent(const std::shared_ptr<ExtensionEventListener>& listener,
                                   const std::string& eventName,
                                   const std::string& clientName,
                                   const ClientInfo& clientInfo,
                                   int ownerId)
    {
        std::string eventClientName = CompositorController::getAliasFromDisplayName(clientName);
        if (eventClientName.empty())
        {
            eventClientName = clientName;
        }

        Logger::log(LogLevel::Information, "sendExtensionEvent - client:%s eventName:%s", eventClientName.c_str(), eventName.c_str());
        if (eventName == RDK_WINDOW_MANAGER_EXTENSION_EVENT_CLIENT_CONFIG_CHANGED)
        {
            listener->onClientConfigChanged(eventClientName,
                                            clientInfo.visible,
                                            clientInfo.zorder,
                                            clientInfo.opacity,
                                            clientInfo.x,
                                            clientInfo.y,
                                            clientInfo.width,
                                            clientInfo.height);
        }
        else if (eventName == RDK_WINDOW_MANAGER_EXTENSION_EVENT_OWNER_CHANGED)
        {
            listener->onOwnerChanged(ownerId, eventClientName);
        }
    }

    void notifyExtensionClientConfigChanged(const std::string& clientName, const ClientInfo& clientInfo)
    {
        const auto eventIt = gExtensionEventMap.find(RDK_WINDOW_MANAGER_EXTENSION_EVENT_CLIENT_CONFIG_CHANGED);
        if (eventIt == gExtensionEventMap.end())
        {
            return;
        }

        std::vector<std::pair<int, std::shared_ptr<ExtensionEventListener>>> listeners;
        {
            std::lock_guard<std::mutex> lock(gExtensionListenerMapMutex);
            listeners.reserve(gExtensionEventListenerMap.size());
            for (const auto& entry : gExtensionEventListenerMap)
            {
                listeners.emplace_back(entry.first, entry.second);
            }
        }

        for (const auto& entry : listeners)
        {
            sendExtensionEvent(entry.second, eventIt->second, clientName, clientInfo, clientInfo.ownerId);
        }
    }

    void notifyExtensionOwnerChanged(const std::string& clientName, int ownerId)
    {
        const auto eventIt = gExtensionEventMap.find(RDK_WINDOW_MANAGER_EXTENSION_EVENT_OWNER_CHANGED);
        if (eventIt == gExtensionEventMap.end())
        {
            return;
        }

        ClientInfo noopInfo{};
        std::vector<std::pair<int, std::shared_ptr<ExtensionEventListener>>> listeners;
        {
            std::lock_guard<std::mutex> lock(gExtensionListenerMapMutex);
            listeners.reserve(gExtensionEventListenerMap.size());
            for (const auto& entry : gExtensionEventListenerMap)
            {
                listeners.emplace_back(entry.first, entry.second);
            }
        }

        for (const auto& entry : listeners)
        {
            sendExtensionEvent(entry.second, eventIt->second, clientName, noopInfo, ownerId);
        }
    }

    bool CompositorController::addFireboltExtensionListener(const std::string& fbExtensionName, std::shared_ptr<FireboltExtensionEventListener> listener)
    {
        bool success = false;

        if (!listener)
        {
            Logger::log(LogLevel::Error,
                "addFireboltExtensionListener: fbExtensionName:%s listener is null!", fbExtensionName.c_str());
        }
        else
        {
            std::lock_guard<std::mutex> lock(gFireboltExtensionListenerMapMutex);
            gfbExtensionEventListenerMap[fbExtensionName] = listener;
            Logger::log(LogLevel::Information,
                "addFireboltExtensionListener: Listener is registered for fbExtensionName '%s'", fbExtensionName.c_str());
            success = true;
        }

        return success;
    }

    bool CompositorController::removeFireboltExtensionListener(const std::string& fbExtensionName, std::shared_ptr<FireboltExtensionEventListener> listener)
    {
        bool success = false;

        std::lock_guard<std::mutex> lock(gFireboltExtensionListenerMapMutex);
        auto it = gfbExtensionEventListenerMap.find(fbExtensionName);
        if (it == gfbExtensionEventListenerMap.end())
        {
            Logger::log(LogLevel::Warn,
                "removeFireboltExtensionListener: no listener found for fbExtensionName '%s'", fbExtensionName.c_str());
        }
        else if (it->second != listener)
        {
            Logger::log(LogLevel::Warn,
                "removeFireboltExtensionListener: listener mismatch for fbExtensionName '%s'", fbExtensionName.c_str());
        }
        else
        {
            gfbExtensionEventListenerMap.erase(it);
            Logger::log(LogLevel::Information,
                "removeFireboltExtensionListener: Listener removed for fbExtensionName '%s'", fbExtensionName.c_str());
            success = true;
        }

        return success;
    }

    bool CompositorController::onFireboltExtensionEvent(RdkCompositor* eventCompositor, const std::string& eventName)
    {
        bool success = false;

        if (eventCompositor != nullptr && !eventName.empty())
        {
            CompositorListIterator it;
            if (getCompositorInfo(eventCompositor, it))
            {
                Logger::log(LogLevel::Information,
                    "onFireboltExtensionEvent - eventName: %s display: %s", eventName.c_str(), it->name.c_str());

                std::vector<std::pair<std::string, std::shared_ptr<FireboltExtensionEventListener>>> listeners;
                {
                    std::lock_guard<std::mutex> lock(gFireboltExtensionListenerMapMutex);
                    if (gfbExtensionEventListenerMap.empty())
                    {
                        Logger::log(LogLevel::Warn, "onFireboltExtensionEvent - No event listeners registered!");
                    }
                    else
                    {
                        listeners.reserve(gfbExtensionEventListenerMap.size());
                        for (const auto& entry : gfbExtensionEventListenerMap)
                        {
                            listeners.emplace_back(entry.first, entry.second);
                        }
                    }
                }

                for (const auto& entry : listeners)
                {
                    const std::shared_ptr<FireboltExtensionEventListener>& listener = entry.second;

                    Logger::log(LogLevel::Information,
                        "onFireboltExtensionEvent - fbExtensionName: %s eventName: %s display: %s", entry.first.c_str(), eventName.c_str(), it->name.c_str());

                    sendFireboltExtensionEvent(listener, eventName, it->name);
                }

                success = true;
            }
            else
            {
                std::string displayName;
                eventCompositor->displayName(displayName);

                Logger::log(LogLevel::Warn,
                    "onFireboltExtensionEvent - eventName: %s display: %s CompositorInfo not found!",
                    eventName.c_str(), displayName.c_str());
            }
        }
        else
        {
            Logger::log(LogLevel::Error,
                "onFireboltExtensionEvent - Invalid eventCompositor[%p] or eventName[%s]", eventCompositor, eventName.c_str());
        }

        return success;
    }

    int CompositorController::addExtensionEventListener(std::shared_ptr<ExtensionEventListener> listener)
    {
        if (!listener)
        {
            Logger::log(LogLevel::Error,
                "addExtensionEventListener: listener is null!");
            return -1;
        }

        std::lock_guard<std::mutex> lock(gExtensionListenerMapMutex);
        const int listenerTag = ++gExtensionEventListenerTag;
        gExtensionEventListenerMap[listenerTag] = listener;
        Logger::log(LogLevel::Information,
            "addExtensionEventListener: Listener registered with tag %d", listenerTag);
        return listenerTag;
    }

    bool CompositorController::removeExtensionEventListener(int listenerTag)
    {
        bool success = false;

        std::lock_guard<std::mutex> lock(gExtensionListenerMapMutex);
        auto it = gExtensionEventListenerMap.find(listenerTag);
        if (it == gExtensionEventListenerMap.end())
        {
            Logger::log(LogLevel::Warn,
                "removeExtensionEventListener: no listener found for tag %d", listenerTag);
        }
        else
        {
            gExtensionEventListenerMap.erase(it);
            Logger::log(LogLevel::Information,
                "removeExtensionEventListener: Listener removed for tag %d", listenerTag);
            success = true;
        }

        return success;
    }

    void CompositorController::setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener)
    {
        gRdkWindowManagerEventListener = std::move(listener);
    }

    void CompositorController::enableInactivityReporting(bool enable)
    {
        gEnableInactivityReporting = enable;
    }

    void CompositorController::setInactivityInterval(double minutes)
    {
        gInactivityIntervalInSeconds = minutes * 60;
        gNextInactiveEventTime = gLastKeyEventTime + gInactivityIntervalInSeconds;
    }

    void CompositorController::resetInactivityTime()
    {
        gLastKeyEventTime = RdkWindowManager::seconds();
        gNextInactiveEventTime = RdkWindowManager::seconds() + gInactivityIntervalInSeconds;
    }

    double CompositorController::getInactivityTimeInMinutes()
    {
        double inactiveTimeInSeconds = RdkWindowManager::seconds() - gLastKeyEventTime;
        return (inactiveTimeInSeconds / 60.0);
    }

    bool CompositorController::setLogLevel(const std::string level)
    {
        Logger::setLogLevel(level.c_str());
        return true;
    }

    bool CompositorController::getLogLevel(std::string& level)
    {
        Logger::logLevel(level);
        return true;
    }

    bool CompositorController::sendEvent(const std::string& eventName, std::vector<std::map<std::string, RdkWindowManagerData>>& data)
    {
        if (!gRdkWindowManagerEventListener)
        {
            Logger::log(LogLevel::Information,  "event listener is not present and unable to send event: %s", eventName.c_str());
            return false;
        }
        return true;
    }

    bool CompositorController::enableKeyRepeats(bool enable)
    {
        RdkWindowManager::EssosInstance::instance()->setKeyRepeats(enable);
        return true;
    }

    bool CompositorController::getKeyRepeatsEnabled(bool& enable)
    {
        RdkWindowManager::EssosInstance::instance()->keyRepeats(enable);
        return true;
    }

    bool CompositorController::setTopmost(const std::string& client, bool topmost, bool focus)
    {
        Logger::log(LogLevel::Information,  "setTopmost client: %s, topmost: %d, focus: %d", client.c_str(), topmost, focus);
        bool ret = false;

        CompositorListIterator it;
        CompositorList* compositorInfoList = nullptr;
        if (!getCompositorInfo(client, it, &compositorInfoList))
        {
            Logger::log(LogLevel::Error,  "%s no such client ", client.c_str());
            return false;
        }

        CompositorList* targetList;
        if (topmost)
        {
            if (compositorInfoList == &gTopmostCompositorList)
            {
                Logger::log(LogLevel::Information,  "%s is already topmost and cannot set topmost again ", client.c_str());
                return false;
            }

            targetList = &gTopmostCompositorList;
        }
        else
        {
            if (compositorInfoList == &gCompositorList)
            {
                Logger::log(LogLevel::Information,  "%s is already non topmost and cannot set non topmost again ", client.c_str());
                return false;
            }

            targetList = &gCompositorList;
        }

        const auto &compositorInfo = *it;
        compositorInfoList->erase(it);
        targetList->insert(targetList->begin(), compositorInfo);

        if (topmost && focus)
        {
            setFocus(client);
        }

        return true;
    }

    bool CompositorController::getTopmost(std::string& client)
    {
        bool ret = false;

        if (!gTopmostCompositorList.empty())
        {
            client = gTopmostCompositorList.front().name;
            ret = true;
        }
        return ret;
    }

    bool CompositorController::getVirtualResolution(const std::string& client, uint32_t &virtualWidth, uint32_t &virtualHeight)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->getVirtualResolution(virtualWidth, virtualHeight);
            return true;
        }
        return false;
    }

    bool CompositorController::setVirtualResolution(const std::string& client, const uint32_t virtualWidth, const uint32_t virtualHeight)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->setVirtualResolution(virtualWidth, virtualHeight);
            return true;
        }
        return false;
    }

    bool CompositorController::enableVirtualDisplay(const std::string& client, const bool enable)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->enableVirtualDisplay(enable);
            return true;
        }
        return false;
    }

    bool CompositorController::getVirtualDisplayEnabled(const std::string& client, bool &enabled)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            enabled = it->compositor->getVirtualDisplayEnabled();
            return true;
        }
        return false;
    }

    bool CompositorController::getLastKeyPress(uint32_t &keyCode, uint32_t &modifiers, uint64_t &timestampInSeconds)
    {
        double timeSinceLastKeyPress = (RdkWindowManager::seconds() - gLastKeyEventTime);
        uint64_t currentTimeInSeconds = (uint64_t)time(0);
        timestampInSeconds = (uint64_t)currentTimeInSeconds - (uint64_t)timeSinceLastKeyPress;
        keyCode = gLastKeyCode;
        modifiers = gLastKeyModifiers;
        return true;
    }


    bool CompositorController::ignoreKeyInputs(bool ignore)
    {
        bool ret = false;
        if (gIgnoreKeyInputEnabled)
        {
            RdkWindowManager::EssosInstance::instance()->ignoreKeyInputs(ignore);
            ret = true;
        }
        else
        {
            Logger::log(LogLevel::Information,  "key inputs ignore feature is not enabled");
        }
        return ret;
    }

    bool CompositorController::screenShot(uint8_t* &data, uint32_t &size)
    {
        uint32_t width, height;
        RdkWindowManager::EssosInstance::instance()->resolution(width, height);
        size = 4 * width * height;
        data = (uint8_t *)malloc(size);
	if (!data)
    	{
        Logger::log(LogLevel::Error,
            "screenShot: malloc failed for %u bytes", size);
        return false;
    	}
	Logger::log(LogLevel::Information,"Test Abi screenShot: starting glReadPixels width=%u height=%u",width, height);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	Logger::log(LogLevel::Information, "screenShot: glReadPixels completed");

	GLenum err = glGetError();
   	if (err != GL_NO_ERROR)
    	{
        Logger::log(LogLevel::Error, "screenShot: glReadPixels failed, GL error=0x%x", err);

        free(data);
        data = nullptr;
        size = 0;

        return false;
    	}

    	Logger::log(LogLevel::Information, "screenShot: capture successful width=%u height=%u size=%u",
			        width, height, size);

        return true;
    }

    bool CompositorController::enableInputEvents(const std::string& client, bool enable)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            it->compositor->enableInputEvents(enable);
            return true;
        }
        return false;
    }

    bool CompositorController::showCursor()
    {
        if (gCursor)
        {
            gCursor->show();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool CompositorController::hideCursor()
    {
        if (gCursor)
        {
            gCursor->hide();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool CompositorController::setCursorSize(uint32_t width, uint32_t height)
    {
        if (gCursor)
        {
            gCursor->setSize(width, height);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool CompositorController::getCursorSize(uint32_t& width, uint32_t& height)
    {
        if (gCursor)
        {
            gCursor->getSize(width, height);
            return true;
        }
        else
        {
            return false;
        }
    }

    void CompositorController::setKeyRepeatConfig(bool enabled, int32_t initialDelay, int32_t repeatInterval)
    {
        gKeyRepeatConfig.enabled = enabled;
        gKeyRepeatConfig.initialDelay = initialDelay;
        gKeyRepeatConfig.repeatInterval = repeatInterval;

        Logger::log(LogLevel::Information, "setKeyRepeatConfig enabled: %d, initialDelay: %d, repeatInterval: %d",
            enabled, initialDelay, repeatInterval);
    }

    bool CompositorController::setAVBlocked(std::string callsign, bool blockAV)
    {
        return RdkWindowManager::EssosInstance::instance()->setAVBlocked(std::move(callsign), blockAV);
    }

    bool CompositorController::getBlockedAVApplications(std::vector<std::string>& apps)
    {
        RdkWindowManager::EssosInstance::instance()->getBlockedAVApplications(apps);
        return true;
    }

    bool CompositorController::isErmEnabled()
    {
        return RdkWindowManager::EssosInstance::instance()->isErmEnabled();
    }

    bool CompositorController::getClientInfo(const std::string& client, ClientInfo& ci)
    {
        CompositorListIterator it;
        if (!getCompositorInfo(client, it))
            return false;
        auto c = it->compositor;

        c->visible(ci.visible);
        ci.zorder = it->zorder;
        c->opacity(ci.opacity);
        c->position(ci.x, ci.y);
        c->logicalSize(ci.width, ci.height);  // tile/fullscreen dims set by setClientInfo
        c->scale(ci.sx, ci.sy);              // computed scale retained from setClientInfo
        c->crop(ci.cropX, ci.cropY, ci.cropWidth, ci.cropHeight);
        //c->ownerId(ci.ownerId);
        return true;
    }

    bool CompositorController::setClientInfo(const std::string& client, const ClientInfo& ci)
    {
        CompositorListIterator it;
        if (!getCompositorInfo(client, it))
            return false;
        auto c = it->compositor;

        ClientInfo currentInfo{};
        const bool hasCurrentInfo = CompositorController::getClientInfo(client, currentInfo);

        Logger::log(LogLevel::Information, "setClientInfo client:%s x:%d y:%d w:%u h:%u sx:%f sy:%f visible:%d opacity:%f zorder:%d",
            client.c_str(), ci.x, ci.y, ci.width, ci.height, ci.sx, ci.sy, (int)ci.visible, ci.opacity, ci.zorder);

        c->setVisible(ci.visible);
        c->setOpacity(ci.opacity);
        c->setPosition(ci.x, ci.y);

        // drawDirect composes with bounds=(0,0,mWidth,mHeight). If we called
        // setSize(ci.width, ci.height) here, mWidth would change to the tile size
        // (e.g. 456) and WstCompositorSetOutputSize would ask the app to resize.
        // If the app does NOT re-render at the new size, Westeros clips its
        // full-resolution output to the first 456 pixels -- showing only the
        // top-left corner instead of a scaled-down thumbnail.
        //
        // Solution: keep mWidth/mHeight at the app's natural render resolution and
        // compute scale = tile_size / natural_size so the full output is scaled
        // down to fit the tile in the matrix transform.
        uint32_t curW = 0, curH = 0;
        c->size(curW, curH);  // returns mWidth/mHeight (natural render resolution)
        if (curW == 0 || curH == 0)
        {
            // Fall back to actual screen resolution — not hardcoded 1920/1080
            // so that 720p and 4K screens are handled correctly.
            uint32_t screenW = 0, screenH = 0;
            getScreenResolution(screenW, screenH);
            if (curW == 0) curW = (screenW > 0) ? screenW : 1920;
            if (curH == 0) curH = (screenH > 0) ? screenH : 1080;
        }

        double scaleX, scaleY;
        if (ci.sx > 0.0)
            scaleX = ci.sx;
        else
            scaleX = (ci.width > 0) ? ((double)ci.width / curW) : 1.0;

        if (ci.sy > 0.0)
            scaleY = ci.sy;
        else
            scaleY = (ci.height > 0) ? ((double)ci.height / curH) : 1.0;

        it->compositor->setScale(scaleX, scaleY);
        Logger::log(LogLevel::Information, "setClientInfo scale:(%f,%f) client:%s (output:%ux%u tile:%ux%u)",
            scaleX, scaleY, client.c_str(), curW, curH, ci.width, ci.height);

        c->setCrop(ci.cropX, ci.cropY, ci.cropWidth, ci.cropHeight);
        setZorder(client, ci.zorder);

        // Store the logical (tile or fullscreen) dimensions and scale so that
        // getClientInfo returns consistent values in onClientConfigChanged events.
        // EPG needs these to match what it originally sent.
        const uint32_t logicalW = (ci.width > 0) ? ci.width
                                : (ci.sx > 0.0)   ? static_cast<uint32_t>(curW * ci.sx)
                                :                   curW;
        const uint32_t logicalH = (ci.height > 0) ? ci.height
                                : (ci.sy > 0.0)    ? static_cast<uint32_t>(curH * ci.sy)
                                :                    curH;
        c->setLogicalSize(logicalW, logicalH);
        Logger::log(LogLevel::Information, "setClientInfo logicalSize:(%ux%u) scale:(%f,%f) client:%s",
            logicalW, logicalH, scaleX, scaleY, client.c_str());

        ClientInfo updatedInfo{};
        if (CompositorController::getClientInfo(client, updatedInfo))
        {
            const bool nonOwnerConfigChanged = !hasCurrentInfo ||
                (currentInfo.visible != updatedInfo.visible) ||
                (currentInfo.zorder != updatedInfo.zorder) ||
                (currentInfo.opacity != updatedInfo.opacity) ||
                (currentInfo.x != updatedInfo.x) ||
                (currentInfo.y != updatedInfo.y) ||
                (currentInfo.width != updatedInfo.width) ||
                (currentInfo.height != updatedInfo.height) ||
                (currentInfo.cropX != updatedInfo.cropX) ||
                (currentInfo.cropY != updatedInfo.cropY) ||
                (currentInfo.cropWidth != updatedInfo.cropWidth) ||
                (currentInfo.cropHeight != updatedInfo.cropHeight);

            if (nonOwnerConfigChanged)
            {
                notifyExtensionClientConfigChanged(client, updatedInfo);
            }
        }

        return true;
    }

    bool CompositorController::getClientName(WstCompositor* compositor, std::string& clientName)
    {
        for (auto it = gCompositorList.begin(); it != gCompositorList.end(); ++it)
        {
            if(it->compositor->hasCompositor(compositor))
            {
                clientName =  it->name;
                return true;
            }
        }
        return false;
    }

    bool CompositorController::getFireboltSurface(const std::string& client, int surfaceId, uint32_t type)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->convertToFireboltSurface(surfaceId, (SurfaceType) type);
            return result;
        }
        return false;
    }

    bool CompositorController::setFireboltSurfaceZorder(const std::string& client, int surfaceId, int zOrder)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->setFireboltSurfaceZOrder(surfaceId, zOrder);
            return result;
        }
        return false;
    }

    bool CompositorController::setFireboltSurfaceName(const std::string& client, int surfaceId, const std::string& surfaceName)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->setFireboltSurfaceName(surfaceId, surfaceName);
            return result;
        }
        return false;
    }

    bool CompositorController::setFireboltSurfaceOpacity(const std::string& client, int surfaceId, double opacity)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->setFireboltSurfaceOpacity(surfaceId, opacity);
            return result;
        }
        return false;
    }

    bool CompositorController::setFireboltSurfaceBounds(const std::string& client, int surfaceId, int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->setFireboltSurfaceBounds(surfaceId, x, y, width, height);
            return result;
        }
        return false;
    }

    bool CompositorController::setFireboltSurfaceCrop(const std::string& client, int surfaceId, int32_t sx, int32_t sy, uint32_t swidth, uint32_t sheight)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->setFireboltSurfaceCrop(surfaceId, sx, sy, swidth, sheight);
            return result;
        }
        return false;
    }

    bool CompositorController::setFireboltSurfaceVisibility(const std::string& client, int surfaceId, bool visible)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->setFireboltSurfaceVisibility(surfaceId, visible);
            return result;
        }
        return false;
    }

    bool CompositorController::fireboltSurfaceDestroy(const std::string& client, int surfaceId)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result = it->compositor->fireboltSurfaceDestroy(surfaceId);
            return result;
        }
        return false;
    }

    bool CompositorController::getSurfaceInfo(const std::string& client, int surfaceId, FireboltSurfaceInfo& si)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            auto c = it->compositor;

            c->getSurfaceInfo(surfaceId, si);
            return true;
        }
        return false;
    }

    bool CompositorController::enableDisplayRender(const std::string& client, bool enable)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            bool result =it->compositor->enableDisplayRender(enable);
            if (enable && it->isSuspended) //resuming from suspended
            {
                //resetting Display size to original
#ifdef ENABLE_RDKWINDOWMANAGER_RENDER_MINIMIZE
                it->compositor->setSize(it->previousWidth, it->previousHeight);
#endif
                it->isSuspended = false;
                Logger::log(LogLevel::Information,  "resetting Display size to original for %s, width: %d, height: %d", client.c_str(), it->previousWidth, it->previousHeight);
            }
            else if (!enable && !it->isSuspended) // going to suspended
            {
                //saving Display size
                it->compositor->size(it->previousWidth, it->previousHeight);
                it->isSuspended = true;
                Logger::log(LogLevel::Information,  "saving Display size for %s, width: %d, height: %d", client.c_str(), it->previousWidth, it->previousHeight);
                //setting Display size to 1,1
#ifdef ENABLE_RDKWINDOWMANAGER_RENDER_MINIMIZE
                it->compositor->setSize(1, 1);
#endif
            }

            if (result && enable)
            {
                ClientInfo updatedInfo{};
                if (CompositorController::getClientInfo(client, updatedInfo))
                {
                    notifyExtensionClientConfigChanged(client, updatedInfo);
                }
            }
            return result;
        }
        return false;
    }

    bool CompositorController::renderReady(const std::string& client)
    {
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            return it->compositor->renderReady();
        }
        return false;
    }

    bool CompositorController::startVncServer()
    {
        bool result = false;

    #ifdef RDK_WINDOW_MANAGER_VNC_SERVER
        uint32_t width = 0, height = 0;
#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
        // Match capture dimensions in VNCServer2 bridge mode.
        constexpr uint32_t kVncBridgeCaptureWidth = 960;
        constexpr uint32_t kVncBridgeCaptureHeight = 540;
        width = kVncBridgeCaptureWidth;
        height = kVncBridgeCaptureHeight;
#else
        getScreenResolution(width, height);
#endif
        result = VncServerFactory::getInstance().initializeVncServer(width, height);
        if (result)
        {
            gVncBuffer = std::make_shared<RdkWindowManager::VncFrameBuffer>(width, height);
            gVncServerEnabled = true;
            Logger::log(LogLevel::Information, "VNC server started successfully with width %d height %d", width, height);
        }
        else
        {
            Logger::log(LogLevel::Error, "VNC server failed to start");
        }
    #else
        Logger::log(LogLevel::Warn, "VNC server feature is not enabled, attempt to start VNC server failed");
    #endif /* RDK_WINDOW_MANAGER_VNC_SERVER */

        return result;
    }

    bool CompositorController::stopVncServer()
    {
        bool result = false;

    #ifdef RDK_WINDOW_MANAGER_VNC_SERVER
        gVncServerEnabled = false;
        VncServerFactory::getInstance().stopVncServer();
        gVncBuffer.reset();
        Logger::log(LogLevel::Information,  "VNC server stopped successfully");
        result = true;
    #else
        Logger::log(LogLevel::Warn, "VNC server feature is not enabled, attempt to stop VNC server failed");
    #endif /* RDK_WINDOW_MANAGER_VNC_SERVER */

        return result;
    }
}

namespace RdkWindowManager
{
    bool CompositorController::showSplashScreen(uint32_t displayTimeInSeconds)
    {
#ifdef RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN
        Logger::log(LogLevel::Information, "showSplashScreen: requested display time %u s", displayTimeInSeconds);
        if (gShowSplashImage)
        {
            Logger::log(LogLevel::Information, "showSplashScreen: splash already visible, skipping");
            return true;
        }

        const char* splashEnv = getenv("RDKWINDOWMANAGER_SPLASH_IMAGE");
        if (splashEnv == nullptr)
        {
            Logger::log(LogLevel::Warn, "showSplashScreen: RDKWINDOWMANAGER_SPLASH_IMAGE not set");
            return false;
        }

        // Iterate the comma-separated list and pick the first path that exists
        std::string selectedPath;
        std::istringstream stream(splashEnv);
        std::string token;
        while (std::getline(stream, token, ','))
        {
            const size_t start = token.find_first_not_of(" \t");
            const size_t end   = token.find_last_not_of(" \t");
            if (start == std::string::npos)
                continue;
            token = token.substr(start, end - start + 1);
            if (std::ifstream(token).good())
            {
                selectedPath = token;
                Logger::log(LogLevel::Information, "showSplashScreen: using splash image: %s", selectedPath.c_str());
                break;
            }
            Logger::log(LogLevel::Information, "showSplashScreen: skipping '%s' (not found)", token.c_str());
        }

        if (selectedPath.empty())
        {
            Logger::log(LogLevel::Warn, "showSplashScreen: no usable splash image found in RDKWINDOWMANAGER_SPLASH_IMAGE");
            return false;
        }

        gSplashImage = std::make_shared<RdkWindowManager::Image>();
        gShowSplashImage = gSplashImage->loadLocalFile(selectedPath.c_str());
        if (!gShowSplashImage)
        {
            Logger::log(LogLevel::Error, "showSplashScreen: failed to load image: %s", selectedPath.c_str());
            gSplashImage = nullptr;
            return false;
        }

        gSplashDisplayTimeInSeconds = displayTimeInSeconds;
        gSplashStartTime = RdkWindowManager::seconds();
        Logger::log(LogLevel::Information, "showSplashScreen: showing '%s' for %u s", selectedPath.c_str(), displayTimeInSeconds);
        return true;
#else
        Logger::log(LogLevel::Warn, "showSplashScreen: splash screen support not compiled in");
        return false;
#endif // RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN
    }

    bool CompositorController::hideSplashScreen()
    {
#ifdef RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN
        Logger::log(LogLevel::Information, "hideSplashScreen: hiding splash screen");
        if (!gShowSplashImage)
        {
            Logger::log(LogLevel::Information, "hideSplashScreen: splash already hidden, skipping");
            return true;
        }
        gShowSplashImage = false;
        gSplashImage = nullptr;
        Logger::log(LogLevel::Information, "hideSplashScreen: splash screen hidden");
        return true;
#else
        Logger::log(LogLevel::Warn, "hideSplashScreen: splash screen support not compiled in");
        return false;
#endif // RDK_WINDOW_MANAGER_ENABLE_SPLASH_SCREEN
    }
}

