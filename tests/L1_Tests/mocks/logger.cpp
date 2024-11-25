#include "logger.h"

namespace RdkWindowManager
{
LogLevel Logger::sLogLevel = LogLevel::Debug;
void Logger::log(LogLevel level, const char* format, ...) 
{
    // Empty function body to remove undefined reference errors
}
}