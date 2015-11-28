#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "date_time.h"

bool dstCheck(const date_time* localDateTime)
{
  // As of 2007, most of the US observes DST between the 2nd Sunday morning in
  //  March and the 1st Sunday morning in November. DST in the US changes by
  //  time zone, so at 2am local time on the 2nd Sunday in March, we increase
  //  the offset from UTC by one hour.
  
  // We can bail out if month is < 3 or > 11.
  if ( (localDateTime->month < 3) || (localDateTime->month > 11) )
  {
    return false;
  }

  // We can also bail out if month is > 3 and < 11.
  if ( (localDateTime->month > 3) && (localDateTime->month < 11) )
  {
    return true;
  }
  
  // Okay, edge cases, March and November. We can do a couple more low-math
  //  checks to quickly decide whether the date is a possible one.
  // For November, the date of the first Sunday *must* be less than 8, so if
  //  the date is greater than 8, we can return false.
  if (localDateTime->month == 11)
  {
    if (localDateTime->day > 7)
    {
      return false;
    }
    // Otherwise, we need to figure out whether we've seen the first Sunday
    //  yet.
    date_time firstOfNov = 
                      {.day = 1, .month = 11, .year = localDateTime->year};
    int8_t firstDayOfNov = calculateDayOfWeek(&firstOfNov);
    int8_t firstSundayOfNov = (9 - firstDayOfNov) % 7;
    // If we *haven't* seen the first Sunday yet, we are in DST still.
    if (localDateTime->day < firstSundayOfNov)
    {
      return true;
    }

    // If it's *after* the first Sunday, we aren't in DST anymore.
    if (localDateTime->day > firstSundayOfNov)
    {
      return false;
    }
    
    // If we're here, it's the first Sunday of November right now, and we only
    //  need to know if the current hour is < 2; at 0200 MST, DST ends.
    if (localDateTime->hrs < 2)
    {
      return true;
    }
    return false;
  }

  // For March, dates less than 8, or greater than 13 are automatically out.
  if (localDateTime->month == 3)
  {
    if (localDateTime->day < 8)
    {
      return false;
    }
    if (localDateTime->day > 13)
    {
      return true;
    }

    // Otherwise, we need to figure out whether we've see the second Sunday
    //  yet. 
    date_time firstOfMar = 
                      {.day = 1, .month = 3, .year = localDateTime->year};
    int8_t firstDayOfMar = calculateDayOfWeek(&firstOfMar);
    int8_t secondSundayOfMar = ((9 - firstDayOfMar) % 7) + 7;

    // If we *haven't* seen the second Sunday yet, we aren't in DST yet.
    if (localDateTime->day < secondSundayOfMar)
    {
      return false;
    }

    // If it's *after* the second Sunday, we are in DST now.
    if (localDateTime->day > secondSundayOfMar)
    {
      return true;
    }
    
    // If we're here, it's the second Sunday of November right now, and we only
    //  need to know if the current hour is < 2; at 0200 MST, DST starts.
    if (localDateTime->hrs < 2)
    {
      return false;
    }
    return true;
  }
  return false; // We *should* never get here, but we need to return something
                //  or chaos ensues.
}

