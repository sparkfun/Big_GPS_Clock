#include <project.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "date_time.h"
#include "gps_meta.h"
#include "ws281x_7seg.h"

// Various support functions.
bool enableUSBCDC();
CY_ISR_PROTO(ClockTickISR);

// Current value of each digit/colons.
volatile bool colon;
volatile bool colonUpdated;

#define TWENTY_FOUR_HOUR_TIME 1

int main()
{
  // This array store the colors for each pixel of each digit. For now, they all
  //  get the same value, but in the future it may be possible to calculate
  //  values for effects like "wipe" or "rainbow".
  uint32_t LEDValues[6][84];

  // Simple: is the segment in the array on or off?
  bool segmentValues[6][7];

  // Data received via UART from the GPS.
  char inboundData[128];
  uint8_t inboundDataIndex = 0;

  date_time currDateTime;

  // Starting at 88:88:88 gives a quick visual check on whether the code is
  //  running or not.
  currDateTime.month = 8;
  currDateTime.day = 8;
  currDateTime.year = 8;
  currDateTime.secs = 8;
  currDateTime.tsecs = 8;
  currDateTime.min = 8;
  currDateTime.tmin = 8;
  currDateTime.hrs = 8;
  
  char strtemp[64];

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
  
  // Clear the memory in the StripLights object. This is *not* the same as the
  //  memory we declared above!
	StripLights_MemClear(StripLights_BLACK);
  
  // Write out the values to the lights. This will blank the display.
  uint8_t digitIndex;
  for (digitIndex = 0; digitIndex < 6; digitIndex++)
  {
    StripChannelSelect_Write(digitIndex); // Point the mux at the string.
    StripLights_Trigger(1);               // Push the data to the string.
    CyDelay(10);                          // Delay a skoosh.
  }
   
  // This bit actually DOES initialize the memory we created. That memory will
  //  be copied over into the memory for the striplight object, and then the
  //  striplight component will read from that memory while writing to the
  //  lights. This "double buffer" means we don't have to worry (as much)
  //  about resource contention on that memory.
  uint16_t initIndex;
  for (initIndex = 0; initIndex<504; initIndex++)
  {
    *(LEDValues[0]+initIndex) = StripLights_WHITE;
  }
  
  // It's good practice to start with the buffer cleared; we don't *know* what
  //  was in there, or what will happen if we don't clear it and try to parse
  //  it, so zap it.
  UART_ClearRxBuffer();

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
        inboundDataIndex = 0;
        parseNMEAData(inboundData, &currDateTime);
      }
    }

    // The colonUpdated flag gets cleared in an ISR; if it's clear, we should
    //  flip the state of the colons so they blink.
    if (!colonUpdated)
    {
      // Channel 6 is the two colon strings, in parallel. They are each tied
      //  to a separate pin, however, so the pin drivers don't get overloaded.
      StripChannelSelect_Write(6);
      uint8_t i = 0;
      if (colon) // colon is true when the colon should be ON
      {
        for (i = 0; i<8; i++)
        {
            StripLights_Pixel(i, 0, StripLights_WHITE);
        }
      }
      else // colon is false when the colon should be OFF
      {
        for (i = 0; i<8; i++)
        {
            StripLights_Pixel(i, 0, StripLights_BLACK);
        }
      }
      // Triggering here writes the data out the proper pins.
      StripLights_Trigger(1); 
      CyDelay(10); // This delay seems needed...something in the component.
      colonUpdated = true; // We don't want to do this every loop.
    }
      
    // Write out the current values to the digits.
    //  Note that we do this every time through the loop; that's easier than
    //  checking to see if the time changed and I'm lazy. All this first part
    //  does is set or clear the booleans in the segmentValues array, based
    //  on the current value for each digit.
    
    // Calculate current local display time, including DST.
    
    int8_t displayHours = currDateTime.hrs;
    if (dstCheck((const date_time*)&currDateTime))
    {
      if(++displayHours > 23)
      {
        displayHours = 0;
      }
    }

#if TWENTY_FOUR_HOUR_TIME    
    if (displayHours < 10)
    {
      writeDigit(segmentValues[5], 0);
      writeDigit(segmentValues[4], displayHours);
    }
    else
    {
      writeDigit(segmentValues[5], 1);
      writeDigit(segmentValues[4], displayHours - 10);
    }
#else
    if (displayHours > 12)
    {
      displayHours -= 12;
    }
    else if (displayHours == 0)
    {
      displayHours = 12;
    }
    if (displayHours < 10)
    {
      writeDigit(segmentValues[5], 0);
      writeDigit(segmentValues[4], displayHours);
    }
    else
    {
      writeDigit(segmentValues[5], 1);
      writeDigit(segmentValues[4], displayHours - 10);
    }
#endif


    writeDigit(segmentValues[3], currDateTime.tmin);
    writeDigit(segmentValues[2], currDateTime.min);
    writeDigit(segmentValues[1], currDateTime.tsecs);
    writeDigit(segmentValues[0], currDateTime.secs);
    
    // Optional debugging statement...
    if (USBCDCOkay)
    {
      if (USBUART_CDCIsReady())
      {
      sprintf(strtemp, "Time: %u:%u%u:%u%u\nDate: %u-%u-%u\n", \
          displayHours,  \
          currDateTime.tmin, \
          currDateTime.min, \
          currDateTime.tsecs, \
          currDateTime.secs, \
          currDateTime.day, \
          currDateTime.month, \
          currDateTime.year );
      USBUART_PutString(strtemp);
      }
    }
    updateDisplay((bool**) LEDValues, (uint32_t**) segmentValues);
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



/* [] END OF FILE */

