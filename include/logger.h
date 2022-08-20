#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

#define TERM_RED     "\033[1;31m"
#define TERM_YELLOW  "\033[1;33m"
#define TERM_BLUE    "\033[1;34m"
#define TERM_DEFAULT "\033[0m"

void _error(const char *fmt, ...);
void _warn(const char *fmt, ...);
void _debug(const char *fmt, ...);
void _log(const char *fmt, ...);

#endif