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

#include <iostream>
#include <GLES2/gl2.h>
#ifdef RDK_WINDOW_MANAGER_ENABLE_IPC
#include "servermessagehandler.h"
#endif

#include "essosinstance.h"
#include "compositorcontroller.h"
#include "linuxkeys.h"
#include "eastereggs.h"
#include "linuxinput.h"
#include "animation.h"
#include "logger.h"
#include "rdkwindowmanager.h"
#include "rdkwindowmanagerimage.h"
#include "permissions.h"
#include <unistd.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <thread>
#include <cstring>

#define RDK_WINDOW_MANAGER_FPS 40

#define RDK_WINDOW_MANAGER_RAM_MONITOR_INTERVAL_SECONDS 1
#define RDK_WINDOW_MANAGER_DEFAULT_LOW_MEMORY_THRESHOLD_MB 200
#define RDK_WINDOW_MANAGER_DEFAULT_CRITICALLY_LOW_MEMORY_THRESHOLD_MB 100
#define RDK_WINDOW_MANAGER_DEFAULT_SWAP_INCREASE_THRESHOLD_MB 50
#define RDK_WINDOW_MANAGER_SPLASH_SCREEN_FILE_CHECK "/tmp/.rdkwindowmanagersplash"

int gCurrentFramerate = RDK_WINDOW_MANAGER_FPS;
bool gRdkWindowManagerIsRunning = false;

bool gEnableRamMonitor = true;
double gRamMonitorIntervalInSeconds = RDK_WINDOW_MANAGER_RAM_MONITOR_INTERVAL_SECONDS;
double gLowRamMemoryThresholdInMb =  RDK_WINDOW_MANAGER_DEFAULT_LOW_MEMORY_THRESHOLD_MB;
double gCriticallyLowRamMemoryThresholdInMb = RDK_WINDOW_MANAGER_DEFAULT_CRITICALLY_LOW_MEMORY_THRESHOLD_MB;
double gSwapMemoryIncreaseThresoldInMb =  RDK_WINDOW_MANAGER_DEFAULT_SWAP_INCREASE_THRESHOLD_MB;

bool gLowRamMemoryNotificationSent = false;
bool gCriticallyLowRamMemoryNotificationSent = false;
bool gForce720 = false;

#ifdef RDK_WINDOW_MANAGER_ENABLE_IPC
std::shared_ptr<RdkWindowManager::ServerMessageHandler> gServerMessageHandler;
bool gIpcEnabled = false;
#endif

std::thread gMemoryMonitorThread;
bool gRunMemoryMonitor = true;
std::mutex gMemoryMonitorMutex;

namespace RdkWindowManager
{

