#ifndef DATETIME_H
#define DATETIME_H

#include <ctime>

unsigned int year(tm *ts);
unsigned int month(tm *ts);
unsigned int day(tm *ts);
unsigned int hour(tm *ts);
unsigned int minute(tm *ts);
unsigned int second(tm *ts);
char* s_weekday(tm *ts);
char* timestamp();
time_t clock_ts(void);

#endif