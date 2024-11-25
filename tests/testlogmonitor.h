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
