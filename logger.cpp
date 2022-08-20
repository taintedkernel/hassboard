#include "logger.h"

#include <stdio.h>

void _error(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  printf(TERM_RED "[err]  " TERM_DEFAULT);
  vprintf(fmt, argptr);
  printf("\n");
  va_end(argptr);
}

void _warn(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  printf(TERM_YELLOW "[warn]  " TERM_DEFAULT);
  vprintf(fmt, argptr);
  printf("\n");
  va_end(argptr);
}

void _debug(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  printf("[dbg]  ");
  vprintf(fmt, argptr);
  printf("\n");
  va_end(argptr);
}

void _log(const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  printf(TERM_BLUE "[info] " TERM_DEFAULT);
  vprintf(fmt, argptr);
  printf("\n");
  va_end(argptr);
}