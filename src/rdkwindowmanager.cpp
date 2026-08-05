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

#include "essosinstance.h"
#include "compositorcontroller.h"
#include "linuxkeys.h"
#include "linuxinput.h"
#include "logger.h"
#include "rdkwindowmanager.h"
#include "rdkwindowmanagerimage.h"
#ifdef RDK_WINDOW_MANAGER_BUILD_SPLIT_SCREEN_POC
#include "splitscreenmanager.h"
#endif
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

    void initialize()
    {
        #ifdef RDK_WINDOW_MANAGER_LOGGER
        Logger::setLogFile(RDK_WINDOW_MANAGER_LOGFILE);
        #endif
        Logger::log(LogLevel::Information, "initializing rdk window manager\n");

        mapNativeKeyCodes();
        mapVirtualKeyCodes();
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

        CompositorController::initialize();
       //launchMemoryMonitorThread();
    }

    void deinitialize()
    {

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
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // opaque black: alpha=1 keeps the DRM plane fully visible
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
        uint32_t width = 0;
        uint32_t height = 0;
        RdkWindowManager::EssosInstance::instance()->resolution(width, height);
        glViewport( 0, 0, width, height );
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // opaque black: alpha=1 keeps the DRM plane fully visible
        glClear(GL_COLOR_BUFFER_BIT);

        RdkWindowManager::CompositorController::draw();
    }

    void present()
    {
        RdkWindowManager::EssosInstance::instance()->update();
    }

    void update()
    {
        RdkWindowManager::CompositorController::update();
#ifdef RDK_WINDOW_MANAGER_BUILD_SPLIT_SCREEN_POC
        SplitScreenManager::instance().update();
#endif
    }
}
