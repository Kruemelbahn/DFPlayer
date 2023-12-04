//=== PlayerControl for DFPlayer ===
#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <Bounce.h>

/* description of In's and Out's
  -  0     (used  USB)
  -  1     (used  USB)
  -  2 In  (used  I²C-Interrupt, DCC-In)
  -  3 In   used  Selector S2.0
  -  4 In   used  start sound     [CS]   (used  by ETH-Shield for SD-Card-select, Remark: memory using SD = 4862 Flash, 791 RAM))
  -  5 In   used  stop sound immediatly
  -  6 Out  used  by HeartBeat
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
*/

#define SOUND_START     4
#define SOUND_STOP      5
#define SOUND_STOP_2    8
#define SWITCH_OUT      9

#define PLAYER_RXD     10
#define PLAYER_TXD     11
#define PLAYER_BUSY    12
#define PLAYER_STATE   13
#define PLAYER_LEVEL   14
#define SENSOR_IN       0    // A0

#define S2_0            3
#define S2_1           15
#define S2_2           16
#define S2_3           17

// more definitions are made in DFPlayer
#define ENABLE_PORTEXPANDER   	(GetCV(ADD_FUNCTIONS_1) & 0x04)
#define ENABLE_OFF_IMMEDIATLY 	(GetCV(ADD_FUNCTIONS_1) & 0x08)
#define ENABLE_KEEP_STATE     	(GetCV(ADD_FUNCTIONS_1) & 0x20)
#define ENABLE_SWITCH_AND_SNDNO (GetCV(ADD_FUNCTIONS_1) & 0x40)
#define ENABLE_BARRIER_MODE     (GetCV(ADD_FUNCTIONS_1) & 0x80)

//=== declaration of var's =======================================
/*
 * ETH-Shield uses Arduino-pins: 10, 11, 12, 13
 * therefor we comment out all functions used by DFPlayer 
 *   (e.g. SoftwareSerial, mp3_set_serial, mp3_stop, mp3_play, mp3_set_volume)
 */
#if not defined ETHERNET_BOARD
  SoftwareSerial playerSerial(PLAYER_RXD, PLAYER_TXD); // RX, TX
#endif

#define PORT_EXPANDER_ADDRESS   0x24  //  16E
#define PORT_EXPANDER_ADDRESS_2 0x25  //  16E
Adafruit_MCP23017 portsIn_24 = Adafruit_MCP23017();
Adafruit_MCP23017 portsIn_25 = Adafruit_MCP23017();

// instanciate Bounce-Object for 20ms
Bounce bouncerSoundStart = Bounce(SOUND_START, 20);
Bounce bouncerSoundStop = Bounce(SOUND_STOP, 20);

boolean b_PortExpander_24_present = false;
boolean b_PortExpander_25_present = false;
uint16_t ui16_InPortExpander_24 = 0;
uint16_t ui16_InPortExpander_25 = 0;

uint16_t ui16_Stoerung = 0;

boolean b_SoundStart = false;
boolean b_SoundStop = false;
boolean b_SoundStop2 = false;
boolean b_PlayerBusy = false;
boolean b_PlayerBusyMirror = false;
boolean b_OutputState = false;

boolean b_SoundFromLN = false;
boolean b_SwitchOutputFromLN = false;
uint8_t ui8_SoundNoFromLN = 0;
boolean b_keepPlayingMode3 = false;
boolean b_keepSoundState = false;
boolean b_SwitchStateOffDelay = false;

boolean b_SoundFred = false;

boolean b_PlayerState = false;

uint8_t ui8_ValueFromDIPSwitch = 0;
uint8_t ui8_SoundNoFromPortExpander = 0;
uint8_t ui8_SoundNoFromStop = 0;
uint8_t ui8_PlayerState = 0;
uint8_t ui8_OutputStateMode0 = 0;
uint8_t ui8_OutputStateMode2 = 0;
uint8_t ui8_CurrentSoundPlaying = 0;

#if defined DEBUG
uint8_t ui8_PlayerStateMirror = UINT8_MAX;
uint8_t ui8_OutputStateMode0Mirror = UINT8_MAX;
#endif

unsigned long ul_StartSoundDelay = 0;
unsigned long ul_SoundMillisPause = 0;
unsigned long ul_SoundMillisWait = 0;
unsigned long ul_OutputMillisDelayON = 0;
unsigned long ul_OutputMillisDelayOFF = 0;
unsigned long ul_OutputMillisPulse = 0;

