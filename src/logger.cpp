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

void _debug(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  __logHelper("[dbg]  ", fmt, argptr);
  va_end(argptr);
}

void _log(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  __logHelper(TERM_BLUE "[info] ", fmt, argptr);
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