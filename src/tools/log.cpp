#include "log.h"

std::chrono::steady_clock::time_point Log::startTime;

void Log::init() {
    startTime = std::chrono::steady_clock::now();
}

const char* Log::levelString(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "\033[36mDEBUG\033[0m";
        case LOG_INFO:  return "\033[32mINFO\033[0m";
        case LOG_WARN:  return "\033[33mWARN\033[0m";
        case LOG_ERROR: return "\033[31mERROR\033[0m";
        case LOG_FATAL: return "\033[31mFATAL\033[0m";
        default:        return "\033[31mUNKNOWN\033[0m";
    }
}


void Log::log(LogLevel level,const char* module,const char* fmt,...)
{
    using namespace std::chrono;

    auto now = steady_clock::now();
    auto ms = duration_cast<milliseconds>(now - startTime).count();

    printf("[%06lld.%03lld] [%s] %s: ",
        ms / 1000,
        ms % 1000,
        levelString(level),
        module);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}