int8_t calculateDayOfWeek(const date_time* localDateTime)
{
  // Uses the "Key Value" method of calculating the day of the week, as here:
  //  1. Take last two digits of the year and divide by 4, discarding any 
  //     fractional portions left over.
  //  2. Add day of month.
  //  3. Add monthly "key value", as defined below.
  //  4. Iff month == 1 or month == 2, AND it's a leap year, subtract 1.
  //  5. Add century "key value", as defined below. The index of the key value
  //        i = (C % 400)/100 where C is e.g., 2000, 1900, 1800, etc.
  //  6. Add the last two digits of the year.
  //  7. The day of the week is the result of the above, modulo 7, where
  //     Saturday is 0, Sunday is 1, etc etc.
  //
  //  This method has the advantage of not requiring floating point maths, so
  //  it is likely to be faster on an embedded device.
  
  // Magic key values. These can be found online. Somebody smarter than me
  //  figured them out.
  const int16_t monthKeyValue[] = {1, 4, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6};
  const int16_t centuryKeyValue[] = {6, 0, 2, 4};

  // The GPS I'm using returns a two-digit year; you may need to separate the
  //  year from the century manually. That's easy; just modulo by 100.
  int16_t dayOfWeek = localDateTime->year;

  // I'm hard-coding this. It's 2015 now; if we make it to the year 2100 and
  //  somebody is reading this, sorry about that. This GPS doesn't return the
  //  full four-digit year.
  int16_t century = 2000;

  // Okay, here we go:
  //  1. Take last two digits of the year and divide by 4, discarding any 
  //     fractional portions left over.
  dayOfWeek = dayOfWeek/4; // We can ignore the leftovers b/c integer math.

  //  2. Add day of month.
  dayOfWeek += localDateTime->day;

  //  3. Add monthly "key value", as defined below.
  dayOfWeek += monthKeyValue[localDateTime->month - 1];

  //  4. Iff month == 1 or month == 2, AND it's a leap year, subtract 1.
  if (localDateTime->month < 3) // Cheap check.
  {
    // isLeapYear is a more expensive check, so wait until we know for sure
    //  that it's January or February before doing the check.
    if (isLeapYear(century + localDateTime->year))
    {
      --dayOfWeek;
    }
  }

  //  5. Add century "key value", as defined below. The index of the key value
  //        i = (C % 400)/100 where C is e.g., 2000, 1900, 1800, etc.
    dayOfWeek += centuryKeyValue[(2000%400)/100];

  //  6. Add the last two digits of the year.
  dayOfWeek += localDateTime->year;

  //  7. The day of the week is the result of the above, modulo 7, where
  //     Saturday is 0, Sunday is 1, etc etc.
  return (int8_t)(dayOfWeek%7);
}

bool isLeapYear(int16_t year)
{
  // Checking for a leap year (at least, in Gregorian) is easy: if the year is
  //  divisible by four, but not by 100, it's a leap year. The exception is
  //  years divisble by 400; they're leap years as well.
  if (year % 400 == 0)
  {
    return true;
  }
  if (year % 100 == 0)
  {
    return false;
  }
  if (year % 4 == 0)
  {
    return true;
  }
  return false;
}

void utcOffsetDateTime(date_time* currDateTime)
{
  date_time adjusted_date;
  memcpy(&adjusted_date, (const void*)currDateTime, sizeof(date_time));

  // We need to add the timezone offset and then, if that pushes us across a
  //  date boundary, decide how to adjust the date accordingly. If the time
  //  offset is positive, we would advance the date by one versus UTC date,
  //  and if the offset is negative, we decrease the date by one from the
  //  UTC date.
  // Of course, sometimes, that's going to push us across a month boundary,
  //  too, so we need to be aware of that.
  // Last note: we *won't* be taking DST into account here; that is the last
  //  thing that happens, and it happens *immediately* before display. We're
  //  cheating a bit; because we only *really* care about time, date only
  //  matters for calculating the DST status, so post-DST change date doesn't
  //  matter.
  adjusted_date.hrs += TIMEZONE;

  // If we just rolled back past midnight, we want to reduce the date by one.
  if (adjusted_date.hrs < 0)
  {
    if (--adjusted_date.day == 0)
    {
      if (--adjusted_date.month == 0)
      {
        adjusted_date.month = 12;
        adjusted_date.day = 31;
        if (--adjusted_date.year == -1)
        {
          adjusted_date.year = 99;
        }
      }
      else
      {
        adjusted_date.day = daysThisMonth(adjusted_date.month);
      }
    }
  }
  // If we just rolled ahead past midnight, we want to increase the date by one.
  else if (adjusted_date.hrs > 23)
  {
    if (++adjusted_date.day > daysThisMonth(adjusted_date.month))
    {
      if (++adjusted_date.month == 13)
      {
        adjusted_date.month = 1;
        adjusted_date.day = 1;
        if (++adjusted_date.year == 100)
        {
          adjusted_date.year = 0;
        }
      }
      else
      {
        adjusted_date.day = 1;
      }
    }
  }  
  memcpy(currDateTime, (const void*)(&adjusted_date), sizeof(date_time));
}

int8_t daysThisMonth(int8_t month)
{
  switch(month)
  {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
      break;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
      break;
    case 2:
      return 28;
      break;
  }
  return 30;
}

