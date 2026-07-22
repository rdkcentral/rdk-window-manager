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

#include "linuxkeys.h"
#include "rdkwindowmanagerjson.h"
#include <iostream>
#include "logger.h"

#include <map>

struct RdkWindowManagerKeyMap
{
  uint32_t code;
  uint32_t flags;
};

static std::map<uint32_t, struct RdkWindowManagerKeyMap> sRdkWindowManagerKeyMap;
static std::map<std::string, struct RdkWindowManagerKeyMap> sRdkWindowManagerVirtualKeyMap;

uint32_t getKeyFlag(std::string modifier)
{
  uint32_t flag = 0;
  if (0 == modifier.compare("ctrl"))
  {
    flag = RDK_WINDOW_MANAGER_FLAGS_CONTROL;
  }
  else if (0 == modifier.compare("shift"))
  {
    flag = RDK_WINDOW_MANAGER_FLAGS_SHIFT;
  }
  else if (0 == modifier.compare("alt"))
  {
    flag = RDK_WINDOW_MANAGER_FLAGS_ALT;
  }
  return flag;
}

void mapNativeKeyCodes()
{
  //populate from the key map file
  const char* keyMapFile = getenv("RDK_WINDOW_MANAGER_KEYMAP_FILE");
  if (keyMapFile)
  {
    rapidjson::Document document;
    bool ret = RdkWindowManager::RdkWindowManagerJson::readJsonFile(keyMapFile, document);
    if (false == ret)
    {
      RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "RDKWindowManager keymap read error : [unable to open/read file (%s)", keyMapFile);
      return;
    }

    if (document.HasMember("keyMappings")) {
      const rapidjson::Value& jsonValue = document["keyMappings"];

      if (jsonValue.IsArray())
      {
        for (rapidjson::SizeType k = 0; k < jsonValue.Size(); k++)
        {
          uint32_t keyCode = 0, mappedKeyCode = 0;
          uint32_t flags = 0;

          const rapidjson::Value& mapEntry = jsonValue[k];
          if (mapEntry.IsObject() && mapEntry.HasMember("keyCode")  && mapEntry.HasMember("mapped"))
          {
            const rapidjson::Value& keyCodeValue = mapEntry["keyCode"];
            if (keyCodeValue.IsUint())
            {
              keyCode = keyCodeValue.GetUint();
            }
            else
            {
              RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "Ignoring keycode entry because of format issues of keycode");
              continue;
            }

            const rapidjson::Value& mappedValue = mapEntry["mapped"];
            if (mappedValue.IsObject() && mappedValue.HasMember("keyCode") && mappedValue.HasMember("modifiers"))
            {
                const rapidjson::Value& mappedKeyCodeValue = mappedValue["keyCode"];
                if (mappedKeyCodeValue.IsUint())
                {
                  mappedKeyCode = mappedKeyCodeValue.GetUint();
                }
                else
                {
                  RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "Ignoring keycode entry because of format issues of mapped keycode");
                  continue;
                }

                const rapidjson::Value& modifiersValue = mappedValue["modifiers"];
                if (modifiersValue.IsArray()) {
                  for (rapidjson::SizeType i = 0; i < modifiersValue.Size(); i++)
                  {
                    if (modifiersValue[i].IsString()) {
                      flags |= getKeyFlag(modifiersValue[i].GetString());
                    }
                  }
                }

                struct RdkWindowManagerKeyMap keyMap;
                keyMap.code = mappedKeyCode;
                keyMap.flags = flags;
                sRdkWindowManagerKeyMap[keyCode] = keyMap;
              }
              else
              {
                RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "Ignoring keycode entry because of format issues in mapped params");
                continue;
              }
            }
            else
            {
              RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "Ignoring keycode entry because of format issues of keycode entry value");
              continue;
            }
          }
        }
      }
      else
      {
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "Ignored file read due to keyMappings entry not present");
      }
    }
    else
    {
      RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "Ignored file read due to keyMap env not set");
    }
}

