#include "gps_meta.h"
#include "project.h"
#include "date_time.h"
#include <stdint.h>

void parseNMEAData(char* dataBuffer, date_time* utcDateTime)
{
  uint8_t i = 0;
  const char RMCKey[7] = "$GPRMC";
  const char NMEAToken[2] = ",";
  char* currData;
  currData =strtok(dataBuffer, NMEAToken);
  if (strcmp(currData, RMCKey) != 0)
  {
    return;
  }
  currData = strtok(NULL, NMEAToken);
  // currData now contains the UTC time string. hhmmss.xxx if you really want
  // to go out to millisecond precision. You'll see the char - '0' thing here
  // a lot; that's a quick, cheesy way to convert an ASCII digit to its
  // equivalent 8-bit unsigned integer value.
  utcDateTime->hrs = ((currData[0] - '0') * 10) + (currData[1] - '0');
  utcDateTime->tmin = currData[2] - '0';
  utcDateTime->min = currData[3] - '0';
  utcDateTime->tsecs = currData[4] - '0';
  utcDateTime->secs = currData[5] - '0';
  
  for (i = 0; i <8; i++)
  {
    currData = strtok(NULL, NMEAToken);
  }
  // Now, currData contains the UTC date string. ddmmyy - guess *somebody*
  // didn't learn from the whole "Y2K" thing. We don't need to do the tens
  // separation here because we don't care about retaining the characters.
  // We *want* the numerical version of the value to calculate DST changes.
  utcDateTime->day = ((currData[0]-'0') * 10) + (currData[1]-'0');
  utcDateTime->month = ((currData[2]-'0') * 10) + (currData[3]-'0'); 
  utcDateTime->year = ((currData[4]-'0') * 10) + (currData[5]-'0'); 
  // Adjust the time for UTC offset.
  utcOffsetDateTime(utcDateTime);

  /* // This section can be used to check the time adjusting code. By placing
     //  it here, and changing the UART pin from P2.2 to P2.0, you can issue
     //  strings from a terminal to test random times for compliance, as the
     //  output will only occur after a string has been received and parsed.
  if (USBUART_CDCIsReady())
  {
  int8_t displayHours = utcDateTime->hrs;
  char strtemp[64];
  if (dstCheck(utcDateTime))
  {
    if(++displayHours > 23)
    {
      displayHours = 0;
    }
  }
  sprintf(strtemp, "Time: %u:%u%u:%u%u\nDate: %u-%u-%u\n", \
      displayHours,  \
      utcDateTime->tmin, \
      utcDateTime->min, \
      utcDateTime->tsecs, \
      utcDateTime->secs, \
      utcDateTime->day, \
      utcDateTime->month, \
      utcDateTime->year );
  USBUART_PutString(strtemp);
  }
  */
}

// This stuff is for calculating a checksum if you want to send a configuration
//  string back to the GPS device. I'm leaving it here because it was painful
//  to figure out. It's currently unused.
void GPSChecksum(gpsMessage* message)
{
  uint16_t i = 0;
  uint8_t checksum = message->messageID;
  for (i = 0; i < message->length; ++i)
  {
    checksum ^= *((message->payload) + i);
  }
  message->checksum = checksum;
}