    double seconds()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec + ((double)ts.tv_nsec/1000000000);
    }

    double milliseconds()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ((double)(ts.tv_sec * 1000) + ((double)ts.tv_nsec/1000000));
    }

    double microseconds()
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ((double)(ts.tv_sec * 1000000) + ((double)ts.tv_nsec/1000));
    }

    bool systemRam(uint32_t& freeKb, uint32_t& totalKb, uint32_t& usedSwapKb)
    {
        uint32_t availableKb = 0;
	bool ret = systemRam(freeKb, totalKb, availableKb, usedSwapKb);
        return ret;
    }

    bool systemRam(uint32_t& freeKb, uint32_t& totalKb, uint32_t& availableKb, uint32_t& usedSwapKb)
    {
        struct sysinfo systemInformation;
        int ret = sysinfo(&systemInformation);
        uint64_t freeMemKb=0, usedSwapMemKb=0, totalMemKb=0;

        if (0 != ret)
        {
            Logger::log(Debug, "failed to get memory details");
            return false;
        }
	totalMemKb = (systemInformation.totalram * systemInformation.mem_unit)/1024;
        freeMemKb = (systemInformation.freeram * systemInformation.mem_unit)/1024;
        usedSwapMemKb = ((systemInformation.totalswap - systemInformation.freeswap) * systemInformation.mem_unit)/1024;
        totalKb = (uint32_t) totalMemKb;
        freeKb = (uint32_t) freeMemKb;
        usedSwapKb = (uint32_t) usedSwapMemKb;
        FILE* file = fopen("/proc/meminfo", "r");
        if (!file)
        {
            Logger::log(Debug, "failed to get memory details");
            return false;
        }
        char buffer[128];
        bool readMemory = false;
        int32_t availableMemory = -1;
        while (char* line = fgets(buffer, 128, file))
        {
            char* token = strtok(line, " ");
            if (!token)
            {
                break;
            }
            if (!strcmp(token, "MemAvailable:"))
            {
                if ((token = strtok(nullptr, " ")))
                {
                    readMemory = true;	
                    availableKb = atoll(token);
                }
                else
		{
                    Logger::log(Debug, "failed to get memory details");
                }
		break;
            }
        }
        if (!readMemory)
        {
            fclose(file);
            return false;
        }
        fclose(file);
        return true;
    }

    void setMemoryMonitor(const bool enable, const double interval)
    {
        gMemoryMonitorMutex.lock();
        gEnableRamMonitor = enable;
        gRamMonitorIntervalInSeconds = interval;
        gMemoryMonitorMutex.unlock();
    }

    void setMemoryMonitor(std::map<std::string, RdkWindowManagerData> &configuration)
    {
        gMemoryMonitorMutex.lock();
        for ( const auto &monitorConfiguration : configuration )
        {
            if (monitorConfiguration.first == "enable")
            {
                gEnableRamMonitor = monitorConfiguration.second.toBoolean();
            }
            else if (monitorConfiguration.first == "interval")
            {
                gRamMonitorIntervalInSeconds = monitorConfiguration.second.toDouble();
            }
            else if (monitorConfiguration.first == "lowRam")
            {
                gLowRamMemoryThresholdInMb = monitorConfiguration.second.toDouble();
            }
            else if (monitorConfiguration.first == "criticallyLowRam")
            {
                gCriticallyLowRamMemoryThresholdInMb = monitorConfiguration.second.toDouble();
            }
            else if (monitorConfiguration.first == "swapIncreaseLimit")
            {
                gSwapMemoryIncreaseThresoldInMb = monitorConfiguration.second.toDouble();
            }
        }
        if (gCriticallyLowRamMemoryThresholdInMb  > gLowRamMemoryThresholdInMb)
        {
            Logger::log(Warn, "criticial low ram threshold configuration is lower than low ram threshold");
            gCriticallyLowRamMemoryThresholdInMb = gLowRamMemoryThresholdInMb;
        }
        gMemoryMonitorMutex.unlock();
    }

    static void evaluateMemoryUsage(uint32_t& availableKb, uint32_t& usedSwapKb, float swapIncreaseMb, uint32_t freeKb)
    {
        float availableMb = availableKb/1024;
        std::vector<std::map<std::string, RdkWindowManagerData>> eventData(1);
        eventData[0] = std::map<std::string, RdkWindowManagerData>();
        eventData[0]["freeKb"] = freeKb;
        eventData[0]["availableKb"] = availableKb;
        eventData[0]["usedSwapKb"] = usedSwapKb;
        if ((availableMb < gLowRamMemoryThresholdInMb) || (swapIncreaseMb > gSwapMemoryIncreaseThresoldInMb))
        {
            if (!gLowRamMemoryNotificationSent)
            {
                CompositorController::sendEvent(RDK_WINDOW_MANAGER_EVENT_DEVICE_LOW_RAM_WARNING, eventData);
                gLowRamMemoryNotificationSent = true;
            }
            if ((!gCriticallyLowRamMemoryNotificationSent) && (availableMb < gCriticallyLowRamMemoryThresholdInMb))
            {
                  CompositorController::sendEvent(RDK_WINDOW_MANAGER_EVENT_DEVICE_CRITICALLY_LOW_RAM_WARNING, eventData);
                  gCriticallyLowRamMemoryNotificationSent = true;
            }
            else if ((gCriticallyLowRamMemoryNotificationSent) && (availableMb >= gCriticallyLowRamMemoryThresholdInMb))
            {
                CompositorController::sendEvent(RDK_WINDOW_MANAGER_EVENT_DEVICE_CRITICALLY_LOW_RAM_WARNING_CLEARED, eventData);
                gCriticallyLowRamMemoryNotificationSent = false;
            }
        }
        else
        {
            if (gCriticallyLowRamMemoryNotificationSent)
            {
                CompositorController::sendEvent(RDK_WINDOW_MANAGER_EVENT_DEVICE_CRITICALLY_LOW_RAM_WARNING_CLEARED, eventData);
                gCriticallyLowRamMemoryNotificationSent = false;
            }
            if (gLowRamMemoryNotificationSent)
            {
                CompositorController::sendEvent(RDK_WINDOW_MANAGER_EVENT_DEVICE_LOW_RAM_WARNING_CLEARED, eventData);
                gLowRamMemoryNotificationSent = false;
            }
        }
    }

    static void launchMemoryMonitorThread()
    {
        gMemoryMonitorThread = std::thread([=]()
        {
            bool runMemoryMonitor = gRunMemoryMonitor;
            float swap1=0, swap2=0, swap3=0, swap4=0, swap5=0;
            uint32_t usedSwapKb=0, availableKb=0, freeKb=0, totalKb=0;
            bool ret = systemRam(freeKb, totalKb, availableKb, usedSwapKb);
            float usedSwapMb = 0;
            if (ret)
            {
                usedSwapMb = usedSwapKb/1024;
                swap1=usedSwapMb;
                swap2=usedSwapMb;
                swap3=usedSwapMb;
                swap4=usedSwapMb;
                swap5=usedSwapMb;
	    }
            while (runMemoryMonitor)
            {
                gMemoryMonitorMutex.lock();
                int32_t ramMonitorIntervalInMs = gRamMonitorIntervalInSeconds;
                bool enableRamMonitor = gEnableRamMonitor;
                gMemoryMonitorMutex.unlock();
                ramMonitorIntervalInMs = ramMonitorIntervalInMs*1000*1000;
                if (enableRamMonitor)
                {
                    ret = systemRam(freeKb, totalKb, availableKb, usedSwapKb);
                    if (ret)
                    {
                        usedSwapMb = usedSwapKb/1024;
                        swap1=swap2;
                        swap2=swap3;
                        swap3=swap4;
                        swap4=swap5;
                        swap5=usedSwapMb;
                        evaluateMemoryUsage(availableKb, usedSwapKb, (swap5-swap1), freeKb);
                    }
                }
                usleep(ramMonitorIntervalInMs);
                gMemoryMonitorMutex.lock();
                runMemoryMonitor = gRunMemoryMonitor;
                gMemoryMonitorMutex.unlock();
	    }
        });
        gMemoryMonitorThread.detach();
    }

    void initialize()
    {
        Logger::log(LogLevel::Information, "initializing rdk window manager\n");

        mapNativeKeyCodes();
        mapVirtualKeyCodes();
        populateEasterEggDetails();
        readInputDevicesConfiguration();

        char const *loglevel = getenv("RDK_WINDOW_MANAGER_LOG_LEVEL");
        if (loglevel)
        {
            Logger::setLogLevel(loglevel);
        }

        char const *s = getenv("RDK_WINDOW_MANAGER_FRAMERATE");
        if (s)
        {
            int fps = atoi(s);
            if (fps > 0)
            {
                gCurrentFramerate = fps;
            }
        }

        char const *lowRamMemoryThresholdInMb = getenv("RDK_WINDOW_MANAGER_LOW_MEMORY_THRESHOLD");
        if (lowRamMemoryThresholdInMb)
        {
            double lowRamMemoryThresholdInMbValue = std::stod(lowRamMemoryThresholdInMb);
            if (lowRamMemoryThresholdInMbValue > 0)
            {
                gLowRamMemoryThresholdInMb = lowRamMemoryThresholdInMbValue;
            }
        }

        char const *criticalLowRamMemoryThresholdInMb = getenv("RDK_WINDOW_MANAGER_CRITICALLY_LOW_MEMORY_THRESHOLD");
        if (criticalLowRamMemoryThresholdInMb)
        {
            double criticalLowRamMemoryThresholdInMbValue = std::stod(criticalLowRamMemoryThresholdInMb);
            if (criticalLowRamMemoryThresholdInMbValue > 0)
            {
                if (criticalLowRamMemoryThresholdInMbValue  <= gLowRamMemoryThresholdInMb)
                {
                    gCriticallyLowRamMemoryThresholdInMb = criticalLowRamMemoryThresholdInMbValue;
                }
                else
                {
                    Logger::log(Warn, "criticial low ram threshold is lower than low ram threshold");
                    gCriticallyLowRamMemoryThresholdInMb = gLowRamMemoryThresholdInMb;
                }
            }
        }

        char const *swapIncreaseThresholdInMb = getenv("RDK_WINDOW_MANAGER_SWAP_MEMORY_INCREASE_THRESHOLD");
        if (swapIncreaseThresholdInMb)
        {
            double swapIncreaseThresholdInMbValue = std::stod(swapIncreaseThresholdInMb);
            if (swapIncreaseThresholdInMbValue > 0)
            {
                gSwapMemoryIncreaseThresoldInMb = swapIncreaseThresholdInMbValue;
            }
        }

        uint32_t initialKeyDelay = 500;
        char const *keyDelay = getenv("RDK_WINDOW_MANAGER_KEY_INITIAL_DELAY");
        if (keyDelay)
        {
            int value = atoi(keyDelay);
            if (value > 0)
            {
                initialKeyDelay = value;
            }
        }

        uint32_t repeatKeyInterval = 100;
        char const *repeatInterval = getenv("RDK_WINDOW_MANAGER_KEY_REPEAT_INTERVAL");
        if (repeatInterval)
        {
            int value = atoi(repeatInterval);
            if (value > 0)
            {
                repeatKeyInterval = value;
            }
        }

        RdkWindowManager::EssosInstance::instance()->configureKeyInput(initialKeyDelay, repeatKeyInterval);

        #ifdef RDK_WINDOW_MANAGER_ENABLE_IPC
        char const* ipcSetting = getenv("RDK_WINDOW_MANAGER_ENABLE_IPC");
        if (ipcSetting && (strcmp(ipcSetting,"1") == 0))
        {
            gIpcEnabled = true;
        }
        if (gIpcEnabled)
        {
            gServerMessageHandler = std::make_shared<RdkWindowManager::ServerMessageHandler>();
            gServerMessageHandler->start();
        }
        #endif

        #ifdef RDK_WINDOW_MANAGER_ENABLE_FORCE_1080
        char const* graphicsResolution720 = getenv("RDK_WINDOW_MANAGER_SET_GRAPHICS_720");
        if (graphicsResolution720 && (strcmp(graphicsResolution720,"1") == 0))
        {
            Logger::log(LogLevel::Information,  "RDK_WINDOW_MANAGER_SET_GRAPHICS_720 is set");
            gForce720 = true;
        }

        std::ifstream file720("/tmp/rdkwindowmanager720");
        if (file720.good() || gForce720)
        {
            Logger::log(LogLevel::Information,  "!!!!! forcing 720 start!");
            RdkWindowManager::EssosInstance::instance()->initialize(false, 1280, 720);
            gForce720 = true;
        }
        else
        {
            Logger::log(LogLevel::Information,  "!!!!! forcing 1080 start!");
            RdkWindowManager::EssosInstance::instance()->initialize(false, 1920, 1080);
        }
        #else
        RdkWindowManager::EssosInstance::instance()->initialize(false);
        #endif //RDK_WINDOW_MANAGER_ENABLE_FORCE_1080
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        char const *splashScreen = getenv("RDK_WINDOW_MANAGER_SHOW_SPLASH_SCREEN");
        if (splashScreen)
        {
            std::ifstream splashScreenFile(RDK_WINDOW_MANAGER_SPLASH_SCREEN_FILE_CHECK);
            bool showSplashScreen =  !splashScreenFile.good();

            char const *splashScreenDisableFile = getenv("RDK_WINDOW_MANAGER_DISABLE_SPLASH_SCREEN_FILE");
            if (splashScreenDisableFile)
            {
                std::ifstream splashScreenDisableFileHandle(splashScreenDisableFile);
                if (splashScreenDisableFileHandle.good())
                {
                    Logger::log(Warn, "not showing splash screen as disable splash screen file is present");
                    showSplashScreen = false;
                    std::ofstream output(RDK_WINDOW_MANAGER_SPLASH_SCREEN_FILE_CHECK);
                    splashScreenDisableFileHandle.close();
                    int32_t ret = std::remove(splashScreenDisableFile);
                    if (0 != ret)
                    {
                        Logger::log(Warn, "splash screen disable file remove failed");
                    }
                }
            }
            if (showSplashScreen)
            {
                uint32_t splashTime = 0;
                char const *splashTimeValue = getenv("RDK_WINDOW_MANAGER_SHOW_SPLASH_TIME_IN_SECONDS");
                if (splashTimeValue)
                {
                    int value = atoi(splashTimeValue);
                    if (value > 0)
                    {
                        splashTime = (uint32_t)(value);
                    }
                }
                CompositorController::showSplashScreen(splashTime);
                std::ofstream output(RDK_WINDOW_MANAGER_SPLASH_SCREEN_FILE_CHECK);
            }
            else
            {
                Logger::log(Warn, "splash screen will not be displayed since this is not first run since boot");
            }
        }

        CompositorController::initialize();
        launchMemoryMonitorThread();
    }

    void deinitialize()
    {
        gMemoryMonitorMutex.lock();
        gRunMemoryMonitor = false;
        gMemoryMonitorMutex.unlock();
    }

    void run()
    {
        gRdkWindowManagerIsRunning = true;
        while( gRdkWindowManagerIsRunning )
        {
            update();
            uint32_t width = 0;
            uint32_t height = 0;
            RdkWindowManager::EssosInstance::instance()->resolution(width, height);
            glViewport( 0, 0, width, height );
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            const double maxSleepTime = (1000 / gCurrentFramerate) * 1000;
            double startFrameTime = microseconds();
            RdkWindowManager::CompositorController::draw();
            RdkWindowManager::EssosInstance::instance()->update();

            double frameTime = (int)microseconds() - (int)startFrameTime;
            int32_t sleepTimeInMs = gCurrentFramerate - frameTime;
            if (frameTime < maxSleepTime)
            {
                int sleepTime = (int)maxSleepTime-(int)frameTime;
                usleep(sleepTime);
            }
        }
    }

    void draw()
    {
        RdkWindowManager::EssosInstance::instance()->update();
        uint32_t width = 0;
        uint32_t height = 0;
        RdkWindowManager::EssosInstance::instance()->resolution(width, height);
        glViewport( 0, 0, width, height );
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        RdkWindowManager::CompositorController::draw();
    }

    void update()
    {
        #ifdef RDK_WINDOW_MANAGER_ENABLE_IPC
        if (gIpcEnabled)
        {
            gServerMessageHandler->process();
        }
        #endif
        RdkWindowManager::CompositorController::update();
    }
}
