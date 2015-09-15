#include <project.h>
#include <stdbool.h>
#include <stdio.h>
#include "gps_meta.h"

// Each digit is made up of seven segments, and each segment is a string of 12
//  WS2812 LEDs. This enum gives the order in which those strings appear. The
//  segment names are "standard" across most seven segment LED types.
enum SEGMENT_NAME {D, C, B, A, F, G, E};

// This array store the colors for each pixel of each digit. For now, they all
//  get the same value, but in the future it may be possible to calculate
//  values for effects like "wipe" or "rainbow".
uint32_t LEDValues[6][84];

// Simple: is the segment in the array on or off?
bool segmentValues[6][7];

// Various support functions.
void writeDigit(uint8_t position, uint8_t digit);
void updateSegmentVals(uint8_t digit, enum SEGMENT_NAME segment, uint32_t* newVals);
bool enableUSBCDC();
void parseNMEAData();
CY_ISR_PROTO(ClockTickISR);

// Current value of each digit/colons.
volatile uint8_t secs;
volatile uint8_t tsecs;
volatile uint8_t min;
volatile uint8_t tmin;
volatile uint8_t hrs;
volatile bool colon;
volatile bool colonUpdated;

// Data received via UART from the GPS.
uint8_t inboundData[64];
uint8_t inboundDataIndex;

