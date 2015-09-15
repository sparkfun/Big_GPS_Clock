#ifndef __gps_meta_h__
#define __gps_meta_h__

  #include <stdint.h>
  
  // This is a standard gpsMessage block. Normally it's just a string, but by
  //  making it a struct instead we can manipulate the subfields a lot easier.
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
  