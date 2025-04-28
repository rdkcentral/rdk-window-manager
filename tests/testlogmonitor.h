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

#ifndef TESTLOG_MONITOR_H
#define TESTLOG_MONITOR_H

#include <string>
#include <mqueue.h>

// Structure for thread arguments
typedef struct _RdkTestLogMonitorConfig {
    std::string logFilePath;
}RdkTestLogMonitorConfig;

// Callback function type 
typedef bool (*RdkTestLogMonitorCallbackFunc)(void *context,void *userParam, const char *pattern_matched_line);

// Structure to hold callback and context
typedef struct _RdkTestLogMonitorSubscribeConfig  {
    RdkTestLogMonitorCallbackFunc callback; // Function pointer for the callback
    void *context; // Context to pass to the callback
    void *userParam;
}RdkTestLogMonitorSubscribeConfig;

// Function to subscribe to the log monitor
void* rdkTestLogMonitorSubscribe(RdkTestLogMonitorSubscribeConfig subscribeCfg);

// Function to unsubscribe from the log monitor
bool rdkTestLogMonitorUnsubscribe(void* handle);

// Function to initialize the log monitor system
int rdkTestLogMonitorInitialize(RdkTestLogMonitorConfig monitorCfg);

// Function to destroy the log monitor system
void rdkTestLogMonitorDestory(void);

#endif // TESTLOG_MONITOR_H
