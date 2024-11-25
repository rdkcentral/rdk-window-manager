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
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <cstdlib>
#include <cerrno>
#include <chrono>
#include <thread>
#include <mqueue.h>
#include <ctime>
#include <regex>
#include <unordered_map>
#include <fcntl.h>
#include <sys/file.h>
#include <string>
#include <sstream>
#include "testlogmonitor.h"

#define RDK_TESTLOGGING_DEBUG
#define RDK_TESTLOGGING_NAME                      "rdktestlogmonitor"

#ifdef RDK_TESTLOGGING_DEBUG
#define RDK_TESTLOGGING_INFO(aMessage)               do {printf( "[INFO] %s: F:%s <%s @%05d>: ", RDK_TESTLOGGING_NAME, basename(__FILE__), __FUNCTION__, __LINE__); printf aMessage; printf( "\n");}while(0)
#define RDK_TESTLOGGING_WARN(aMessage)               do {printf( "[WARN] %s: F:%s <%s @%05d>: ", RDK_TESTLOGGING_NAME, basename(__FILE__), __FUNCTION__, __LINE__); printf aMessage; printf( "\n");}while(0)
#define RDK_TESTLOGGING_ERROR(aMessage)              do {printf( "[ERROR] %s: F:%s <%s @%05d>: ", RDK_TESTLOGGING_NAME, basename(__FILE__), __FUNCTION__, __LINE__); printf aMessage; printf( "\n");}while(0)
#else
#define RDK_TESTLOGGING_INFO(aMessage)
#define RDK_TESTLOGGING_WARN(aMessage)
#define RDK_TESTLOGGING_ERROR(aMessage)
#endif 

// Enumeration for message types
typedef enum _RdkTestLogMonitorMessageType {
    RDK_TEST_LOG_MONITOR_UNKNOWN = 0,
    RDK_TEST_LOG_MONITOR_SUBSCRIBE,
    RDK_TEST_LOG_MONITOR_UNSUBSCRIBE
}RdkTestLogMonitorMessageType;

// Enumeration for events
typedef enum _RdkTestLogMonitorEventType {
    RDK_TEST_EVENT_SUBSCRIBE,
    RDK_TEST_EVENT_UNSUBSCRIBE,
    RDK_TEST_EVENT_LOG_RECEIVED,
    RDK_TEST_EVENT_LOG_PROCESSED,
    RDK_TEST_EVENT_LOG_WAITTIME_EXPIRED
}RdkTestLogMonitorEventType;

// Enumeration for state machine states
typedef enum _RdkTestLogMonitorState{
    RDK_TEST_LOG_MONITOR_STATE_IDLE,
    RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED,
    RDK_TEST_LOG_MONITOR_STATE_LOGRECEIVED,
    RDK_TEST_LOG_MONITOR_STATE_WAITING_FOR_UNSUBSCRIBED
}RdkTestLogMonitorState;

// Structure for message queue messages
typedef struct _RdkTestQueueMessage {
    RdkTestLogMonitorMessageType msgType;
    void* param;
}RdkTestQueueMessage;

// Structure for event handling
typedef struct _RdkTestLogMonitorEvent {
    RdkTestLogMonitorEventType eventType;
    void* data;
}RdkTestLogMonitorEvent;

static mqd_t mq; // Message queue descriptor
static pthread_t monitorThread; // Thread descriptor
static pthread_cond_t stateCondition; 
static std::string queueName;

static int subscriptionHandle = 0;
static RdkTestLogMonitorSubscribeConfig globalCallbackInfo;

thread_local std::unordered_map<std::string, std::string> resourceToClientMap;
thread_local std::unordered_map<std::string, std::string> compositorToResourceMap;


// Array of error patterns for checking
static const char* errorPatterns[] = {"ERROR", "FAIL", "WARN", "FATAL"};

// State machine variables
static RdkTestLogMonitorState currentState = RDK_TEST_LOG_MONITOR_STATE_IDLE;