void mapVirtualKeyCodes()
{
  //populate from the key map file
  const char* virtualKeyMapFile = getenv("RDK_WINDOW_MANAGER_VIRTUAL_KEYMAP_FILE");
  if (virtualKeyMapFile)
  {
    rapidjson::Document document;
    bool ret = RdkWindowManager::RdkWindowManagerJson::readJsonFile(virtualKeyMapFile, document);
    if (false == ret)
    {
      std::cout << "RDKWindowManager virtual keymap read error : [unable to open/read file (" <<  virtualKeyMapFile << ")]\n";
      return;
    }

    if (document.HasMember("virtualKeys"))
    {
      const rapidjson::Value& jsonValue = document["virtualKeys"];

      if (jsonValue.IsArray())
      {
        for (rapidjson::SizeType k = 0; k < jsonValue.Size(); k++)
        {
          std::string key;
          uint32_t mappedKeyCode = 0;
          uint32_t flags = 0;

          const rapidjson::Value& mapEntry = jsonValue[k];
          if (mapEntry.IsObject() && mapEntry.HasMember("key")  && mapEntry.HasMember("keyCode") && mapEntry.HasMember("modifiers"))
          {
            const rapidjson::Value& keyValue = mapEntry["key"];
            if (keyValue.IsString())
            {
              key = keyValue.GetString();
            }
            else
            {
              std::cout << "Ignoring key entry because of format issues of key\n";
              continue;
            }

            const rapidjson::Value& mappedKeyCodeValue = mapEntry["keyCode"];
            if (mappedKeyCodeValue.IsUint())
            {
              mappedKeyCode = mappedKeyCodeValue.GetUint();
            }
            else
            {
              std::cout << "Ignoring keycode entry because of format issues of mapped keycode\n";
              continue;
            }

            const rapidjson::Value& modifiersValue = mapEntry["modifiers"];
            if (modifiersValue.IsArray()) {
              for (rapidjson::SizeType i = 0; i < modifiersValue.Size(); i++)
              {
                if (modifiersValue[i].IsString()) {
                  flags |= getKeyFlag(modifiersValue[i].GetString());
                }
              }
            }

            struct RdkWindowManagerKeyMap keyMap;
            keyMap.code = mappedKeyCode;
            keyMap.flags = flags;
            sRdkWindowManagerVirtualKeyMap[key] = keyMap;
          }
          else
          {
            std::cout << "Ignoring entry because of missing key/keycode/modifiers params\n";
            continue;
          }
        }
      }
      else
      {
        std::cout << "Ignoring keycode entry because of format issues of virtualkeys \n";
      }
    }
    else
    {
      std::cout << "Ignored file read due to virtualKeys entry not present";
    }
  }
  else
  {
    std::cout << "Ignored file read due to virtual keyMap env not set\n";
  }
}