//=== functions ==================================================
void    SetSoundStatusFromLN(boolean bState, uint8_t ui8_SndNo)
{ 
	b_SoundFromLN = bState; 
  if (b_SoundFromLN)  // "Grün" oder "Rot"
    stopSound();  		// stop any playing sound before starting a new one...
	ui8_SoundNoFromLN = ui8_SndNo;
}

void    SetSwitchOutputStatusFromLN(boolean bState) { b_SwitchOutputFromLN = bState; }

uint8_t GetPlayerStatus()
{ 
  uint8_t bReturn(b_PlayerBusy << 3 | b_OutputState << 4 | b_SoundFromLN << 5 | b_PlayerState << 6 | b_SwitchOutputFromLN << 7);
  if(isButtonOKPressed())
    bReturn |= (b_SoundStart | b_SoundStop << 1 | b_SoundStop2 << 2);
  else
    bReturn |= (digitalRead(SOUND_START) | digitalRead(SOUND_STOP) << 1 | digitalRead(SOUND_STOP_2) << 2);
  return bReturn;
}

uint8_t GetValueFromDIPSwitch() { return ui8_ValueFromDIPSwitch; }
uint8_t GetCurrentSoundPlaying() { return ui8_CurrentSoundPlaying; }

uint8_t GetPlayState() { return ui8_PlayerState; }
uint8_t GetOutState()
{ 
  uint8_t ui8_SwitchMode(GetCV(PIN5_MODUS));
  if(ui8_SwitchMode == 0)
    return ui8_OutputStateMode0;
  if (ui8_SwitchMode == 2)
    return ui8_OutputStateMode2;
  return 0;
}

boolean IsSoundFred() { return b_SoundFred; }

uint16_t GetStoerung() { return ui16_Stoerung; }

uint16_t GetPortExpander_24() { return ui16_InPortExpander_24; }
uint16_t GetPortExpander_25() { return ui16_InPortExpander_25; }
boolean IsPortExpanderPresent_24() { return b_PortExpander_24_present; }
boolean IsPortExpanderPresent_25() { return b_PortExpander_25_present; }

void CheckAndInitPlayer()
{
  uint8_t ui8_InvertInputs(GetCV(INP_INVERT));
  uint8_t ui8_NotConnected(GetCV(INP_NOT_CONNECTED));

  boolean bEnableBarrierMode(ENABLE_BARRIER_MODE && !ENABLE_LN);
  if(bEnableBarrierMode)
    ui8_NotConnected &= 0xEC;
#if not defined ETHERNET_BOARD
  if((ui8_NotConnected & 0x01) == 0)
    pinMode(SOUND_START, (ui8_InvertInputs & 0x01) ? INPUT_PULLUP : INPUT);
#endif
  if((ui8_NotConnected & 0x02) == 0)
    pinMode(SOUND_STOP, (ui8_InvertInputs & 0x02) ? INPUT_PULLUP : INPUT);
  if(((ui8_NotConnected & 0x10) == 0) && !ENABLE_LN)
    pinMode(SOUND_STOP_2, (ui8_InvertInputs & 0x10) ? INPUT_PULLUP : INPUT);
  pinMode(PLAYER_BUSY, (ui8_InvertInputs & 0x40) ? INPUT_PULLUP : INPUT);

  pinMode(S2_0, INPUT_PULLUP);
  pinMode(S2_1, INPUT_PULLUP);
  pinMode(S2_2, INPUT_PULLUP);
  pinMode(S2_3, INPUT_PULLUP);

  if((ui8_NotConnected & 0x20) == 0)
    pinMode(SWITCH_OUT, OUTPUT);

  pinMode(PLAYER_STATE, OUTPUT);

  bouncerSoundStart.interval(GetCV(DEBOUNCE_ON) * 10);
  bouncerSoundStop.interval(GetCV(DEBOUNCE_ON) * 10);

  // Note: Analogue pins are automatically set as inputs
  
  //---Portexpander?
  b_PortExpander_24_present = false;
  if(ENABLE_PORTEXPANDER)
  {
    Wire.begin();
    Wire.beginTransmission(PORT_EXPANDER_ADDRESS);
    if(Wire.endTransmission() == 0)
    {
#if defined DEBUG
      Serial.print(F("portexpander at "));
      Serial.println(PORT_EXPANDER_ADDRESS, HEX); 
#endif    
      portsIn_24.begin(PORT_EXPANDER_ADDRESS - 0x20);
    
      for(uint8_t i = 0; i <= 15; i++)
      {
        portsIn_24.pinMode(i, INPUT);
        portsIn_24.pullUp(i, HIGH);
      } // for(uint8_t i = 0; i < 15; i++)
    
      b_PortExpander_24_present = true;

      //--- search for second expander:
      Wire.beginTransmission(PORT_EXPANDER_ADDRESS_2);
      if(Wire.endTransmission() == 0)
      {
#if defined DEBUG
      Serial.print(F("portexpander at "));
      Serial.println(PORT_EXPANDER_ADDRESS_2, HEX); 
#endif    
        portsIn_25.begin(PORT_EXPANDER_ADDRESS_2 - 0x20);
      
        for(uint8_t i = 0; i <= 15; i++)
        {
          portsIn_25.pinMode(i, INPUT);
          portsIn_25.pullUp(i, HIGH);
        } // for(uint8_t i = 0; i < 15; i++)
      
        b_PortExpander_25_present = true;
      } // if(Wire.endTransmission() == 0)
      //--- second expander
    } // if(Wire.endTransmission() == 0)
  }
  //---

#if not defined ETHERNET_BOARD
  #if defined DEBUG
    Serial.println("...initialize DFPlay...");
  #endif
  playerSerial.begin (9600);
  mp3_set_serial (playerSerial);  //set softwareSerial for DFPlayer-mini mp3 module 
  delay(1);
  stopSound();
#endif
}