static pthread_mutex_t stateMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutexattr_t recursiveMutexAttr;

static mqd_t createMessageQueue();
static void handleEvent(RdkTestLogMonitorEvent* event);
static void transitionState(RdkTestLogMonitorState newState);
static void processLogLine(const std::string& logLine);
static bool readLine(int fd, std::string& line);
static void* monitorLogFile(void* arg);
static int initRecursiveMutex(void);
static void destroyRecursiveMutex(void);

/**
 * @brief Creates and initializes a POSIX message queue for inter-thread communication.
 *
 * This function sets up a message queue with specified attributes, such as maximum
 * messages and message size. It generates a unique queue name using the process ID and
 * thread ID and returns a descriptor to the created queue.
 *
 * @return The descriptor for the created message queue, or -1 if an error occurs.
 */
static mqd_t createMessageQueue() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(RdkTestQueueMessage);
    attr.mq_curmsgs = 0;

    // Get process ID and thread ID to ensure a unique queue name
    pid_t pid = getpid();
    pthread_t tid = pthread_self();

    // Convert PID and TID to a string
    std::ostringstream oss;
    oss << pid << "_" << tid;

    // Create the queue name using PID and TID
    std::string queueName = "/my_queue_" + oss.str();

    // Log the queue name
    RDK_TESTLOGGING_INFO(("queueName:  %s ", queueName.c_str()));

    mqd_t mq = mq_open(queueName.c_str(), O_CREAT | O_RDWR  , 0666, &attr);
    if (mq == -1) {
        RDK_TESTLOGGING_ERROR(("mq_open error:  %s ",strerror(errno)));
        return -1;
    }
    return mq;
}

/**
 * @brief Transitions the state machine to a new state.
 *
 * This function performs the state transition of the log monitor state machine. It updates
 * the current state and signals any threads waiting on the state condition variable.
 *
 * @param newState The new state to transition to.
 */
static void transitionState(RdkTestLogMonitorState newState) {
    pthread_mutex_lock(&stateMutex);
    if (currentState != newState) {
        RDK_TESTLOGGING_INFO(("Transitioning from state %d to state %d", currentState,newState));
        currentState = newState;
        if(newState == RDK_TEST_LOG_MONITOR_STATE_IDLE || newState == RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED)
        {
            pthread_cond_signal(&stateCondition); // Notify the waiting thread
        }
    }
    pthread_mutex_unlock(&stateMutex);
}

/**
 * @brief Handles events related to log monitoring, such as subscribe, unsubscribe, and log processing.
 *
 * This function processes various events, triggering state transitions based on the
 * current state and the event type. Events include subscription, unsubscription, log
 * reception, log processing, and timeout expiry.
 *
 * @param event Pointer to the event structure (RdkTestLogMonitorEvent) containing event type and data.
 */
static void handleEvent(RdkTestLogMonitorEvent* event) {

    switch (event->eventType) {
        case RDK_TEST_EVENT_SUBSCRIBE:
            if (currentState == RDK_TEST_LOG_MONITOR_STATE_IDLE) {
                RDK_TESTLOGGING_INFO(("Handling subscription event in IDLE state."));
                transitionState(RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED);
            }
            break;

        case RDK_TEST_EVENT_UNSUBSCRIBE:
            if (currentState == RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED || currentState == RDK_TEST_LOG_MONITOR_STATE_LOGRECEIVED) {
                RDK_TESTLOGGING_INFO(("Handling unsubscribe event."));
                transitionState(RDK_TEST_LOG_MONITOR_STATE_WAITING_FOR_UNSUBSCRIBED);
            }
            break;

        case RDK_TEST_EVENT_LOG_RECEIVED:
            if (currentState == RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED) {
                RDK_TESTLOGGING_INFO(("Handling log received event."));
                transitionState(RDK_TEST_LOG_MONITOR_STATE_LOGRECEIVED);
            }
            break;

        case RDK_TEST_EVENT_LOG_PROCESSED:
            if ((currentState == RDK_TEST_LOG_MONITOR_STATE_LOGRECEIVED)||(currentState == RDK_TEST_LOG_MONITOR_STATE_WAITING_FOR_UNSUBSCRIBED))  {
                RDK_TESTLOGGING_INFO(("Handling unsubscribe after log processed."));
                transitionState(RDK_TEST_LOG_MONITOR_STATE_IDLE);
            }
            break;

        case RDK_TEST_EVENT_LOG_WAITTIME_EXPIRED:
            RDK_TESTLOGGING_INFO(("Handling log wait time expired event."));
            transitionState(RDK_TEST_LOG_MONITOR_STATE_IDLE);
            break;

        default:
            RDK_TESTLOGGING_ERROR(("Unknown event type!"));
            break;
    }

}

