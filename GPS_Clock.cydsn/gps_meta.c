include "gps_meta.h"

// For a message to be accepted by the GPS clock, it must contain a properly
//  calculated checksum. Given a pointer to a gpsMessage struct, this function
//  calculates that checksum, which is then stored in the gpsMessage struct.
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