void HandlePlayer()
{
  uint8_t ui8_InvertInputs(GetCV(INP_INVERT));
  uint8_t ui8_NotConnected(GetCV(INP_NOT_CONNECTED));

/*
  BarrierMode
  - closing barrier using 'Sound ein' (SOUND_START)
  - opening barrier using 'Sound aus' (SOUND_STOP)
  - barrier positionfeedback using 'Sound aus 2' (SOUND_STOP_2)
    -- 'Sound aus 2' must be enabled (CV6 Bit 4 = 0)

  - suppress sound when
    -- closing barrier and barrier is already closed 
    -- opening barrier and barrier is already open 
  
  - enable with CV9 Bit 7 = 1
  - not available when using LN
*/
  boolean bEnableBarrierMode(ENABLE_BARRIER_MODE && !ENABLE_LN);
  if(bEnableBarrierMode)
    ui8_NotConnected &= 0xEC;
  boolean bBarrierOpen(digitalRead(SOUND_STOP_2) == 0);
  if(ui8_InvertInputs & 0x10)
    bBarrierOpen = !bBarrierOpen;
  if(ENABLE_LN)
    bBarrierOpen = bEnableBarrierMode = false;

  // check the inputs
  if(ui8_NotConnected & 0x02)
    b_SoundStop = false;
  else
  {
    // in BarrierMode we want to open the barrier
  	if(GetCV(ADD_FUNCTIONS_1) & 0x10)
		{
			// use debouncing
			bouncerSoundStop.update();
			bouncerSoundStop.read();
			if(ui8_InvertInputs & 0x01)
			{
				if(bouncerSoundStop.risingEdge())  // Pulldown, Taster wird betätigt = geschlossen
        {
          ui8_SoundNoFromStop = 0;  
          b_SoundStop = false;
        }        
				if(bouncerSoundStop.fallingEdge())
        {
          ui8_SoundNoFromStop = GetCV(SOUND_ON_STOP);
          if(bEnableBarrierMode && bBarrierOpen)
            ui8_SoundNoFromStop = 0;  // no Sound on Opening barrier
          if(ui8_SoundNoFromStop > 0)
            stopSoundImmediatly();
					b_SoundStop = true;
        }        
			}
			else		
			{
				if(bouncerSoundStop.risingEdge())
        {
          ui8_SoundNoFromStop = GetCV(SOUND_ON_STOP);
          if(bEnableBarrierMode && bBarrierOpen)
            ui8_SoundNoFromStop = 0;  // no Sound on Opening barrier
          if(ui8_SoundNoFromStop > 0)
            stopSoundImmediatly();
					b_SoundStop = true;
        }        
				if(bouncerSoundStop.fallingEdge())  // Pullup, Taster wird betätigt = geschlossen
        {
          ui8_SoundNoFromStop = 0;  
          b_SoundStop = false;
        }        
			}
		}	// debouncing
    else
    {
      b_SoundStop = (digitalRead(SOUND_STOP) != 0);
      if(bEnableBarrierMode && bBarrierOpen)
        b_SoundStop = false;  // no Sound on Opening barrier
      if(ui8_InvertInputs & 0x02)
        b_SoundStop = !b_SoundStop;
      if (b_SoundStop)
        ui8_SoundNoFromStop = 0;  
		}	// no debouncing
  }
    
  if((ui8_NotConnected & 0x10) || ENABLE_LN)
    b_SoundStop2 = false;
  else
  {
    b_SoundStop2 = (digitalRead(SOUND_STOP_2) != 0);
    if(ui8_InvertInputs & 0x10)
      b_SoundStop2 = !b_SoundStop2;
  }

  if(ui8_NotConnected & 0x01)
    b_SoundStart = false;
  else
  {
    // in BarrierMode we want to close the barrier
		if(GetCV(ADD_FUNCTIONS_1) & 0x10)
		{
			// use debouncing
			bouncerSoundStart.update();
			bouncerSoundStart.read();
			if(ui8_InvertInputs & 0x01)
			{
				if(bouncerSoundStart.risingEdge())  // Pulldown, Taster wird betätigt = geschlossen
					b_SoundStart = false;
				if(bouncerSoundStart.fallingEdge())
        {
					b_SoundStart = true;
          if(bEnableBarrierMode && !bBarrierOpen)
  					b_SoundStart = false;  // no Sound on Closing barrier
        }        
			}
			else		
			{
				if(bouncerSoundStart.risingEdge())
        {
					b_SoundStart = true;
          if(bEnableBarrierMode && !bBarrierOpen)
  					b_SoundStart = false;  // no Sound on Closing barrier
        }        
				if(bouncerSoundStart.fallingEdge())  // Pullup, Taster wird betätigt = geschlossen
					b_SoundStart = false;
			}
		}	// debouncing
		else
		{
			b_SoundStart = (digitalRead(SOUND_START) != 0);
      if(bEnableBarrierMode && !bBarrierOpen)
        b_SoundStart = false;  // no Sound on Closing barrier
			if(ui8_InvertInputs & 0x01)
				b_SoundStart = !b_SoundStart;
		} // no debouncing
  }
    
  boolean b_SoundStopImmediatly(false);
  if(ENABLE_OFF_IMMEDIATLY)
  {
    // use stop immediatly:
    b_SoundStopImmediatly = b_SoundStop;
    b_SoundStop = false;
  }
  else
    b_SoundStop |= b_SoundStop2;
  
  b_PlayerBusy  = (digitalRead(PLAYER_BUSY) != 0);
  if(ui8_InvertInputs & 0x40)
    b_PlayerBusy = !b_PlayerBusy;

	// Kontakte schalten nach GND, d.h. geschaltet = LOW (wichtig für die Sound-FRED-Taster!)
  ui8_ValueFromDIPSwitch = (  (digitalRead(S2_0) == LOW)
                            | (digitalRead(S2_1) == LOW) << 1
                            | (digitalRead(S2_2) == LOW) << 2
                            | (digitalRead(S2_3) == LOW) << 3);

  uint8_t ui8_Modus(GetCV(SOUND_MODUS));
#if defined _SPECIAL_SOUND_FRED_
	b_SoundFred = true;
#else
  b_SoundFred = (ui8_Modus == 4);
#endif  

  // Handle Portexpander:
  if(!b_SoundFred && b_PortExpander_24_present)
  {
    ui16_InPortExpander_24 = portsIn_24.readGPIOAB();
    if(ui16_InPortExpander_24 != UINT16_MAX) // wenigstens ein Eingang ist betätigt (=0)
    {
      uint16_t ui16_In(~ui16_InPortExpander_24); // Eingänge immer 0-aktiv!
      ui8_SoundNoFromPortExpander = 1;
      while(ui16_In >>= 1)
        ++ui8_SoundNoFromPortExpander;
    }
    if(b_PortExpander_25_present)
    {
      ui16_InPortExpander_25 = portsIn_25.readGPIOAB();
      if(ui16_InPortExpander_25 != UINT16_MAX) // wenigstens ein Eingang ist betätigt (=0)
      {
        uint16_t ui16_In(~ui16_InPortExpander_25); // Eingänge immer 0-aktiv!
        ui8_SoundNoFromPortExpander = 16;
        while(ui16_In >>= 1)
          ++ui8_SoundNoFromPortExpander;
      }
    } // if(b_PortExpander_25_present)
  } // if(b_PortExpander_24_present)
  else
    ui8_SoundNoFromPortExpander = 0;
  
  // Handle Player:
  boolean bAnalogInAvailable((ui8_NotConnected & 0x80) == 0);
  if(isPlayerControlActiveFrom_LCD() || IBNbyLCD())
  {
    b_keepPlayingMode3 = false;
    ui8_PlayerState = 0;
  }
  else
  {
    if(b_SoundStopImmediatly)
    {
      // sound stop immediatly using using CV10:
      uint8_t ui8_Pin5Level(GetCV(PIN5_LEVEL));
      if(!bAnalogInAvailable || (ui8_Pin5Level == 0))
        stopSoundImmediatly();
      else
      {
        uint16_t ui16_sensorValue(analogRead(SENSOR_IN));
          if (ui16_sensorValue <= (uint16_t)(ui8_Pin5Level << 3))
            stopSoundImmediatly();
      }
    }
    else
    {
#if defined DEBUG
      if(ui8_PlayerState != ui8_PlayerStateMirror)
      {
        Serial.print(F("Play-State "));
        Serial.print(ui8_PlayerState);
        if(b_keepPlayingMode3)
          Serial.print(F(" keep playing"));
        Serial.println();
        ui8_PlayerStateMirror = ui8_PlayerState;
      }
#endif
      boolean b_SoundOn(b_SoundStart || b_SoundFromLN || (ui8_SoundNoFromPortExpander > 0) || (ui8_SoundNoFromStop > 0));
      if(b_SoundFred)
        b_SoundOn = (ui8_ValueFromDIPSwitch != 0);
      if(!b_SoundOn)
      {
        if(ui8_Modus == 2)
          stopSoundImmediatly();
      }
      if((ui8_Modus == 3) && b_SoundStop)
      {
        b_keepPlayingMode3 = false;
        ui8_PlayerState = 4;
      }
#if defined DEBUG
      if(ui8_PlayerState != ui8_PlayerStateMirror)
      {
        Serial.print(F("Play-State "));
        Serial.print(ui8_PlayerState);
        if(b_keepPlayingMode3)
          Serial.print(F(" keep playing"));
        Serial.println();
        ui8_PlayerStateMirror = ui8_PlayerState;
      }
#endif
      switch(ui8_PlayerState)
      {
        case 0: // start sound?
						    b_PlayerState = b_SoundOn;                
								if(!b_SoundOn)
									break;
                ++ui8_PlayerState;
                startSound();
                ul_StartSoundDelay = millis();
                if(ui8_Modus == 3)
                  b_keepPlayingMode3 = true;
                break;
        case 1: // wait for player busy
                if(b_PlayerBusy)
                {
                  ui16_Stoerung &= ~(1uL);  // Störung löschen
                  ++ui8_PlayerState;  // sound has started
                }
                else
                {
                  if (millis() - ul_StartSoundDelay > 2000) // wait 2s for start - else show error
                  {
#if defined DEBUG
                    Serial.println(F("Sound didnot start"));
#endif
                    ui16_Stoerung |= 1;
                    ui8_PlayerState = 0;
                    b_keepPlayingMode3 = false;
                  }
                }
                break;
        case 2: // wait for sound stop
                if((ui8_Modus == 2) && !b_SoundOn)
                {
                  stopSoundImmediatly();
                  break;
                }
                if(!b_PlayerBusy)
                {
                  ++ui8_PlayerState;  // sound has stopped
                  ul_SoundMillisPause = millis();
                }
                break;
        case 3: // sound has stopped
                {
									if(ui8_Modus == 0)
                  {
                    ui8_PlayerState = 4;
                    break;
                  }
                  if((ui8_Modus == 2) && !b_SoundOn)
                  {
                    stopSoundImmediatly();
                    break;
                  }
                  if(ui8_Modus == 4)
                  {
                    ui8_PlayerState = 8;
                    break;
                  }
                  if(GetCV(SOUND_PAUSE) > 0)
                    ui8_PlayerState = 5;  // wait until pause is over
                  else
                    ui8_PlayerState = 0;  // start sound new
                  break;
                }
        case 4: // wait until b_SoundOn is off;
                if(!b_SoundOn)
                {
                  if(ui8_Modus == 0)
                  {
                    if(GetCV(SOUND_WAIT) > 0)
                    {
                      ul_SoundMillisWait = millis();
                      ui8_PlayerState = 6;
                      break;
                    }
                  }
                  ui8_PlayerState = 0;
                }
                break;
        case 5: // wait until pause is over;
                {
                  unsigned long ul_SoundPause(GetCV(SOUND_PAUSE) * 100);
                  if ((millis() - ul_SoundMillisPause) > ul_SoundPause)
                    ui8_PlayerState = 0;  // pause is over: start sound new
                  break;
                }
        case 6: // wait until wait is over;
                {
                  unsigned long ul_SoundWait(GetCV(SOUND_WAIT) * 1000);  // CV is in seconds!
                  if ((millis() - ul_SoundMillisWait) > ul_SoundWait)
                    ui8_PlayerState = 7;  // wait is over: can start newly
                  break;
                }
        case 7: // wait until b_SoundOn is off;
                if(!b_SoundOn)
                  ui8_PlayerState = 0;
                break;
        case 8: // wait until button connected at dip-switch are off;
                if(!ui8_ValueFromDIPSwitch)
                  ui8_PlayerState = 0;
                break;
      } // switch(ui8_PlayerState)
    } // else
  } // else

  digitalWrite(PLAYER_STATE, b_PlayerState);
  
  // set Switch / Output
  if((ui8_NotConnected & 0x20) == 0)  // Schaltausgang vorhanden
  {
    uint8_t ui8_SwitchMode(GetCV(PIN5_MODUS));
    if(ui8_SwitchMode == 0)
    {
      if(!bAnalogInAvailable) // CV6 Bit7 = 1
      {
#if defined DEBUG
        if(ui8_OutputStateMode0 != ui8_OutputStateMode0Mirror)
        {
          Serial.print(F("Output-State "));
          Serial.print(ui8_OutputStateMode0);
          if(b_keepSoundState)
            Serial.print(F(" keep sound state"));
          Serial.println();
          ui8_OutputStateMode0Mirror = ui8_OutputStateMode0;
        }
#endif
        // follow PD4(4) using CV12, CV13
        if (b_SoundStop2 || b_SoundStopImmediatly)
        {
          b_keepSoundState = false;
          b_SwitchStateOffDelay = false;
        }
        if(b_SoundStart || b_keepSoundState || b_SwitchStateOffDelay) // PD4(4)
        {
          switch(ui8_OutputStateMode0)
          {
            case 0: ul_OutputMillisDelayON = millis();   // keep actual timevalue
                    ++ui8_OutputStateMode0;
                    b_SwitchStateOffDelay = false;
                    b_keepSoundState = ENABLE_KEEP_STATE;
                    break;
            case 1: {
                      if(GetCV(PIN9_DELAY_ON) > 0)
                        ++ui8_OutputStateMode0;
                      else
                      {
                          b_OutputState = true;
                          ui8_OutputStateMode0 = 3;
                          ul_OutputMillisPulse = millis();
                      }
                    }
                    break;
            case 2: // wait until delay is over;
                    {
                      unsigned long ul_OutputDelayON(GetCV(PIN9_DELAY_ON) * 100);
                      if ((millis() - ul_OutputMillisDelayON) > ul_OutputDelayON)
                      {
                        // delay is over:
                        b_OutputState = true;
                        ++ui8_OutputStateMode0;
                        ul_OutputMillisPulse = millis();
                      }
                      break;
                    }
            case 3: {
                      if(GetCV(PIN9_DURATION) > 0)
                        ++ui8_OutputStateMode0;
                      else
                        ui8_OutputStateMode0 = 5;
                    }
                    break;
             case 4:{
                      unsigned long ul_OutputDuration(GetCV(PIN9_DURATION) * 100);
                      if ((millis() - ul_OutputMillisPulse) > ul_OutputDuration)
                      {
                        b_OutputState = false;
                        b_keepSoundState = false;
                        ++ui8_OutputStateMode0;
                      }
                    }
                    break;
             case 5:{
                      if(GetCV(PIN9_DELAY_OFF) > 0)
                      {
                        ul_OutputMillisDelayOFF = millis();
                        ++ui8_OutputStateMode0;
                        b_SwitchStateOffDelay = true;
                      }
                      else
                        ui8_OutputStateMode0 = 7;
                    }
                    break;
             case 6:{
                      unsigned long ul_OutputDelayOFF(GetCV(PIN9_DELAY_OFF) * 100);
                      if ((millis() - ul_OutputMillisDelayOFF) > ul_OutputDelayOFF)
                      {
                        // delay is over:
                        b_OutputState = false;
                        ++ui8_OutputStateMode0;
                      }
                    }
                    break;
             case 7:// wait until b_SoundStart = false
                    b_SwitchStateOffDelay = false;
                    b_keepSoundState = false;
                    break;
          } // switch(ui8_OutputStateMode0)
        } // if(b_SoundStart || b_keepSoundState || b_SwitchStateOffDelay )
        else
        {
          b_OutputState = false;
          ui8_OutputStateMode0 = 0;
        }
      } // if(!bAnalogInAvailable)
    } // if(ui8_SwitchMode == 0)

    if(ui8_SwitchMode == 1)
    {
      if(b_PlayerBusy)
      {
        if(bAnalogInAvailable && !ENABLE_OFF_IMMEDIATLY)
        {
          // level-depending using CV15, CV16:
          uint16_t ui16_sensorValue(analogRead(SENSOR_IN));
          if (ui16_sensorValue >= (uint16_t)(GetCV(PIN9_LEVEL_ON) << 3))
            b_OutputState = true;
          if (ui16_sensorValue <= (uint16_t)(GetCV(PIN9_LEVEL_OFF) << 3))
            b_OutputState = false;
        } // if(bAnalogInAvailable && !ENABLE_OFF_IMMEDIATLY)
      } // if(b_PlayerBusy)
      else
        b_OutputState = false;
    } // if(ui8_SwitchMode == 1)

    if(ui8_SwitchMode == 2)
		{
      b_OutputState = b_PlayerBusy;

      // check if delays are requested
      if (b_PlayerBusy && !b_PlayerBusyMirror && GetCV(PIN9_DELAY_ON))
			{	// rising edge
        if (!ul_OutputMillisDelayON)
          ul_OutputMillisDelayON = millis();
        ui8_OutputStateMode2 = 1; 
			}
			if(!b_PlayerBusy && b_PlayerBusyMirror && GetCV(PIN9_DELAY_OFF))
			{	// falling edge
        if (!ul_OutputMillisDelayOFF)
          ul_OutputMillisDelayOFF = millis();
        ui8_OutputStateMode2 = 2;
      }
			b_PlayerBusyMirror = b_PlayerBusy;

      if(ui8_OutputStateMode2 == 1)
      {
        unsigned long ul_OutputDelayON(GetCV(PIN9_DELAY_ON) * 100);
        if ((millis() - ul_OutputMillisDelayON) > ul_OutputDelayON)
				{
          // delay is over:
          ui8_OutputStateMode2 = 0;
					ul_OutputMillisDelayON = 0;
				}
        else
          b_OutputState = false;
      }
      if(ui8_OutputStateMode2 == 2)
      {
        unsigned long ul_OutputDelayOFF(GetCV(PIN9_DELAY_OFF) * 100);
        if ((millis() - ul_OutputMillisDelayOFF) > ul_OutputDelayOFF)
				{
          // delay is over:
          ui8_OutputStateMode2 = 0;
					ul_OutputMillisDelayOFF = 0;
				}
        else
          b_OutputState = true;
      }
      // --- end

  	} // if(ui8_SwitchMode == 2)

    if(ui8_SwitchMode == 3)
    	b_OutputState = b_SwitchOutputFromLN;
		else
    	b_OutputState |= b_SwitchOutputFromLN;

    // check if SwitchOut is enable by Sound-No
    uint8_t ui8_SwitchWithSound(GetCV(SW_WITH_SND));
    uint8_t ui8_CurrentSndNo(ui8_CurrentSoundPlaying & 0x0F);
    if(ENABLE_SWITCH_AND_SNDNO && ui8_SwitchWithSound && ui8_CurrentSndNo && (ui8_CurrentSndNo <= 8) && b_OutputState)
		{
      uint8_t ui8_SwitchWithSoundMask(1 << (ui8_CurrentSndNo - 1));
      if (!(ui8_SwitchWithSound & ui8_SwitchWithSoundMask))
        b_OutputState = false;
		}
    // --- end

    if(ui8_InvertInputs & 0x20)
      digitalWrite(SWITCH_OUT, !b_OutputState);
    else
      digitalWrite(SWITCH_OUT, b_OutputState);
  } // if((ui8_NotConnected & 0x20) == 0)
}