/**
 * @brief Processes a single log line and checks for error patterns.
 *
 * This function processes a log line, extracts relevant information like resource address,
 * compositor address, and application ID, and updates internal mappings. It checks the log
 * for specific error patterns and invokes a callback if an error is detected.
 *
 * @param logLine The log line to be processed.
 */
static void processLogLine(const std::string& logLine) {
    // Regex patterns to match client, resource, mWstCompositor, and rdkwmtestapp directly
    std::regex resourcePattern(R"(resource@(\S+))");
    std::regex compositorPattern(R"(mWstCompositor@(\S+))"); 
    std::regex appPattern(R"(rdkwmtestapp\S*)"); 

    std::smatch clientMatch, resourceMatch, compositorMatch, appMatch;

    // Extract the resource address
    std::string resourceAddress;
    if (std::regex_search(logLine, resourceMatch, resourcePattern)) {
        resourceAddress = resourceMatch[1].str();
    }

    // Extract the compositor address
    std::string compositorAddress;
    if (std::regex_search(logLine, compositorMatch, compositorPattern)) {
        compositorAddress = compositorMatch[1].str();
    }

    // Extract the application ID (rdkwmtestapp)
    std::string appId;
    if (std::regex_search(logLine, appMatch, appPattern)) {
        appId = appMatch[0].str(); // Extract the full match for rdkwmtestapp
    }

    // If both resource and appId are found, update the map
    if (!resourceAddress.empty() && !appId.empty()) {
        resourceToClientMap[resourceAddress] = appId;
    }

    // If compositor is present, find the corresponding resource
    if (!compositorAddress.empty())
    {
        auto it = compositorToResourceMap.find(compositorAddress);
        if (it != compositorToResourceMap.end())
        {
            // Get the resource mapped to this compositor
            resourceAddress = it->second; 
            // Now check if the resource is already in the map with the app ID
            auto resourceIt = resourceToClientMap.find(resourceAddress);
            if (resourceIt != resourceToClientMap.end()) 
            {
                std::string existingAppId = resourceIt->second;
                if (existingAppId.empty() && !appId.empty()) 
                {
                    // If no app ID found for this resource, map it with the current rdkwmtestapp
                    resourceToClientMap[resourceAddress] = appId;
                }
            }
        }
    }

    // Search for error patterns and trigger callbacks
    for (const char* pattern : errorPatterns)
    {
        std::regex regexPattern(pattern, std::regex_constants::icase);
        if (std::regex_search(logLine, regexPattern))
        {
            RDK_TESTLOGGING_INFO(("Error pattern found: %s", logLine.c_str()));
            if (!resourceAddress.empty())
            {
                auto resourceIt = resourceToClientMap.find(resourceAddress);
                if (resourceIt != resourceToClientMap.end())
                {
                    std::string rdkwmtestapp = resourceIt->second;
                    if (!rdkwmtestapp.empty())
                    {
                        RDK_TESTLOGGING_INFO(("Sending callback for app: %s", rdkwmtestapp.c_str()));
                        // Pass the app ID (rdkwmtestapp) along with the log line to the callback
                        globalCallbackInfo.callback(globalCallbackInfo.context, globalCallbackInfo.userParam, (logLine + " App: " + rdkwmtestapp).c_str());
                    }
                    else
                    {
                        RDK_TESTLOGGING_INFO(("No app ID found for resource: %s", resourceAddress.c_str()));
                    }
                }
                else
                {
                    RDK_TESTLOGGING_INFO(("Resource not found in the map for resource: %s", resourceAddress.c_str()));
                }
            }
            break;
        }
    }
}

