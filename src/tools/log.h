#ifndef LOG_H
#define LOG_H
#include <cstdio>
#include <cstdarg>
#include <chrono>


#define LOGD(module, ...) Log::log(LOG_DEBUG, module, __VA_ARGS__)
#define LOGI(module, ...) Log::log(LOG_INFO,  module, __VA_ARGS__)
#define LOGW(module, ...) Log::log(LOG_WARN,  module, __VA_ARGS__)
#define LOGE(module, ...) Log::log(LOG_ERROR, module, __VA_ARGS__)
#define LOGF(module, ...) Log::log(LOG_FATAL, module, __VA_ARGS__)

enum LogLevel
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

class Log
{
public:
    static void init();

    static void log(LogLevel level,
                    const char* module,
                    const char* fmt,
                    ...);

private:
    static const char* levelString(LogLevel level);

    static std::chrono::steady_clock::time_point startTime;
};

#endif // LOG_H