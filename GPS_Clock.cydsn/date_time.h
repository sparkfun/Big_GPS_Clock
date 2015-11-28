#ifndef __date_time_h__
#define __date_time_h__

#include <stdint.h>
#include <stdbool.h>

typedef struct 
{
  int8_t month;
  int8_t day;
  int8_t year;
  int8_t secs;
  int8_t tsecs;
  int8_t min;
  int8_t tmin;
  int8_t hrs;
} date_time;

// Define new timezones here; this is the standard time offset, NOT the DST
//  offset.
#define MST -7

#define TIMEZONE MST

// Some places don't observe DST; we provide this flag for those localities.
#define OBSERVES_DST 1

// Function definitions.
bool dstCheck(const date_time* currDateTime);
void utcOffsetDateTime(date_time* currDateTime);
bool isLeapYear(int16_t year);
int8_t calculateDayOfWeek(const date_time* localDateTime);
int8_t daysThisMonth(int8_t month);

#endif

