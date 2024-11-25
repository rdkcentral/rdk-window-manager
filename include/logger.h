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

#ifndef RDK_WINDOW_MANAGER_LOGGER_H
#define RDK_WINDOW_MANAGER_LOGGER_H
#include <string>

namespace RdkWindowManager
{
    enum LogLevel { 
        Debug,
        Information,
        Warn,
        Error, 
        Fatal
    };

    class Logger
    {
        public:
            static void log(LogLevel level, const char* format, ...);
            static void setLogLevel(const char* loglevel);
            static void logLevel(std::string& level);
            #ifdef RDK_WINDOW_MANAGER_LOGGER
            static void setLogFile(const std::string& filename);
            #endif
            static void enableFlushing(bool enable);
            static bool isFlushingEnabled();

        private:
            static LogLevel sLogLevel;
            static bool sFlushingEnabled;
            #ifdef RDK_WINDOW_MANAGER_LOGGER
            static FILE* logFile;
            #endif
    };
}

#endif //RDK_WINDOW_MANAGER_LOGGER_H