/**
 * @brief Subscribe to log monitoring for a specified configuration.
 *
 * This function allows a client to subscribe to the log monitoring service
 * by providing a configuration that includes a callback function and related parameters.
 * It also sends a subscription message to the message queue and waits for state transition.
 *
 * @param subscribeCfg Configuration structure containing callback, context, and user parameters.
 * @return A handle (void pointer) for the subscription if successful, or NULL on failure.
 */
void* rdkTestLogMonitorSubscribe(RdkTestLogMonitorSubscribeConfig subscribeCfg) 
{
    if (currentState != RDK_TEST_LOG_MONITOR_STATE_IDLE)
    {
        RDK_TESTLOGGING_ERROR(("Cannot subscribe. Current state:%d ", currentState));
        return NULL;
    }

    // Prepare subscription message
    RdkTestQueueMessage message;
    message.msgType = RDK_TEST_LOG_MONITOR_SUBSCRIBE;
    message.param = malloc(sizeof(RdkTestLogMonitorSubscribeConfig));
    if (message.param) 
    {
        memcpy(message.param, &subscribeCfg, sizeof(RdkTestLogMonitorSubscribeConfig)); 
    } 
    else 
    {
        RDK_TESTLOGGING_ERROR(("Failed to allocate memory for subscription parameters."));
        return NULL;
    }

    // Serialize the RdkTestQueueMessage structure and send
    if (mq_send(mq, reinterpret_cast<const char*>(&message), sizeof(RdkTestQueueMessage), 0) == 0)
    {
        subscriptionHandle++;
        globalCallbackInfo.context = subscribeCfg.context;
        globalCallbackInfo.userParam = subscribeCfg.userParam;
        globalCallbackInfo.callback = subscribeCfg.callback;

        pthread_mutex_lock(&stateMutex);
        // Wait until the currentState becomes RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED
        while (currentState != RDK_TEST_LOG_MONITOR_STATE_SUBSCRIBED) {
            pthread_cond_wait(&stateCondition, &stateMutex);
        }
        pthread_mutex_unlock(&stateMutex);
        free(message.param); 
        return reinterpret_cast<void*>(subscriptionHandle);
    }
    else
    {
        RDK_TESTLOGGING_ERROR(("mq_send error: %s", strerror(errno)));
        free(message.param);
    }

    return NULL;
}

/**
 * @brief Unsubscribe from log monitoring using the provided subscription handle.
 *
 * This function allows a client to unsubscribe from log monitoring. It sends
 * an unsubscribe message to the message queue and waits for the state to transition
 * to idle, indicating the unsubscribe has been processed.
 *
 * @param handle Subscription handle (void pointer) to be used for unsubscribing.
 * @return true if successfully unsubscribed, false otherwise.
 */
bool rdkTestLogMonitorUnsubscribe(void* handle) 
{
    if (reinterpret_cast<void*>(subscriptionHandle) != handle) 
    {
        RDK_TESTLOGGING_ERROR(("Unexpected unsubscribe handle."));
        return false;
    }

    RDK_TESTLOGGING_INFO(("Preparing unsubscribe message."));
    // Prepare unsubscribe message
    RdkTestQueueMessage message;
    message.msgType = RDK_TEST_LOG_MONITOR_UNSUBSCRIBE;
    message.param = handle; 

    if (mq_send(mq, reinterpret_cast<const char*>(&message), sizeof(RdkTestQueueMessage), 0) == 0) 
    {
        RDK_TESTLOGGING_INFO(("State transition to confirm unsubscribe."));
        // Wait for state transition to confirm unsubscribe
        pthread_mutex_lock(&stateMutex);
        // Wait until the currentState becomes RDK_TEST_LOG_MONITOR_STATE_IDLE
        while (currentState != RDK_TEST_LOG_MONITOR_STATE_IDLE) {
            pthread_cond_wait(&stateCondition, &stateMutex);
        }
        pthread_mutex_unlock(&stateMutex);
        globalCallbackInfo = {nullptr, nullptr, nullptr};

        return true;
    }
    else
    {
        RDK_TESTLOGGING_ERROR(("mq_send error: %s", strerror(errno)));
    }
    RDK_TESTLOGGING_INFO(("Unsubscribed."));
    return false;
}

