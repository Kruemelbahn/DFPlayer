/*
 * DFPlayer
 
used I²C-Addresses:
  - 0x20 LCD-Panel (optional)
  - 0x22 7Seg-Status-Display (optional)
  - 0x24 first PortExpander (optional)
  - 0x25 second PortExpander (optional)
  - 0x3D FastClock-Interface
  - 0x70 FastClock-LED-Display
  
discrete In/Outs used for functionalities:
  -  0     (used  USB)
  -  1     (used  USB)
  -  2 In  (used  I²C-Interrupt, DCC-In)
  -  3 In   used  Selector S2.0
	-  4 In   used  start sound
  -  5 In   used  stop sound immediatly
  -  6 Out  used  by HeartBeat / Stoerung
  -  7 Out (used              LocoNet [TxD]) 
  -  8 In   used  stop sound (LocoNet [TxD])
  -  9 Out  used  switch     (LocoNet [CTS])
  - 10 In   used  Player [RxD]    [SS]   (used  by ETH-Shield for W1500-select)
  - 11 Out  used  Player [TxD]    [MOSI] (used  by ETH-Shield) 
  - 12 In   used  Player busy     [MISO] (used  by ETH-Shield) 
  - 13 Out  used  Player state    [SCK]  (used  by ETH-Shield) 
  - 14 In   used  Player-Level (Speaker-Signal)
  - 15 In   used  Selector S2.1
  - 16 In   used  Selector S2.2
  - 17 In   used  Selector S2.3
  - 18     (used by I²C: SDA)
  - 19     (used by I²C: SCL)

 *************************************************** 
 *  Copyright (c) 2018 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
 *  All rights reserved.
 *
 *  LICENSE
 *  -------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************
 */

//=== global stuff =======================================
//#define DEBUG 1   // enables Outputs (debugging informations) to Serial monitor (needs ~1k of code)
                  // note: activating SerialMonitor in Arduino-IDE
                  //       will create a reset in software on board!
                  // please comment out also includes in system.ino

//#define DEBUG_CV 1 // enables CV-Output to serial port during program start (saves 180Bytes of code :-)
//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define ETHERNET_BOARD 1  // needs ~11.5k of code, ~180Bytes of RAM
//#define ETHERNET_WITH_LOCOIO 1
//#define ETHERNET_PAGE_SERVER 1

//#define FAST_CLOCK_LOCAL  //use local ports for slave clock if no I2C-clock is found.

//=============================================
// if activated, the software can only be used for Sound-FRED without other possibilitys such as LN, Start stop etc.
//#define _SPECIAL_SOUND_FRED_	

#if defined _SPECIAL_SOUND_FRED_
	#if F_CPU != 8000000
	  #error "Invalid clock speed - must be 8MHz"  
  #endif
#else
	#if F_CPU != 16000000
	  #error "Invalid clock speed - must be 16MHz"  
  #endif
#endif
//=============================================

#define LCD     // used in GlobalOutPrint.ino

#include "CV.h"

// more definitions are made in PlayerControl
#define ENABLE_SOUND_FROM_S2  (GetCV(ADD_FUNCTIONS_1) & 0x01)
#define ENABLE_LEVEL_FROM_S2  (GetCV(ADD_FUNCTIONS_1) & 0x02)
#define ENABLE_LN             (GetCV(LN_FUNCTIONS) & 0x01)
#define ENABLE_LN_E5          (1)
#define ENABLE_LN_16          (GetCV(LN_FUNCTIONS) & 0x02)
#define ENABLE_LN_FC_MODUL    (GetCV(LN_FUNCTIONS) & 0x04)
#define ENABLE_LN_FC_INTERN   (GetCV(LN_FUNCTIONS) & 0x08)
#define ENABLE_LN_FC_INVERT   (GetCV(LN_FUNCTIONS) & 0x20)
#define ENABLE_LN_FC_SLAVE    (ENABLE_LN_FC_MODUL)
#define ENABLE_LN_FC_MASTER   (0)
#define ENABLE_LN_SW_INVERT   (GetCV(LN_FUNCTIONS) & 0x80)
#define ENABLE_LN_FC_JMRI     (GetCV(LN_FUNCTIONS) & 0x10)

#define UNREFERENCED_PARAMETER(P) { (P) = (P); }

