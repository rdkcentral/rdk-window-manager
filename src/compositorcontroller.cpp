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
#include <map>
#include <ctime>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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
        CompositorInfo() : name(), compositor(nullptr), eventListeners(), mimeType() {}
        std::string name;
        std::shared_ptr<RdkCompositor> compositor;
        std::map<uint32_t, std::vector<KeyListenerInfo>> keyListenerInfo;
        std::vector<std::shared_ptr<RdkWindowManagerEventListener>> eventListeners;
        std::string mimeType;
        bool autoDestroy;
        int32_t zorder;
    };

    struct KeyInterceptInfo
    {
        KeyInterceptInfo() : keyCode(-1), flags(0), compositorInfo() {}
        uint32_t keyCode;
        uint32_t flags;
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
            client(client) , triggerTime(triggerTime), keyCode(keyCode), modifiers(modifiers) {}
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

    static std::map<uint32_t, std::vector<KeyInterceptInfo>> gKeyInterceptInfoMap;

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

    std::shared_ptr<Cursor> gCursor = nullptr;
    KeyRepeatConfig gKeyRepeatConfig;
    std::vector<GenerateKeyEvent> gGenerateKeyEvents;

    std::string standardizeName(const std::string& clientName)
    {
        std::string displayName = clientName;
        std::transform(displayName.begin(), displayName.end(), displayName.begin(), [](unsigned char c){ return std::tolower(c); });
        return displayName;
    }

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
                 listener->onApplicationFirstFrame(client);
         }
    }
    
    bool interceptKey(uint32_t keycode, uint32_t flags, uint64_t metadata, bool isPressed)
    {
        bool ret = false;
        if (gKeyInterceptInfoMap.end() != gKeyInterceptInfoMap.find(keycode))
        {
            for (int i=0; i<gKeyInterceptInfoMap[keycode].size(); i++)
            {
                struct KeyInterceptInfo& info = gKeyInterceptInfoMap[keycode][i];
                if (info.flags == flags && info.compositorInfo.compositor->getInputEventsEnabled())
                {
                    Logger::log(Debug, "Key %d intercepted by client %s", keycode, info.compositorInfo.name.c_str());
                    if (isPressed)
                    {
                        info.compositorInfo.compositor->onKeyPress(keycode, flags, metadata);
                    }
                    else
                    {
                        info.compositorInfo.compositor->onKeyRelease(keycode, flags, metadata);
                    }
                    ret = true;
                }
            }
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
        if (gKeyRepeatConfig.enabled)
        {
            if (gLastKeyPressStartTime > 0.0)
            {
                double currentTime = RdkWindowManager::seconds();
                if (gLastKeyRepeatTime == 0.0)
                {
                    if ((currentTime - gLastKeyPressStartTime) * 1000.0 > gKeyRepeatConfig.initialDelay)
                    {
                        CompositorController::onKeyPress(gLastKeyCode, gLastKeyModifiers, gLastKeyMetadata);
                        gLastKeyRepeatTime = currentTime;
                    }
                }
                else
                {
                    if ((currentTime - gLastKeyRepeatTime) * 1000.0 > gKeyRepeatConfig.repeatInterval)
                    {
                        CompositorController::onKeyPress(gLastKeyCode, gLastKeyModifiers, gLastKeyMetadata);
                        gLastKeyRepeatTime = currentTime;
                    }
                }

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
            it = std::find_if(gTopmostCompositorList.begin(), gTopmostCompositorList.end(), lambda);

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

    bool CompositorController::addKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
    {
        //Logger::log(LogLevel::Information,  "key intercept added " << keyCode << " flags " << flags << std::endl;
        CompositorListIterator it;
        if (getCompositorInfo(client, it))
        {
            struct KeyInterceptInfo info;
            info.keyCode = keyCode;
            info.flags = flags;
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
        return generateKey(client, keyCode, flags, virtualKey, 0.0);
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

    bool CompositorController::getZOrder(std::vector<std::string>& clients)
    {
        clients.clear();

        for (const auto &client : gTopmostCompositorList)
        {
            std::string clientName = client.name;
            clients.push_back(clientName);
        }

        for (const auto &client : gCompositorList)
        {
            std::string clientName = client.name;
            clients.push_back(clientName);
        }
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
            it->compositor->setVisible(visible);
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
            RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information, "crop  function for %s set to x=%f,y=%f,w=%f,h=%f", client.c_str(),cropX,cropY,cropWidth,cropHeight);
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
        bool topmost, bool focus , bool autodestroy)
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
        compositorInfo.autoDestroy = autodestroy;
        compositorInfo.compositor = std::make_shared<RdkCompositorNested>();

        uint32_t width = 0;
        uint32_t height = 0;
        RdkWindowManager::EssosInstance::instance()->resolution(width, height);
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

        bool ret = compositorInfo.compositor->createDisplay(compositorDisplayName, clientDisplayName, width, height,
            virtualDisplayEnabled, virtualWidth, virtualHeight);

        if (ret)
        {
            if ((!topmost && getNumCompositorInfo() == 0) || (topmost && focus))
            {
                gFocusedCompositor = compositorInfo;
                Logger::log(LogLevel::Information,  "rdkwindowmanager_focus create: setting focus of first application created %s", gFocusedCompositor.name.c_str());
            }
            else if (focus)
            {
                gFocusedCompositor = compositorInfo;
            }

            /* Updating compositor list based on topmost+1 zorder */
            if (topmost)
            {
                compositorInfo.zorder = (gTopmostCompositorList.empty() == true) ? 0 : (gTopmostCompositorList.begin()->zorder + 1);
                addCompositor(&gTopmostCompositorList, compositorInfo);
            }
            else
            {
                compositorInfo.zorder = (gCompositorList.empty() == true) ? 0 : (gCompositorList.begin()->zorder + 1);
                addCompositor(&gCompositorList, compositorInfo);
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

        for (auto reverseIterator = gCompositorList.rbegin(); reverseIterator != gCompositorList.rend(); reverseIterator++)
        {
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
            if ((eventName.compare(RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED) == 0) && (it->autoDestroy == true))
            {
                clientToKill = it->name;
                killClient = true;
            }
        }
        if (true == killClient)
        {
            CompositorController::kill(clientToKill);
        }
        return true;
    }

    void CompositorController::setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener)
    {
        gRdkWindowManagerEventListener = listener;
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
            Logger::log(LogLevel::Information,  "event listener is not present and unable to send event ", eventName.c_str());
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

        auto compositorInfo = *it;
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
        uint64_t timeSinceLastKeyPress = RdkWindowManager::seconds() - gLastKeyEventTime;
        time_t currentTimeInSeconds = time(0);
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
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
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
        return RdkWindowManager::EssosInstance::instance()->setAVBlocked(callsign, blockAV);
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
        c->size(ci.width, ci.height);
        c->crop(ci.cropX, ci.cropY, ci.cropWidth, ci.cropHeight);

        return true;
    }

    bool CompositorController::setClientInfo(const std::string& client, const ClientInfo& ci)
    {
        CompositorListIterator it;
        if (!getCompositorInfo(client, it))
            return false;
        auto c = it->compositor;

        c->setVisible(ci.visible);
        c->setOpacity(ci.opacity);
        c->setPosition(ci.x, ci.y);
        c->setSize(ci.width, ci.height);
        c->setCrop(ci.cropX, ci.cropY, ci.cropWidth, ci.cropHeight);
        setZorder(client, ci.zorder);

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
}