bool keyCodeFromWayland(uint32_t waylandKeyCode, uint32_t waylandFlags, uint32_t &mappedKeyCode, uint32_t &mappedFlags)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "key event - keyCode: %u flags: %u", waylandKeyCode, waylandFlags);
    std::map<uint32_t, struct RdkWindowManagerKeyMap>::iterator it  = sRdkWindowManagerKeyMap.find(waylandKeyCode);
    if (it != sRdkWindowManagerKeyMap.end())
    {
      mappedKeyCode = it->second.code;
      mappedFlags = it->second.flags;
      RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "key mapped from config - mappedKeyCode: %u mappedFlags: %u", mappedKeyCode, mappedFlags);
      return true;
    }
    int standardKeyCode = 0;
    switch (waylandKeyCode)
    {
    case WAYLAND_KEY_ENTER:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ENTER;
        break;
    case WAYLAND_KEY_BACKSPACE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_BACKSPACE;
        break;
    case WAYLAND_KEY_TAB:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_TAB;
        break;
    case WAYLAND_KEY_RIGHTSHIFT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SHIFT;
        break;
    case WAYLAND_KEY_LEFTSHIFT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SHIFT;
        break;
    case WAYLAND_KEY_RIGHTCTRL:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_CTRL;
        break;
    case WAYLAND_KEY_LEFTCTRL:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_CTRL;
        break;
    case WAYLAND_KEY_RIGHTALT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ALT;
        break;
    case WAYLAND_KEY_LEFTALT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ALT;
        break;
    case WAYLAND_KEY_PAUSE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PAUSE;
        break;
    case WAYLAND_KEY_CAPSLOCK:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_CAPSLOCK;
        break;
    case WAYLAND_KEY_ESC:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ESCAPE;
        break;
    case WAYLAND_KEY_SPACE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SPACE;
        break;
    case WAYLAND_KEY_PAGEUP:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PAGEUP;
        break;
    case WAYLAND_KEY_PAGEDOWN:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PAGEDOWN;
        break;
    case WAYLAND_KEY_END:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_END;
        break;
    case WAYLAND_KEY_HOME:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_HOME;
        break;
    case WAYLAND_KEY_LEFT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_LEFT;
        break;
    case WAYLAND_KEY_UP:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_UP;
        break;
    case WAYLAND_KEY_RIGHT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_RIGHT;
        break;
    case WAYLAND_KEY_DOWN:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_DOWN;
        break;
    case WAYLAND_KEY_COMMA:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_COMMA;
        break;
    case WAYLAND_KEY_DOT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PERIOD;
        break;
    case WAYLAND_KEY_SLASH:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_FORWARDSLASH;
        break;
    case WAYLAND_KEY_0:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ZERO;
        break;
    case WAYLAND_KEY_1:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ONE;
        break;
    case WAYLAND_KEY_2:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_TWO;
        break;
    case WAYLAND_KEY_3:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_THREE;
        break;
    case WAYLAND_KEY_4:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_FOUR;
        break;
    case WAYLAND_KEY_5:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_FIVE;
        break;
    case WAYLAND_KEY_6:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SIX;
        break;
    case WAYLAND_KEY_7:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SEVEN;
        break;
    case WAYLAND_KEY_8:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_EIGHT;
        break;
    case WAYLAND_KEY_9:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NINE;
        break;
    case WAYLAND_KEY_SEMICOLON:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SEMICOLON;
        break;
    case WAYLAND_KEY_EQUAL:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_EQUALS;
        break;
    case WAYLAND_KEY_A:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_A;
        break;
    case WAYLAND_KEY_B:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_B;
        break;
    case WAYLAND_KEY_C:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_C;
        break;
    case WAYLAND_KEY_D:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_D;
        break;
    case WAYLAND_KEY_E:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_E;
        break;
    case WAYLAND_KEY_F:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F;
        break;
    case WAYLAND_KEY_G:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_G;
        break;
    case WAYLAND_KEY_H:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_H;
        break;
    case WAYLAND_KEY_I:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_I;
        break;
    case WAYLAND_KEY_J:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_J;
        break;
    case WAYLAND_KEY_K:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_K;
        break;
    case WAYLAND_KEY_L:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_L;
        break;
    case WAYLAND_KEY_M:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_M;
        break;
    case WAYLAND_KEY_N:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_N;
        break;
    case WAYLAND_KEY_O:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_O;
        break;
    case WAYLAND_KEY_P:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_P;
        break;
    case WAYLAND_KEY_Q:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_Q;
        break;
    case WAYLAND_KEY_R:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_R;
        break;
    case WAYLAND_KEY_S:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_S;
        break;
    case WAYLAND_KEY_T:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_T;
        break;
    case WAYLAND_KEY_U:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_U;
        break;
    case WAYLAND_KEY_V:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_V;
        break;
    case WAYLAND_KEY_W:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_W;
        break;
    case WAYLAND_KEY_X:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_X;
        break;
    case WAYLAND_KEY_Y:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_Y;
        break;
    case WAYLAND_KEY_Z:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_Z;
        break;
    case WAYLAND_KEY_LEFTBRACE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_OPENBRACKET;
        break;
    case WAYLAND_KEY_BACKSLASH:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_BACKSLASH;
        break;
    case WAYLAND_KEY_RIGHTBRACE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_CLOSEBRACKET;
        break;
    case WAYLAND_KEY_KP0:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD0;
        break;
    case WAYLAND_KEY_KP1:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD1;
        break;
    case WAYLAND_KEY_KP2:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD2;
        break;
    case WAYLAND_KEY_KP3:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD3;
        break;
    case WAYLAND_KEY_KP4:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD4;
        break;
    case WAYLAND_KEY_KP5:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD5;
        break;
    case WAYLAND_KEY_KP6:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD6;
        break;
    case WAYLAND_KEY_KP7:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD7;
        break;
    case WAYLAND_KEY_KP8:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD8;
        break;
    case WAYLAND_KEY_KP9:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_NUMPAD9;
        break;
    case WAYLAND_KEY_KPASTERISK:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_MULTIPLY;
        break;
    case WAYLAND_KEY_KPPLUS:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ADD;
        break;
    case WAYLAND_KEY_KPMINUS:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SUBTRACT;
        break;
    case WAYLAND_KEY_KPDOT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_DECIMAL;
        break;
    case WAYLAND_KEY_KPSLASH:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_DIVIDE;
        break;
    case WAYLAND_KEY_F1:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F1;
        break;
    case WAYLAND_KEY_F2:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F2;
        break;
    case WAYLAND_KEY_F3:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F3;
        break;
    case WAYLAND_KEY_F4:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F4;
        break;
    case WAYLAND_KEY_F5:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F5;
        break;
    case WAYLAND_KEY_F6:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F6;
        break;
    case WAYLAND_KEY_F7:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F7;
        break;
    case WAYLAND_KEY_F8:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F8;
        break;
    case WAYLAND_KEY_F9:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F9;
        break;
    case WAYLAND_KEY_F10:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F10;
        break;
    case WAYLAND_KEY_F11:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F11;
        break;
    case WAYLAND_KEY_F12:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F12;
        break;
    case WAYLAND_KEY_DELETE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_DELETE;
        break;
    case WAYLAND_KEY_SCROLLLOCK:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_SCROLLLOCK;
        break;
    case WAYLAND_KEY_PRINT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PRINTSCREEN;
        break;
    case WAYLAND_KEY_INSERT:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_INSERT;
        break;
    case WAYLAND_KEY_MUTE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_MUTE;
        break;
    case WAYLAND_KEY_VOLUME_DOWN:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_VOLUME_DOWN;
        break;
    case WAYLAND_KEY_VOLUME_UP:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_VOLUME_UP;
        break;
    
    #ifdef WAYLAND_KEY_PLAYPAUSE
    case WAYLAND_KEY_PLAYPAUSE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PLAYPAUSE;
        break;
    #endif /* WAYLAND_KEY_PLAYPAUSE */

    #ifdef WAYLAND_KEY_PLAY
    case WAYLAND_KEY_PLAY:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_PLAY;
        break;
    #endif /* WAYLAND_KEY_PLAY */

    #ifdef WAYLAND_KEY_FASTFORWARD
    case WAYLAND_KEY_FASTFORWARD:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_FASTFORWARD;
        break;
    #endif /* RDK_WINDOW_MANAGER_KEY_FASTFORWARD  */

    #ifdef WAYLAND_KEY_REWIND
    case WAYLAND_KEY_REWIND:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_REWIND;
        break;
    #endif /* WAYLAND_KEY_REWIND */

    #ifdef WAYLAND_KEY_KPENTER
    case WAYLAND_KEY_KPENTER:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_ENTER;
        break;
    #endif /* WAYLAND_KEY_KPENTER */

    #ifdef WAYLAND_KEY_BACK
    case WAYLAND_KEY_BACK:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_BACK;
        break;
    #endif /* WAYLAND_KEY_BACK */

    #ifdef WAYLAND_KEY_MENU
    case WAYLAND_KEY_MENU:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_MENU;
        break;
    #endif /* WAYLAND_KEY_MENU */

    #ifdef WAYLAND_KEY_HOMEPAGE
    case WAYLAND_KEY_HOMEPAGE:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_HOMEPAGE;
        break;
    #endif /* WAYLAND_KEY_HOMEPAGE */

   case WAYLAND_KEY_F13:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F13;
        break;
   case WAYLAND_KEY_F14:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F14;
        break;
   case WAYLAND_KEY_F15:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F15;
        break;
    case WAYLAND_KEY_F16:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F16;
        break;
   case WAYLAND_KEY_F17:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F17;
        break;
   case WAYLAND_KEY_F18:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F18;
        break;
   case WAYLAND_KEY_F19:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F19;
        break;
// Key WAYLAND_KEY_F20 is reserved for RDK FP Power key
//   case WAYLAND_KEY_F20:
//        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F20;
//        break;
   case WAYLAND_KEY_F21:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F21;
        break;
    case WAYLAND_KEY_F22:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F22;
        break;
   case WAYLAND_KEY_F23:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F23;
        break;
   case WAYLAND_KEY_F24:
        standardKeyCode = RDK_WINDOW_MANAGER_KEY_F24;
        break;
    default:
        RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "unknown key code %u", waylandKeyCode);
        standardKeyCode = waylandKeyCode;
        break;
    }
    mappedKeyCode = standardKeyCode;
    mappedFlags = waylandFlags;
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "key mapped - mappedKeyCode: %u mappedFlags: %u", mappedKeyCode, mappedFlags);
    return true;
}