/**
 * @brief Reads a line from a file descriptor.
 *
 * This function reads a line of text from a file descriptor, handling partial reads
 * and buffering to ensure a complete line is returned. It reads until a newline
 * character is found or the end of the file is reached.
 *
 * @param fd The file descriptor to read from.
 * @param line Reference to a string where the read line will be stored.
 * @return true if a line was successfully read, false otherwise.
 */
static bool readLine(int fd, std::string& line) 
{
    static std::string buffer; // Buffer to hold incomplete lines
    line.clear();
    char chunk[256];
    ssize_t bytesRead;

    // Continue to read chunks from the file descriptor until a complete line is found or EOF
    while (true) 
    {
        // Process any complete lines already in the buffer
        size_t pos = buffer.find('\n');
        if (pos != std::string::npos) 
        {
            // If a newline is found, extract the line and return it
            line = buffer.substr(0, pos + 1);
            buffer.erase(0, pos + 1); 
            return true;
        }

        // If no newline is found, read more data from the file descriptor
        bytesRead = read(fd, chunk, sizeof(chunk) - 1); 
        if (bytesRead < 0) 
        {
            RDK_TESTLOGGING_ERROR(("read error: %s", strerror(errno)));
            return false; 
        }
        if (bytesRead == 0) 
        {
            if (!buffer.empty())
            {
                 // Return the last incomplete line or remaining data
                line = buffer;
                buffer.clear(); 
                return true; 
            }
            // No more lines to read
            return false; 
        }

        // Null-terminate the chunk and append it to the buffer
        chunk[bytesRead] = '\0';
        // Append the chunk to the buffer
        buffer += chunk;
    }
}


/**
 * @brief Monitors a specified log file and processes log entries.
 *
 * This thread function monitors a log file, using polling to detect new log entries
 * and events from the message queue. Log entries are processed to detect patterns and
 * trigger callbacks. The function also handles state transitions for subscribing and
 * unsubscribing.
 *
 * @param arg Pointer to the log monitor configuration structure (RdkTestLogMonitorConfig).
 * @return NULL when the thread finishes execution.
 */
