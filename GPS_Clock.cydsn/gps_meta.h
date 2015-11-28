#ifndef __gps_meta_h__
#define __gps_meta_h__

#include <stdint.h>
#include "date_time.h"

void parseNMEAData(char* dataBuffer, date_time* utcDateTime);
  
// Everything below this point relates to creating a message to send back to
//  the GPS, to reconfigure it. I'm leaving it in, just in case it proves to
//  be of some use in the future.
typedef struct gpsMessage
{
  uint8_t start;
  uint16_t length;
  uint8_t messageID;
  char *payload;
  uint8_t checksum;
  uint8_t end[2];
} gpsMessage;

// Pseudo-constructor for a new gpsMessage struct, which auto-populates the
//  start and end members.
#define newGPSMessage(X) gpsMessage X = { \
  .start[0] = '$', \
  .end[0] = '\r', \
  .end[1] = '\n' \
}

void GPSChecksum(gpsMessage* message);

#endif
  