bool keyCodeFromVirtual(std::string& virtualKey, uint32_t &mappedKeyCode, uint32_t &mappedFlags)
{
    RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "virtual key event - key: %s", virtualKey.c_str());
    std::map<std::string, struct RdkWindowManagerKeyMap>::iterator it  = sRdkWindowManagerVirtualKeyMap.find(virtualKey);
    if (it != sRdkWindowManagerVirtualKeyMap.end())
    {
      mappedKeyCode = it->second.code;
      mappedFlags = it->second.flags;
      RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Debug, "virtaul key mapped from config - mappedKeyCode: %u mappedFlags: %u", mappedKeyCode, mappedFlags);
      return true;
    }
    return false;
}

uint32_t keyCodeToWayland(uint32_t keyCode)
{
    uint32_t  waylandKeyCode = 0;

   switch( keyCode )
   {
      case RDK_WINDOW_MANAGER_KEY_BACKSPACE:
         waylandKeyCode = WAYLAND_KEY_BACKSPACE;
         break;
      case RDK_WINDOW_MANAGER_KEY_TAB:
         waylandKeyCode = WAYLAND_KEY_TAB;
         break;
      case RDK_WINDOW_MANAGER_KEY_ENTER:
         waylandKeyCode = WAYLAND_KEY_ENTER;
         break;
      case RDK_WINDOW_MANAGER_KEY_SHIFT:
         waylandKeyCode = WAYLAND_KEY_LEFTSHIFT;
         break;
      case RDK_WINDOW_MANAGER_KEY_CTRL:
         waylandKeyCode = WAYLAND_KEY_LEFTCTRL;
         break;
      case RDK_WINDOW_MANAGER_KEY_ALT:
         waylandKeyCode = WAYLAND_KEY_LEFTALT;
         break;
      case RDK_WINDOW_MANAGER_KEY_CAPSLOCK:
         waylandKeyCode = WAYLAND_KEY_CAPSLOCK;
         break;
      case RDK_WINDOW_MANAGER_KEY_ESCAPE:
         waylandKeyCode = WAYLAND_KEY_ESC;
         break;
      case RDK_WINDOW_MANAGER_KEY_SPACE:
         waylandKeyCode = WAYLAND_KEY_SPACE;
         break;
      case RDK_WINDOW_MANAGER_KEY_PAGEUP:
         waylandKeyCode = WAYLAND_KEY_PAGEUP;
         break;
      case RDK_WINDOW_MANAGER_KEY_PAGEDOWN:
         waylandKeyCode = WAYLAND_KEY_PAGEDOWN;
         break;
      case RDK_WINDOW_MANAGER_KEY_END:
         waylandKeyCode = WAYLAND_KEY_END;
         break;
      case RDK_WINDOW_MANAGER_KEY_HOME:
         waylandKeyCode = WAYLAND_KEY_HOME;
         break;
      case RDK_WINDOW_MANAGER_KEY_LEFT:
         waylandKeyCode = WAYLAND_KEY_LEFT;
         break;
      case RDK_WINDOW_MANAGER_KEY_UP:
         waylandKeyCode = WAYLAND_KEY_UP;
         break;
      case RDK_WINDOW_MANAGER_KEY_RIGHT:
         waylandKeyCode = WAYLAND_KEY_RIGHT;
         break;
      case RDK_WINDOW_MANAGER_KEY_DOWN:
         waylandKeyCode = WAYLAND_KEY_DOWN;
         break;
      case RDK_WINDOW_MANAGER_KEY_INSERT:
         waylandKeyCode = WAYLAND_KEY_INSERT;
         break;
      case RDK_WINDOW_MANAGER_KEY_DELETE:
         waylandKeyCode = WAYLAND_KEY_DELETE;
         break;
      case RDK_WINDOW_MANAGER_KEY_ZERO:
         waylandKeyCode = WAYLAND_KEY_0;
         break;
      case RDK_WINDOW_MANAGER_KEY_ONE:
         waylandKeyCode = WAYLAND_KEY_1;
         break;
      case RDK_WINDOW_MANAGER_KEY_TWO:
         waylandKeyCode = WAYLAND_KEY_2;
         break;
      case RDK_WINDOW_MANAGER_KEY_THREE:
         waylandKeyCode = WAYLAND_KEY_3;
         break;
      case RDK_WINDOW_MANAGER_KEY_FOUR:
         waylandKeyCode = WAYLAND_KEY_4;
         break;
      case RDK_WINDOW_MANAGER_KEY_FIVE:
         waylandKeyCode = WAYLAND_KEY_5;
         break;
      case RDK_WINDOW_MANAGER_KEY_SIX:
         waylandKeyCode = WAYLAND_KEY_6;
         break;
      case RDK_WINDOW_MANAGER_KEY_SEVEN:
         waylandKeyCode = WAYLAND_KEY_7;
         break;
      case RDK_WINDOW_MANAGER_KEY_EIGHT:
         waylandKeyCode = WAYLAND_KEY_8;
         break;
      case RDK_WINDOW_MANAGER_KEY_NINE:
         waylandKeyCode = WAYLAND_KEY_9;
         break;
      case RDK_WINDOW_MANAGER_KEY_A:
         waylandKeyCode = WAYLAND_KEY_A;
         break;
      case RDK_WINDOW_MANAGER_KEY_B:
         waylandKeyCode = WAYLAND_KEY_B;
         break;
      case RDK_WINDOW_MANAGER_KEY_C:
         waylandKeyCode = WAYLAND_KEY_C;
         break;
      case RDK_WINDOW_MANAGER_KEY_D:
         waylandKeyCode = WAYLAND_KEY_D;
         break;
      case RDK_WINDOW_MANAGER_KEY_E:
         waylandKeyCode = WAYLAND_KEY_E;
         break;
      case RDK_WINDOW_MANAGER_KEY_F:
         waylandKeyCode = WAYLAND_KEY_F;
         break;
      case RDK_WINDOW_MANAGER_KEY_G:
         waylandKeyCode = WAYLAND_KEY_G;
         break;
      case RDK_WINDOW_MANAGER_KEY_H:
         waylandKeyCode = WAYLAND_KEY_H;
         break;
      case RDK_WINDOW_MANAGER_KEY_I:
         waylandKeyCode = WAYLAND_KEY_I;
         break;
      case RDK_WINDOW_MANAGER_KEY_J:
         waylandKeyCode = WAYLAND_KEY_J;
         break;
      case RDK_WINDOW_MANAGER_KEY_K:
         waylandKeyCode = WAYLAND_KEY_K;
         break;
      case RDK_WINDOW_MANAGER_KEY_L:
         waylandKeyCode = WAYLAND_KEY_L;
         break;
      case RDK_WINDOW_MANAGER_KEY_M:
         waylandKeyCode = WAYLAND_KEY_M;
         break;
      case RDK_WINDOW_MANAGER_KEY_N:
         waylandKeyCode = WAYLAND_KEY_N;
         break;
      case RDK_WINDOW_MANAGER_KEY_O:
         waylandKeyCode = WAYLAND_KEY_O;
         break;
      case RDK_WINDOW_MANAGER_KEY_P:
         waylandKeyCode = WAYLAND_KEY_P;
         break;
      case RDK_WINDOW_MANAGER_KEY_Q:
         waylandKeyCode = WAYLAND_KEY_Q;
         break;
      case RDK_WINDOW_MANAGER_KEY_R:
         waylandKeyCode = WAYLAND_KEY_R;
         break;
      case RDK_WINDOW_MANAGER_KEY_S:
         waylandKeyCode = WAYLAND_KEY_S;
         break;
      case RDK_WINDOW_MANAGER_KEY_T:
         waylandKeyCode = WAYLAND_KEY_T;
         break;
      case RDK_WINDOW_MANAGER_KEY_U:
         waylandKeyCode = WAYLAND_KEY_U;
         break;
      case RDK_WINDOW_MANAGER_KEY_V:
         waylandKeyCode = WAYLAND_KEY_V;
         break;
      case RDK_WINDOW_MANAGER_KEY_W:
         waylandKeyCode = WAYLAND_KEY_W;
         break;
      case RDK_WINDOW_MANAGER_KEY_X:
         waylandKeyCode = WAYLAND_KEY_X;
         break;
      case RDK_WINDOW_MANAGER_KEY_Y:
         waylandKeyCode = WAYLAND_KEY_Y;
         break;
      case RDK_WINDOW_MANAGER_KEY_Z:
         waylandKeyCode = WAYLAND_KEY_Z;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD0:
         waylandKeyCode = WAYLAND_KEY_KP0;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD1:
         waylandKeyCode = WAYLAND_KEY_KP1;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD2:
         waylandKeyCode = WAYLAND_KEY_KP2;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD3:
         waylandKeyCode = WAYLAND_KEY_KP3;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD4:
         waylandKeyCode = WAYLAND_KEY_KP4;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD5:
         waylandKeyCode = WAYLAND_KEY_KP5;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD6:
         waylandKeyCode = WAYLAND_KEY_KP6;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD7:
         waylandKeyCode = WAYLAND_KEY_KP7;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD8:
         waylandKeyCode = WAYLAND_KEY_KP8;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMPAD9:
         waylandKeyCode = WAYLAND_KEY_KP9;
         break;
      case RDK_WINDOW_MANAGER_KEY_MULTIPLY:
         waylandKeyCode = WAYLAND_KEY_KPASTERISK;
         break;
      case RDK_WINDOW_MANAGER_KEY_ADD:
         waylandKeyCode = WAYLAND_KEY_KPPLUS;
         break;
      case RDK_WINDOW_MANAGER_KEY_SUBTRACT:
         waylandKeyCode = WAYLAND_KEY_KPMINUS;
         break;
      case RDK_WINDOW_MANAGER_KEY_DECIMAL:
         waylandKeyCode = WAYLAND_KEY_KPDOT;
         break;
      case RDK_WINDOW_MANAGER_KEY_DIVIDE:
         waylandKeyCode = WAYLAND_KEY_KPSLASH;
         break;
      case RDK_WINDOW_MANAGER_KEY_F1:
         waylandKeyCode = WAYLAND_KEY_F1;
         break;
      case RDK_WINDOW_MANAGER_KEY_F2:
         waylandKeyCode = WAYLAND_KEY_F2;
         break;
      case RDK_WINDOW_MANAGER_KEY_F3:
         waylandKeyCode = WAYLAND_KEY_F3;
         break;
      case RDK_WINDOW_MANAGER_KEY_F4:
         waylandKeyCode = WAYLAND_KEY_F4;
         break;
      case RDK_WINDOW_MANAGER_KEY_F5:
         waylandKeyCode = WAYLAND_KEY_F5;
         break;
      case RDK_WINDOW_MANAGER_KEY_F6:
         waylandKeyCode = WAYLAND_KEY_F6;
         break;
      case RDK_WINDOW_MANAGER_KEY_F7:
         waylandKeyCode = WAYLAND_KEY_F7;
         break;
      case RDK_WINDOW_MANAGER_KEY_F8:
         waylandKeyCode = WAYLAND_KEY_F8;
         break;
      case RDK_WINDOW_MANAGER_KEY_F9:
         waylandKeyCode = WAYLAND_KEY_F9;
         break;
      case RDK_WINDOW_MANAGER_KEY_F10:
         waylandKeyCode = WAYLAND_KEY_F10;
         break;
      case RDK_WINDOW_MANAGER_KEY_F11:
         waylandKeyCode = WAYLAND_KEY_F11;
         break;
      case RDK_WINDOW_MANAGER_KEY_F12:
         waylandKeyCode = WAYLAND_KEY_F12;
         break;
      case RDK_WINDOW_MANAGER_KEY_NUMLOCK:
         waylandKeyCode = WAYLAND_KEY_NUMLOCK;
         break;
      case RDK_WINDOW_MANAGER_KEY_SCROLLLOCK:
         waylandKeyCode = WAYLAND_KEY_SCROLLLOCK;
         break;
      case RDK_WINDOW_MANAGER_KEY_SEMICOLON:
         waylandKeyCode = WAYLAND_KEY_SEMICOLON;
         break;
      case RDK_WINDOW_MANAGER_KEY_EQUALS:
         waylandKeyCode = WAYLAND_KEY_EQUAL;
         break;
      case RDK_WINDOW_MANAGER_KEY_COMMA:
         waylandKeyCode = WAYLAND_KEY_COMMA;
         break;
      case RDK_WINDOW_MANAGER_KEY_PERIOD:
         waylandKeyCode = WAYLAND_KEY_DOT;
         break;
      case RDK_WINDOW_MANAGER_KEY_FORWARDSLASH:
         waylandKeyCode = WAYLAND_KEY_SLASH;
         break;
      case RDK_WINDOW_MANAGER_KEY_GRAVEACCENT:
         waylandKeyCode = WAYLAND_KEY_GRAVE;
         break;
      case RDK_WINDOW_MANAGER_KEY_OPENBRACKET:
         waylandKeyCode = WAYLAND_KEY_LEFTBRACE;
         break;
      case RDK_WINDOW_MANAGER_KEY_BACKSLASH:
         waylandKeyCode = WAYLAND_KEY_BACKSLASH;
         break;
      case RDK_WINDOW_MANAGER_KEY_CLOSEBRACKET:
         waylandKeyCode = WAYLAND_KEY_RIGHTBRACE;
         break;
      case RDK_WINDOW_MANAGER_KEY_SINGLEQUOTE:
         waylandKeyCode = WAYLAND_KEY_APOSTROPHE;
         break;
      case RDK_WINDOW_MANAGER_KEY_PRINTSCREEN:
         waylandKeyCode = WAYLAND_KEY_PRINT;
         break;
      case RDK_WINDOW_MANAGER_KEY_DASH:
         waylandKeyCode = WAYLAND_KEY_MINUS;
         break;
      case RDK_WINDOW_MANAGER_KEY_FASTFORWARD:
         waylandKeyCode = WAYLAND_KEY_FASTFORWARD;
         break;
      case RDK_WINDOW_MANAGER_KEY_REWIND:
         waylandKeyCode = WAYLAND_KEY_REWIND;
         break;
      case RDK_WINDOW_MANAGER_KEY_PAUSE:
         waylandKeyCode = WAYLAND_KEY_PAUSE;
         break;
      case RDK_WINDOW_MANAGER_KEY_PLAY:
         waylandKeyCode = WAYLAND_KEY_PLAY;
         break;
      case RDK_WINDOW_MANAGER_KEY_PLAYPAUSE:
         waylandKeyCode = WAYLAND_KEY_PLAYPAUSE;
         break;
      case RDK_WINDOW_MANAGER_KEY_YELLOW:
         waylandKeyCode = WAYLAND_KEY_YELLOW;
         break;
      case RDK_WINDOW_MANAGER_KEY_BLUE:
         waylandKeyCode = WAYLAND_KEY_BLUE;
         break;
      case RDK_WINDOW_MANAGER_KEY_RED:
         waylandKeyCode = WAYLAND_KEY_RED;
         break;
      case RDK_WINDOW_MANAGER_KEY_GREEN:
         waylandKeyCode = WAYLAND_KEY_GREEN;
         break;
      case RDK_WINDOW_MANAGER_KEY_BACK:
         waylandKeyCode = WAYLAND_KEY_BACK;
         break;
      case RDK_WINDOW_MANAGER_KEY_MENU:
         waylandKeyCode = WAYLAND_KEY_MENU;
         break;
      case RDK_WINDOW_MANAGER_KEY_HOMEPAGE:
         waylandKeyCode = WAYLAND_KEY_HOMEPAGE;
         break;
      case RDK_WINDOW_MANAGER_KEY_MUTE:
         waylandKeyCode = WAYLAND_KEY_MUTE;
         break;
      case RDK_WINDOW_MANAGER_KEY_VOLUME_DOWN:
         waylandKeyCode = WAYLAND_KEY_VOLUME_DOWN;
         break;
      case RDK_WINDOW_MANAGER_KEY_VOLUME_UP:
         waylandKeyCode = WAYLAND_KEY_VOLUME_UP;
         break;
      case RDK_WINDOW_MANAGER_KEY_F13:
         waylandKeyCode = WAYLAND_KEY_F13;
         break;
      case RDK_WINDOW_MANAGER_KEY_F14:
         waylandKeyCode = WAYLAND_KEY_F14;
         break;
      case RDK_WINDOW_MANAGER_KEY_F15:
         waylandKeyCode = WAYLAND_KEY_F15;
         break;
      case RDK_WINDOW_MANAGER_KEY_F16:
         waylandKeyCode = WAYLAND_KEY_F16;
         break;
      case RDK_WINDOW_MANAGER_KEY_F17:
         waylandKeyCode = WAYLAND_KEY_F17;
         break;
      case RDK_WINDOW_MANAGER_KEY_F18:
         waylandKeyCode = WAYLAND_KEY_F18;
         break;
      case RDK_WINDOW_MANAGER_KEY_F19:
         waylandKeyCode = WAYLAND_KEY_F19;
         break;
//       case RDK_WINDOW_MANAGER_KEY_F20:
//         waylandKeyCode = WAYLAND_KEY_F20;
//         break;
       case RDK_WINDOW_MANAGER_KEY_F21:
         waylandKeyCode = WAYLAND_KEY_F21;
         break;
      case RDK_WINDOW_MANAGER_KEY_F22:
         waylandKeyCode = WAYLAND_KEY_F22;
         break;
      case RDK_WINDOW_MANAGER_KEY_F23:
         waylandKeyCode = WAYLAND_KEY_F23;
         break;
      case RDK_WINDOW_MANAGER_KEY_F24:
         waylandKeyCode = WAYLAND_KEY_F24;
         break;
      /* BLE RemoteUnit outbound (after /etc/rcu_keymap.json inbound) */
      case RDK_WINDOW_MANAGER_KEY_DEV_POWER:
         waylandKeyCode = WAYLAND_KEY_POWER;
         break;
      case RDK_WINDOW_MANAGER_KEY_SEARCH:
         waylandKeyCode = WAYLAND_KEY_SEARCH;
         break;
      case RDK_WINDOW_MANAGER_KEY_PROGRAM:
         waylandKeyCode = WAYLAND_KEY_PROGRAM;
         break;
      case RDK_WINDOW_MANAGER_KEY_CONTEXT_MENU:
         waylandKeyCode = WAYLAND_KEY_CONTEXT_MENU;
         break;
      case RDK_WINDOW_MANAGER_KEY_TV:
         waylandKeyCode = WAYLAND_KEY_TV;
         break;
      case RDK_WINDOW_MANAGER_KEY_PROFILE:
         waylandKeyCode = WAYLAND_KEY_PROFILE;
         break;
      case RDK_WINDOW_MANAGER_KEY_RECORD:
         waylandKeyCode = WAYLAND_KEY_RECORD;
         break;
      default:
         RdkWindowManager::Logger::log(RdkWindowManager::LogLevel::Information,  "common key code not found %d",keyCode);
         waylandKeyCode= -1;
         break;
   }

   return  waylandKeyCode;
 }