static void* monitorLogFile(void* arg) {
    auto* args = static_cast<RdkTestLogMonitorConfig*>(arg);
    struct pollfd fds[2];
    std::string logLine;
    struct timespec unsubscribeTime, currentTime;
    off_t currentOffset = 0;

    if (!args) {
        RDK_TESTLOGGING_ERROR(("RdkTestLogMonitorConfig pointer is null!"));
        return nullptr;
    }

    RDK_TESTLOGGING_ERROR(("logFilePath:%s", args->logFilePath.c_str()));
    int fd = open(args->logFilePath.c_str(), O_RDONLY);
    if (fd == -1) {
        RDK_TESTLOGGING_ERROR(("Failed to open log file:%s", strerror(errno)));
        return nullptr;
    }

    // Move to the end of the file for tailing
    currentOffset = lseek(fd, 0, SEEK_END);

    fds[0].fd = fd; 
    fds[0].events = POLLIN;
    fds[1].fd = mq;
    fds[1].events = POLLIN;

    while (true) {
        // Poll both the log file and message queue
        int pollResult = poll(fds, 2, -1);

        if (pollResult == -1) {
            RDK_TESTLOGGING_ERROR(("poll error:%s", strerror(errno)));
            break;
        }

        if ((fds[0].revents & POLLIN) && (currentState != RDK_TEST_LOG_MONITOR_STATE_IDLE)) 
        {
            if (flock(fd, LOCK_EX) == -1) 
            {
                RDK_TESTLOGGING_ERROR(("Error locking file:%s", strerror(errno)));
                break;
            }

            lseek(fd, currentOffset, SEEK_SET);

            while (readLine(fd, logLine)) 
            {
                RdkTestLogMonitorEvent logEvent = {RDK_TEST_EVENT_LOG_RECEIVED, nullptr};
                handleEvent(&logEvent); 
                processLogLine(logLine);

                // Update the offset after each line is processed
                currentOffset = lseek(fd, 0, SEEK_CUR);
            }
            if (flock(fd, LOCK_UN) == -1) 
            {
                RDK_TESTLOGGING_ERROR(("Error unlocking file:%s", strerror(errno)));
                break;
            }

            //wait for completing the processlogline until unsubscricbe received
            if(currentState == RDK_TEST_LOG_MONITOR_STATE_WAITING_FOR_UNSUBSCRIBED)
            {
                RDK_TESTLOGGING_INFO(("Posting the  RDK_TEST_EVENT_LOG_PROCESSED"));
                RdkTestLogMonitorEvent logProcessedEvent = {RDK_TEST_EVENT_LOG_PROCESSED, nullptr};
                handleEvent(&logProcessedEvent);
            }
        }

        if (fds[1].revents & POLLIN)
        {
            RdkTestQueueMessage msg_buffer;
            ssize_t bytes_received = mq_receive(mq, reinterpret_cast<char*>(&msg_buffer), sizeof(RdkTestQueueMessage), nullptr);
            if (bytes_received >= 0) 
            {
                if (msg_buffer.msgType == RDK_TEST_LOG_MONITOR_SUBSCRIBE) 
                {
                    currentOffset = lseek(fd, 0, SEEK_END);
                    RdkTestLogMonitorEvent subscribeEvent = {RDK_TEST_EVENT_SUBSCRIBE, nullptr};
                    handleEvent(&subscribeEvent);
                }
                else if (msg_buffer.msgType == RDK_TEST_LOG_MONITOR_UNSUBSCRIBE)
                {
                    clock_gettime(CLOCK_MONOTONIC, &unsubscribeTime);
                    RdkTestLogMonitorEvent unsubscribeEvent = {RDK_TEST_EVENT_UNSUBSCRIBE, nullptr};
                    handleEvent(&unsubscribeEvent);
                }
            }
            else
            {
                RDK_TESTLOGGING_ERROR(("mq_receive error:%s", strerror(errno)));
            }
        }

        if (currentState == RDK_TEST_LOG_MONITOR_STATE_WAITING_FOR_UNSUBSCRIBED)
        {
            clock_gettime(CLOCK_MONOTONIC, &currentTime);
            long elapsedTime = currentTime.tv_sec - unsubscribeTime.tv_sec;
            if (elapsedTime > 30)
            {
                RdkTestLogMonitorEvent logEvent = {RDK_TEST_EVENT_LOG_WAITTIME_EXPIRED, nullptr};
                RDK_TESTLOGGING_ERROR(("No new log data."));
                handleEvent(&logEvent);
            }
        }
    }

    close(fd);
    return nullptr;
}


/**
 * @brief Initializes a recursive mutex.
 *
 * This function initializes a recursive mutex that allows the same thread to lock
 * the mutex multiple times without causing a deadlock. It also sets up mutex attributes
 * and condition variables required for thread synchronization.
 *
 * @return 0 on success, or an error code on failure.
 */