//--------------------------------------------------------------------------
// encapsulate control-functions
void setSoundLevel(uint8_t ui8_SndLevel)
{
#if defined DEBUG
  Serial.print(F(">volume="));
  Serial.println(ui8_SndLevel);
#endif

#if not defined ETHERNET_BOARD
  delay(1);  //wait 1ms for mp3 module to set volume
  mp3_set_volume (ui8_SndLevel);
  delay(50);
#endif
}

void startSound()
{
  if(b_SoundFred)
  {
#if defined DEBUG
    Serial.println(F(">start as Sound-FRED"));
#endif
    if(ui8_ValueFromDIPSwitch & 0x01)
      startSound(1);
    else if(ui8_ValueFromDIPSwitch & 0x02)
      startSound(2);
    else if(ui8_ValueFromDIPSwitch & 0x04)
      startSound(3);
    else if(ui8_ValueFromDIPSwitch & 0x08)
      startSound(4);
  } // if(b_SoundFred)
  else if(ui8_SoundNoFromStop)
  {
#if defined DEBUG
    Serial.println(F(">start from Stop"));
#endif
    startSound(ui8_SoundNoFromStop);
  } // if(ui8_SoundNoFromStop)
  else if(ui8_SoundNoFromPortExpander)
  {
#if defined DEBUG
    Serial.println(F(">start from portexpander"));
#endif
    startSound(ui8_SoundNoFromPortExpander);
  } // if(ui8_SoundNoFromPortExpander)
  else if(b_SoundFromLN && (ui8_SoundNoFromLN != 0))
  {
#if defined DEBUG
    Serial.println(F(">start from loconet"));
#endif
    startSound(ui8_SoundNoFromLN);
  } // if(b_SoundFromLN && (ui8_SoundNoFromLN != 0))
  else if(ENABLE_SOUND_FROM_S2 && !ENABLE_LEVEL_FROM_S2)
  {
#if defined DEBUG
    Serial.println(F(">start using DIP"));
#endif
    startSound(ui8_ValueFromDIPSwitch + 1);
  } // if(ENABLE_SOUND_FROM_S2 && !ENABLE_LEVEL_FROM_S2)
  else
  {
#if defined DEBUG
    Serial.println(F(">start using cv"));
#endif
    startSound(GetCV(SOUND_NO));
  }
}

