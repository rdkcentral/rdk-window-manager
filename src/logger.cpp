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

#include "logger.h"
#include "rdkwindowmanager.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h> 

namespace RdkWindowManager
{
    static const char* logLevelStrings[] =
    {
      "DEBUG",
      "INFO",
      "WARN",
      "ERROR",
      "FATAL"
    };

    static const int numLogLevels = sizeof(logLevelStrings)/sizeof(logLevelStrings[0]);
    
    const char* logLevelToString(LogLevel l)
    {
      const char* s = "LOG";
      int level = (int)l;
      if (level < numLogLevels)
        s = logLevelStrings[level];
      return s;
    }

    LogLevel Logger::sLogLevel = Information;
    bool Logger::sFlushingEnabled = false;
    
#ifdef RDK_WINDOW_MANAGER_LOGGER
    FILE* Logger::logFile = NULL;
#endif

    void Logger::setLogLevel(const char* loglevel)
    {
      LogLevel level = Information;
      if (loglevel)
      {
        if (strcasecmp(loglevel, "debug") == 0)
          level = Debug;
        else if (strcasecmp(loglevel, "info") == 0)
          level = Information;
        else if (strcasecmp(loglevel, "warn") == 0)
          level = Warn;
        else if (strcasecmp(loglevel, "error") == 0)
          level = Error;
        else if (strcasecmp(loglevel, "fatal") == 0)
          level = Fatal;
      }
      sLogLevel = level;
    }

    void Logger::logLevel(std::string& level)
    {
      level = logLevelToString(sLogLevel);
    }

    void Logger::enableFlushing(bool enable)
    {
      sFlushingEnabled = enable;
    }

    bool Logger::isFlushingEnabled()
    {
      return sFlushingEnabled;
    }

#ifdef RDK_WINDOW_MANAGER_LOGGER
    void Logger::setLogFile(const std::string& filename)
    {
        if (logFile == NULL) 
        {
            /* logfile open in write mode */
            logFile = fopen(filename.c_str(), "w");
            if (logFile == NULL)
            {
                printf("Error: Failed to open log file: %s\n", filename.c_str());
            }
            else
            {
                printf("Info: Log file opened: %s\n", filename.c_str());
            }
        }
        else
        {
            printf("Warning: Log file already opened: %p\n", logFile);
        }
        return;
    }
#endif

    void Logger::log(LogLevel level, const char* format, ...)
    {
      if (level < sLogLevel)
      {
        return;
      }

      int threadId = syscall(__NR_gettid);
      const char* logLevel = logLevelToString(level);
#ifndef RDK_WINDOW_MANAGER_LOGGER_DISABLE_TIMESTAMP
        double secs = seconds();
#endif
      char buffer[256];
      va_list ptr;

      va_start(ptr, format);
#ifdef RDK_WINDOW_MANAGER_LOGGER
      if (NULL != logFile)
      {
#ifdef RDK_WINDOW_MANAGER_LOGGER_DISABLE_TIMESTAMP
        fprintf(logFile, "[%s] RDKWindowManager Thread-%d", logLevel, threadId);
#else
        fprintf(logFile, "[%s] RDKWindowManager Thread-%d Time-%lf: ", logLevel, threadId, secs);
#endif /* RDK_WINDOW_MANAGER_LOGGER_DISABLE_TIMESTAMP */
        vfprintf(logFile, format, ptr);
        fprintf(logFile, "\r\n");
        fflush(logFile);
      }
#endif /* RDK_WINDOW_MANAGER_LOGGER */
      vsnprintf(buffer, sizeof(buffer), format, ptr);
      va_end(ptr);

      #ifdef RDK_WINDOW_MANAGER_LOGGER_DISABLE_TIMESTAMP
        printf("[%s] RDKWindowManager Thread-%d: %s\n", logLevel, threadId, buffer);
      #else
        printf("[%s] RDKWindowManager Thread-%d Time-%lf: %s\n", logLevel, threadId, secs, buffer);
      #endif

      if (sFlushingEnabled)
      {
        fflush(stdout);
      }
    }
}