static int initRecursiveMutex(void) {
    int status;

    // Initialize the mutex attributes
    status = pthread_mutexattr_init(&recursiveMutexAttr);
    if (status != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_mutexattr_init error:%s", strerror(errno)));
        return status; // Return the error code
    }

    // Set the mutex type to recursive
    status = pthread_mutexattr_settype(&recursiveMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    if (status != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_mutexattr_settype error:%s", strerror(errno)));
        pthread_mutexattr_destroy(&recursiveMutexAttr); 
        return status;
    }

    // Initialize the state mutex with the attributes
    status = pthread_mutex_init(&stateMutex, &recursiveMutexAttr);
    if (status != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_mutex_init error:%s", strerror(errno)));
        pthread_mutexattr_destroy(&recursiveMutexAttr); 
        return status;
    }

    // Initialize the condition variable
    status = pthread_cond_init(&stateCondition, nullptr);
    if (status != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_cond_init error:%s", strerror(errno)));
        pthread_mutex_destroy(&stateMutex); 
        pthread_mutexattr_destroy(&recursiveMutexAttr); 
        return status;
    }

    return 0;
}

/**
 * @brief Destroys a recursive mutex.
 *
 * This function destroys a previously initialized recursive mutex, releasing any system resources
 * associated with it. It ensures proper cleanup of the mutex and associated condition variables
 * to avoid memory leaks or undefined behavior.
 *
 * @return 0 on success, or an error code on failure.
 */
static void destroyRecursiveMutex(void) {
    // Destroy the condition variable
    if (pthread_cond_destroy(&stateCondition) != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_cond_destroy error:%s", strerror(errno)));
    }

    // Destroy the mutex
    if (pthread_mutex_destroy(&stateMutex) != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_mutex_destroy error:%s", strerror(errno)));
    }

    // Destroy the mutex attributes
    if (pthread_mutexattr_destroy(&recursiveMutexAttr) != 0) {
        RDK_TESTLOGGING_ERROR(("pthread_mutexattr_destroy error:%s", strerror(errno)));
    }
}

/**
 * @brief Initializes a recursive mutex.
 *
 * This function initializes a recursive mutex that allows the same thread to lock
 * the mutex multiple times without causing a deadlock. It also sets up mutex attributes
 * and condition variables required for thread synchronization.
 *
 * @return 0 on success, or an error code on failure.
 */
int rdkTestLogMonitorInitialize(RdkTestLogMonitorConfig monitorCfg) {
    RDK_TESTLOGGING_INFO(("rdkTestLogMonitorInitialize start"));
    if (initRecursiveMutex() != 0) {
        RDK_TESTLOGGING_ERROR(("Failed to create mutexes."));
        destroyRecursiveMutex();
        return -1;
    }

    mq = createMessageQueue();
    if (mq == -1) {
        RDK_TESTLOGGING_ERROR(("Failed to create mutexes."));
        destroyRecursiveMutex();
        return -1;
    }

    RdkTestLogMonitorConfig* monitorCfgCopy = new RdkTestLogMonitorConfig(monitorCfg);
    if (pthread_create(&monitorThread, nullptr, monitorLogFile, monitorCfgCopy) != 0) {
        RDK_TESTLOGGING_ERROR(("Failed to create monitor thread."));
        rdkTestLogMonitorDestory();
         delete monitorCfgCopy;
        return -1;
    }
    RDK_TESTLOGGING_INFO(("rdkTestLogMonitorInitialize end"));
    return 0;
}



/**
 * @brief Destroys and cleans up the log monitor system.
 *
 * This function is responsible for safely terminating the log monitoring system.
 * It ensures that the log monitor thread is stopped, deallocates any resources
 * (such as the message queue and recursive mutex), and performs any necessary
 * cleanup to ensure a graceful shutdown of the monitoring functionality.
 * After this function is called, no further log monitoring will take place.
 *
 * @return void
 */
void rdkTestLogMonitorDestory(void) {

    if (monitorThread != 0) {
        if (pthread_cancel(monitorThread) == 0) {
            pthread_join(monitorThread, nullptr);
        }
    }

    if (mq != -1) {
        mq_close(mq);
        if (!queueName.empty()) {
            mq_unlink(queueName.c_str());  // Use the stored queue name
        }
        mq = -1;
    }
    destroyRecursiveMutex();

}