int main()
{
  // Reset the index of the inbound data from the GPS to zero.
  inboundDataIndex = 0;
  
  // Starting at 88:88:88 gives a quick visual check on whether the code is
  //  running or not.
  secs=8;
  tsecs=8;
  min=8;
  tmin=8;
  hrs=8;
  uint8_t digitIndex;
  
  // I think this bit is unused at the moment; I'm trying to get the date and
  //  auto update at DST change thing working, and this supports that.
  newGPSMessage(enableDateString);
  sprintf(enableDateString.payload, " stuff here");
  enableDateString.length = sizeof(enableDateString.payload) + 1;
  GPSChecksum(&enableDateString);
  
	// Enable global interrupts, required for StripLights
  CyGlobalIntEnable;
  bool USBCDCOkay = enableUSBCDC(); // IFF USB is present, we'll echo some data
                                    //  out to it; otherwise, ignore.
  
  // Turn on the various peripherals.
  UART_Start();
  StripLights_Start(); 
  ClockTick_Start();
  ClockTickInt_StartEx(ClockTickISR);
	
	// Set dim level 0 = full power, 4 = lowest power
  StripLights_Dim(1);     
  
  // Clear the memory in the StripLights "object". This is *not* the same as the
  //  memory we declared above!
	StripLights_MemClear(StripLights_BLACK);
  
  // Write out the values to the lights. This will blank the display.
  for (digitIndex = 0; digitIndex < 6; digitIndex++)
  {
    StripChannelSelect_Write(digitIndex); // Point the mux at the string.
    StripLights_Trigger(1);               // Push the data to the string.
    CyDelay(10);                          // Delay a skoosh.
  }
   
  // This bit actually DOES initialize the memory we created.
  uint16_t initIndex;
  for (initIndex = 0; initIndex<504; initIndex++)
  {
    *(LEDValues[0]+initIndex) = StripLights_WHITE;
  }
  
  UART_ClearRxBuffer();
  
  // Ideally we should be able to enable the "date" string. Don't know why this
  //  doesn't work. Fix it another day.
  //UART_PutStringConst("$PMTK000*32\r\a");
  //UART_PutStringConst("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0*29\r\a");
  
  // Loop forever.
	for(;;)
	{
    // Pulls in the data from the GPS.
    while (UART_GetRxBufferSize() > 0)
    {
      inboundData[inboundDataIndex] = UART_ReadRxData();
      // if we've found the end-of-NMEA-string character, parse the data
      if (inboundData[inboundDataIndex++] == 0x0A)  // NMEA terminator
      {
        parseNMEAData();
      }
    }
   
    // The colonUpdated flag gets cleared in an ISR; if it's clear, we should
    //  flip the state of the colons so they blink.
    if (!colonUpdated)
    {
      StripChannelSelect_Write(6);
      uint8_t i = 0;
      if (colon)
      {
        for (i = 0; i<8; i++)
        {
            StripLights_Pixel(i, 0, StripLights_WHITE);
        }
      }
      else
      {
        for (i = 0; i<8; i++)
        {
            StripLights_Pixel(i, 0, StripLights_BLACK);
        }
      }
      StripLights_Trigger(1); 
      CyDelay(10);
      colonUpdated = true;
    }
      
    // Optional debugging statement...
    if (USBCDCOkay)
    {
      //USBUART_PutString("OKAY");
    }
    
    // Write out the current values to the digits. Adjust for 12-hour time.
    //  Note that we do this every time through the loop; that's easier than
    //  checking to see if the time changed and I'm lazy. All this first part
    //  does is set or clear the booleans in the segmentValues array.
    writeDigit(0, secs);
    writeDigit(1, tsecs);
    writeDigit(2, min);
    writeDigit(3, tmin);
    if (hrs < 10)
    {
      writeDigit(4, hrs);
      writeDigit(5, 0);
    }
    else
    {
      writeDigit(4, hrs - 10);
      writeDigit(5, 1);
    }
    
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
}

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

void writeDigit(uint8_t position, uint8_t digit)
{
  switch(digit)
  {
    case 0:
      segmentValues[position][A] = true;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = true;
      segmentValues[position][E] = true;
      segmentValues[position][F] = true;
      segmentValues[position][G] = false;
      break;
    case 1:
      segmentValues[position][A] = false;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = false;
      segmentValues[position][E] = false;
      segmentValues[position][F] = false;
      segmentValues[position][G] = false;
      break;
    case 2:
      segmentValues[position][A] = true;
      segmentValues[position][B] = true;
      segmentValues[position][C] = false;
      segmentValues[position][D] = true;
      segmentValues[position][E] = true;
      segmentValues[position][F] = false;
      segmentValues[position][G] = true;
      break;
    case 3:
      segmentValues[position][A] = true;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = true;
      segmentValues[position][E] = false;
      segmentValues[position][F] = false;
      segmentValues[position][G] = true;
      break;
    case 4:
      segmentValues[position][A] = false;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = false;
      segmentValues[position][E] = false;
      segmentValues[position][F] = true;
      segmentValues[position][G] = true;
      break;
    case 5:
      segmentValues[position][A] = true;
      segmentValues[position][B] = false;
      segmentValues[position][C] = true;
      segmentValues[position][D] = true;
      segmentValues[position][E] = false;
      segmentValues[position][F] = true;
      segmentValues[position][G] = true;
      break;
    case 6:
      segmentValues[position][A] = true;
      segmentValues[position][B] = false;
      segmentValues[position][C] = true;
      segmentValues[position][D] = true;
      segmentValues[position][E] = true;
      segmentValues[position][F] = true;
      segmentValues[position][G] = true;
      break;
    case 7:
      segmentValues[position][A] = true;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = false;
      segmentValues[position][E] = false;
      segmentValues[position][F] = false;
      segmentValues[position][G] = false;
      break;
    case 8:
      segmentValues[position][A] = true;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = true;
      segmentValues[position][E] = true;
      segmentValues[position][F] = true;
      segmentValues[position][G] = true;
      break;
    case 9:
      segmentValues[position][A] = true;
      segmentValues[position][B] = true;
      segmentValues[position][C] = true;
      segmentValues[position][D] = true;
      segmentValues[position][E] = false;
      segmentValues[position][F] = true;
      segmentValues[position][G] = true;
      break;
  }
}

void updateSegmentVals(uint8_t digit, enum SEGMENT_NAME segment, uint32_t newVals[12])
{
  uint8_t index;
  for (index = 0; index < 12; index++)
  {
    LEDValues[digit][segment + index] = newVals[index];
  }
}

CY_ISR(ClockTickISR)
{
  if (colon)
  {
    colon = false;
  }
  else
  {
    colon = true;
  }
  colonUpdated = false;
/*  if (++secs > 9)
  {
    secs = 0;
    if (++tsecs > 5)
    {
      tsecs = 0;
      if (++min > 9)
      {
        min = 0;
        if (++tmin > 5)
        {
          tmin = 0;
          if (++hrs > 12)
          {
            hrs = 1;
          }
        }
      }
    }
  }*/
}

bool enableUSBCDC()
{
  USBUART_Start(0, USBUART_3V_OPERATION);
  uint8_t delayCounter = 0;
  /* It's important that we not loop forever here, or we'll never leave in
   * cases where the device isn't plugged into a PC. We do, however, want to
   * provide enough time for the PC to do its thing. */  
  while(USBUART_GetConfiguration() == 0)
  {  
    CyDelay(100);
    delayCounter++;
    if (delayCounter > 20)
    {
      return false;
    }
  }
  USBUART_CDC_Init();
  return true;
}

void parseNMEAData()
{
  if (inboundData[5] != 'C')
  {
    inboundDataIndex = 0;
    return;
  }
  hrs = ((inboundData[7] - '0') * 10) + (inboundData[8] - '0');
  if (hrs < 7)
  {
    hrs += 24;
  }
  hrs -= 6;
  tmin = inboundData[9] - '0';
  min = inboundData[10] - '0';
  tsecs = inboundData[11] - '0';
  secs = inboundData[12] - '0';
  inboundDataIndex = 0;
}

/* [] END OF FILE */
