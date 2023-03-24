#include "logger.h"
#include "datetime.h"

#include <stdio.h>
#include <stdlib.h>

FILE *logger;

void initLogger()
{
  if (LOG_TO_FILE) {
    logger = fopen(LOG_PATH, "w+");
  }
}

void shutdownLogger()
{
  if (LOG_TO_FILE) {
    fclose(logger);
  }
}

void _error(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  __logHelper(TERM_RED "[err]  ", fmt, argptr);
  va_end(argptr);
}

void _warn(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  __logHelper(TERM_YELLOW "[warn] ", fmt, argptr);
  va_end(argptr);
}

void _log(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  __logHelper(TERM_BLUE "[info] ", fmt, argptr);
  va_end(argptr);
}

void _debug(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  __logHelper("[dbg]  ", fmt, argptr);
  va_end(argptr);
}


void _error(std::string fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  _error(fmt.c_str(), argptr);
  va_end(argptr);
}

void _warn(std::string fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  _warn(fmt.c_str(), argptr);
  va_end(argptr);
}

void _log(std::string fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  _log(fmt.c_str(), argptr);
  va_end(argptr);
}

void _debug(std::string fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  _debug(fmt.c_str(), argptr);
  va_end(argptr);
}

void __logHelper(const char *header, const char *fmt, va_list argptr)
{
  // TODO: This can be cleaned up and modified to a single stream
  printf("%s %s%s", timestamp(), header, TERM_DEFAULT);
  vprintf(fmt, argptr);
  printf("\n");

  if (LOG_TO_FILE) {
    fprintf(logger, "%s %s%s", timestamp(), header, TERM_DEFAULT);
    vfprintf(logger, fmt, argptr);
    fprintf(logger, "\n");
    fflush(logger);
  }
}

// https://stackoverflow.com/questions/1666802/is-there-a-class-macro-in-c
std::string methodName(const std::string& prettyFunction, std::string arg)
{
    size_t colons = prettyFunction.find("::");
    size_t begin = prettyFunction.substr(0, colons).rfind(" ") + 1;
    size_t end = prettyFunction.rfind("(") - begin;

    return prettyFunction.substr(begin, end) + "(" + arg + ")";
}
