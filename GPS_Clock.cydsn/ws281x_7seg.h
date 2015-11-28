#ifndef __ws281x_7seg_h__
#define __ws281x_7seg_h__

#include <stdint.h>
#include <stdbool.h>

// Each digit is made up of seven segments, and each segment is a string of 12
//  WS2812 LEDs. This enum gives the order in which those strings appear. The
//  segment names are "standard" across most seven segment LED types.
enum SEGMENT_NAME {D, C, B, A, F, G, E};

void writeDigit(bool digitSegs[7], uint8_t digit);
void updateDisplay(bool** LEDValues, uint32_t** segmentValues);

#endif
