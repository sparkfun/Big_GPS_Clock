#include "ws281x_7seg.h"
#include "project.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
*  Each character is composed of 84 LEDs in 7 groups of 12. Typically, we give
*   the segments these names:
*      A
*   F     B
*      G
*   E     C
*      D
* 
*  In this system, each character has 12 LEDs per segment. The segments are
*   mapped to the segments by range, thus:
*  A - 36-47
*  B - 24-35
*  C - 12-23
*  D - 0-11
*  E - 60-71
*  F - 48-59
*  G - 72-83
*
*  We create 7 functions to write arbitrary values to the LEDs of each segment,
*   a function which turns those on or off as needed to describe the current
*   values, and then we scan through as appropriate in main.
******************************************************************************/

void writeDigit(bool digitSegs[7], uint8_t digit)
{
  switch(digit)
  {
    case 0:
      digitSegs[A] = true;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = true;
      digitSegs[E] = true;
      digitSegs[F] = true;
      digitSegs[G] = false;
      break;
    case 1:
      digitSegs[A] = false;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = false;
      digitSegs[E] = false;
      digitSegs[F] = false;
      digitSegs[G] = false;
      break;
    case 2:
      digitSegs[A] = true;
      digitSegs[B] = true;
      digitSegs[C] = false;
      digitSegs[D] = true;
      digitSegs[E] = true;
      digitSegs[F] = false;
      digitSegs[G] = true;
      break;
    case 3:
      digitSegs[A] = true;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = true;
      digitSegs[E] = false;
      digitSegs[F] = false;
      digitSegs[G] = true;
      break;
    case 4:
      digitSegs[A] = false;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = false;
      digitSegs[E] = false;
      digitSegs[F] = true;
      digitSegs[G] = true;
      break;
    case 5:
      digitSegs[A] = true;
      digitSegs[B] = false;
      digitSegs[C] = true;
      digitSegs[D] = true;
      digitSegs[E] = false;
      digitSegs[F] = true;
      digitSegs[G] = true;
      break;
    case 6:
      digitSegs[A] = true;
      digitSegs[B] = false;
      digitSegs[C] = true;
      digitSegs[D] = true;
      digitSegs[E] = true;
      digitSegs[F] = true;
      digitSegs[G] = true;
      break;
    case 7:
      digitSegs[A] = true;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = false;
      digitSegs[E] = false;
      digitSegs[F] = false;
      digitSegs[G] = false;
      break;
    case 8:
      digitSegs[A] = true;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = true;
      digitSegs[E] = true;
      digitSegs[F] = true;
      digitSegs[G] = true;
      break;
    case 9:
      digitSegs[A] = true;
      digitSegs[B] = true;
      digitSegs[C] = true;
      digitSegs[D] = true;
      digitSegs[E] = false;
      digitSegs[F] = true;
      digitSegs[G] = true;
      break;
  }
}

// NB: This function assumes a 6-digit display, with 12 lights per segment
//  for a total of 84 lights per digit. If you try to use some other dimension
//  you'll bork the code. Sorry.
void updateDisplay(bool** LEDValues, uint32_t** segmentValues)
{
  uint8_t digitIndex;
  // For each digit...
  for (digitIndex = 0; digitIndex < 6; digitIndex++)
  {
    // ...we must index across the segments...
    uint8_t segmentIndex;
    for (segmentIndex = 0; segmentIndex < 8; segmentIndex++)
    {
      //...and, if that segment is to be enabled, send the value to it.
      if (segmentValues[digitIndex][segmentIndex])
      {
        uint8_t pixelIndex;
        for (pixelIndex = 0; pixelIndex < 12; pixelIndex++)
        {
          StripLights_Pixel((segmentIndex * 12)+pixelIndex, 0, 
            LEDValues[digitIndex][(segmentIndex*12 + pixelIndex)]);
        }
      }
      else
      {
        uint8_t pixelIndex;
        for (pixelIndex = 0; pixelIndex < 12; pixelIndex++)
        {
          StripLights_Pixel((segmentIndex * 12)+pixelIndex, 0, 
            StripLights_BLACK);
        }
      }
    }
    StripChannelSelect_Write(digitIndex);
    StripLights_Trigger(1); 
    CyDelay(10);
  }
}
