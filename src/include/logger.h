#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <string>

#define TERM_RED      "\033[1;31m"
#define TERM_YELLOW   "\033[1;33m"
#define TERM_BLUE     "\033[1;34m"
#define TERM_DEFAULT  "\033[0m"

#define LOG_TO_FILE   true
#define LOG_PATH      "/dev/shm/smartgirder.log"

#define __METHOD__          methodName(__PRETTY_FUNCTION__, "")
#define __METHOD_ARG__(arg) methodName(__PRETTY_FUNCTION__, arg)

void initLogger(void);
void shutdownLogger(void);
void _error(const char *fmt, ...);
void _error(std::string fmt, ...);
void _warn(const char *fmt, ...);
void _warn(std::string fmt, ...);
void _debug(const char *fmt, ...);
void _debug(std::string fmt, ...);
void _log(const char *fmt, ...);
void _log(std::string fmt, ...);
void __logHelper(const char *header, const char *fmt, va_list argptr);
std::string methodName(const std::string& prettyFunction, std::string arg);

#endif