#define MANUFACTURER_ID  13   // NMRA: DIY
#define DEVELOPER_ID  58      // NMRA: my ID, should be > 20 (1 = FREMO)

//=============================================

#include <LocoNet.h>  // requested for notifySwitchRequest, notifyFastClock

#include <HeartBeat.h>
HeartBeat oHeartbeat;

//========================================================
void setup()
{
#if defined DEBUG
  // initialize serial and wait for port to open:
  Serial.begin(57600);
#endif

  ReadCVsFromEEPROM();
  
  InitLocoNet();

  InitFastClock();

  CheckAndInitLCDPanel();

  CheckAndInitPlayer();

#if defined ETHERNET_BOARD
  InitEthernet();
#endif
}

void loop()
{
  // generate blinken
  Blinken();

  if(!GetStoerung())
    // light the Heartbeat LED
    oHeartbeat.beat();
  else
    // flash heartbeat-led for disturbance:
    digitalWrite(6, Blinken2Hz());
        
  //=== do LCD handling ==============
  // can be connected every time
  // panel only necessary for setup CV's (or some status informations):
  HandleLCDPanel();
  
  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

  //=== do FastClock handling ==========
  HandleFastClock();

  //=== do Player handling =======
  HandlePlayer();

  //=== do diagnostic handling =======
  CheckAndInit2x7SegmentPanel(true);
  outToDisplay1(GetPlayState(), GetPlayerStatus() & 0x08 ? true : false); // PlayerBusy
  outToDisplay2(GetOutState(), GetPlayerStatus() & 0x10 ? true : false);  // OutputState

#if defined ETHERNET_BOARD
  //=== react on Ethernet requests ===
  HandleEthernetRequests();
#endif
  
#if defined DEBUG
  #if defined DEBUG_MEM
    ViewFreeMemory();  // shows memory usage
    ShowTimeDiff();    // shows time for 1 cycle
  #endif
#endif
}

//=== will be called from LocoNet-Class
// Address: 	Switch Address.
// Output: 		Value 0 for Coil Off, 0x10 for Coil On
// Direction: Value 0 for Thrown/RED, 0x20 for Closed/GREEN
void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction)
{ // OPC_SW_REQ (B0) received:
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  Printout('R');
  PrintAdr(Address);
#endif
  if (Output)
  {
		boolean b_SoundFromLN(Direction ? true : false);  // "Grün" oder "Rot"
    if (ENABLE_LN_SW_INVERT)
      b_SoundFromLN = (Direction ? false : true);  // "Rot oder "Grün"

    uint16_t ui16AdrOn(readAddressFromSound_OnOff());
    if (ui16AdrOn && Address == ui16AdrOn)
      SetSoundStatusFromLN(b_SoundFromLN, 0);
    if (ENABLE_LN_16)
    {
      uint8_t ui8LnMaxAdrCnt(GetCV(LN_MAX_ADRCNT));
      if (ui8LnMaxAdrCnt && ui16AdrOn && (Address >= ui16AdrOn) && (Address < (ui16AdrOn + ui8LnMaxAdrCnt)))
        SetSoundStatusFromLN(b_SoundFromLN, Address - ui16AdrOn + 1);
    } // if(ENABLE_LN_16)

    uint16_t ui16AdrSwOn(readAddressFromSwitchOutput_OnOff());
    if (ui16AdrSwOn && Address == ui16AdrSwOn)
      SetSwitchOutputStatusFromLN(b_SoundFromLN);
  } // if (Output)
}

/*=== will be called from LocoNetFastClockClass
			if telegram is OPC_SL_RD_DATA [0xE7] or OPC_WR_SL_DATA [0xEF] and clk-state != IDLE ==================*/
void notifyFastClock( uint8_t Rate, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Sync )
{
#if defined DEBUG
  Serial.print(F("notifyFastClock "));
  Serial.print(Hour);
  Serial.print(":");
  decout(Serial, Minute, 2);
  Serial.print("[Rate=");
  Serial.print(Rate);
  Serial.print("][Sync=");
  Serial.print(Sync);
	Serial.println("]");
#endif    
  SetFastClock(Rate, Day, Hour, Minute, Sync);
}

void notifyFastClockFracMins(uint16_t FracMins)
{
  HandleFracMins(FracMins);
}