void startSound(uint8_t ui8_SndNo)
{
  if(!b_SoundFred && ENABLE_LEVEL_FROM_S2)
    setSoundLevel(ui8_ValueFromDIPSwitch << 1);
  else
	{
#if defined _SPECIAL_SOUND_FRED_
    uint16_t ui16_sensorValue(analogRead(SENSOR_IN));
		long lVal(map((long)(ui16_sensorValue), 0, 1023, 0, 30));
		setSoundLevel((uint8_t)(lVal));
#else
    setSoundLevel(GetCV(SOUND_LEVEL));
#endif
	}

  ui8_CurrentSoundPlaying = ui8_SndNo;
#if not defined ETHERNET_BOARD
#if defined DEBUG
  Serial.print(F(">play="));
  Serial.println(ui8_SndNo);
#endif
  mp3_play(ui8_SndNo);  // play track sd:\mp3\xxxx.mp3
  delay(50);
#endif
}

void stopSoundImmediatly()
{
  stopSound();
  b_keepPlayingMode3 = false;
  ui8_PlayerState = 0;
}

void stopSound()
{
  ui8_CurrentSoundPlaying = 0;
#if not defined ETHERNET_BOARD
#if defined DEBUG
  Serial.println(F(">play=STOP"));
#endif
  mp3_stop();
  delay(50);
#endif
}
