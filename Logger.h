#pragma once

#include <string>
#include "noncopyable.h"

//  LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(LogmsgFormat, ...)                       \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__);                  \
        logger.log(buf);                                  \
    } while (0);
/*
snprintf()函数用于将格式化的数据写入字符串，其原型为：
int snprintf(char *str, int n, char * format ,[ argument, …]);

【参数】str为要写入的字符串；n为要写入的字符的最大数目，超过n会被截断；format为格式化字符串，与printf()函数相同；argument为变量。

【返回值】成功则返回参数str 字符串长度，失败则返回-1，错误原因存于errno 中。
*/

#define LOG_ERROR(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0);

#define LOG_FATAL(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);    \
    } while (0);

#ifdef MUDEBUG
#define LOG_DEBUG(LogmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0);
#else
#define LOG_DEBUG(LogmsgFormat, ...)
#endif

// 定义日志的级别    INFO    ERROR   FATAL   DEBUG
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日日志类
class Logger
{
public:
    // 获取日志唯一的实例对象
    static Logger &instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    int logLevel_;
    Logger() {}
};