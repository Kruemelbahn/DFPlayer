//=== 2x7Segment === usable for all ====================================
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>  // because use of "Adafruit_MCP23017.h"
#include "4x7SegmentChar.h"

//=== global stuff =======================================

//=== declaration of var's =======================================
#define SEVENSEG_ADDRESS  0x22

Adafruit_MCP23017 portsOut = Adafruit_MCP23017();

uint8_t ui8_2x7SegmentPresent = 0;  // ui8_2x7SegmentPresent: 1 if I2C-2x7Segment-Panel is found
boolean b_CommonAnode(true);	// default
uint8_t ui8_RightDisplay = 0, ui8_LeftDisplay = 0;
uint16_t ui16_MirrorDisplay = 0;

//=== functions for 2x7Segment =======
void CheckAndInit2x7SegmentPanel(boolean b_comAnode)
{
  Wire.begin();
  Wire.beginTransmission(SEVENSEG_ADDRESS);

  boolean b_2x7SegmentDetected(Wire.endTransmission() == 0);
  
  if(b_2x7SegmentDetected && !ui8_2x7SegmentPresent)
  {
    // 2x7Segment (newly) found:
		for (uint8_t i = 0; i <= 15; i++)
			portsOut.pinMode(i, OUTPUT);
		
		ui16_MirrorDisplay = (b_comAnode ? 0 : UINT16_MAX);	// Display off
		portsOut.writeGPIOAB(ui16_MirrorDisplay);

#if defined DEBUG
    Serial.println(F("2x7Segment-Panel found..."));
    Serial.print(F("..."));
#endif

  } // if(b_2x7SegmentDetected && !ui8_2x7SegmentPresent)
  ui8_2x7SegmentPresent = (b_2x7SegmentDetected ? 1 : 0);
  b_CommonAnode = b_comAnode;
}

void outToDisplay1(uint8_t ui8_IndexToCharMap, boolean b_setDP) // right
{
  if(!ui8_2x7SegmentPresent)
    return;

  ui8_RightDisplay = getDigitCode(ui8_IndexToCharMap, b_setDP);
  
  toDisplay();
}

void outToDisplay2(uint8_t ui8_IndexToCharMap, boolean b_setDP) // left
{
  if(!ui8_2x7SegmentPresent)
    return;

  ui8_LeftDisplay = getDigitCode(ui8_IndexToCharMap, b_setDP);
  
  toDisplay();
}

uint8_t getDigitCode(uint8_t ui8_IndexToCharMap, boolean b_setDP)
{
  uint8_t uiRetVal(0);
  if (ui8_IndexToCharMap > MAX_DIGITCODE)
    uiRetVal = digitCodeMap[DIGIT_BLANK]; // BLANK
  else
    uiRetVal = digitCodeMap[ui8_IndexToCharMap];
  if (b_setDP)
    uiRetVal &= digitCodeMap[DIGIT_PERIOD];
  else
    uiRetVal |= digitCodeMap[8];

  if (b_CommonAnode)
    uiRetVal = ~uiRetVal;

  return uiRetVal;
}

void toDisplay()
{
  uint16_t ui16_OutValue((ui8_LeftDisplay << 8) | ui8_RightDisplay);
  if(ui16_OutValue != ui16_MirrorDisplay)
  {	  
    portsOut.writeGPIOAB(ui16_OutValue);
    ui16_MirrorDisplay = ui16_OutValue;
  }
}
