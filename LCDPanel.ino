//=== LCDPanel for DFPlayer ===
#include <Wire.h>
#include <LCDPanel.h>

//=== declaration of var's =======================================

LCDPanel lcd = LCDPanel();

uint8_t ui8_LCDPresent = 0;  // ui8_LCDPresent: 1 if I2C-LCD-Panel is found

/* mode:
  0   after init "AVRSound" is displayed
  1   "Status?" is displayed
  2   "Inbetriebnahme?" is displayed
  5   "Stoerung" is displayed
  7   "FastClock" is displayed
  8   "Steuerung?" is displayed

 10   view current player state
 11   view portexpander state (0x24)
 12   view portexpander state (0x25)
 18   Player-Control
 
 20   (edit) mode for CV1
 21   (edit) mode for CV2
 22   (edit) mode for CV3
 23   (edit) mode for CV4
 24   (edit) mode for CV5
 25   (edit) mode for CV6
 26   (edit) mode for CV7
 27   (edit) mode for CV8
 28   (edit) mode for CV9
 29   (edit) mode for CV10
 30   (edit) mode for CV11
 31   (edit) mode for CV12
 32   (edit) mode for CV13
 33   (edit) mode for CV14
 34   (edit) mode for CV15
 35   (edit) mode for CV16
 36   (edit) mode for CV17
 37   (edit) mode for CV18
 38   (edit) mode for CV19
 
200   confirm display CV's
210   confirm display I2C-Scan
211   I2C-Scan

220   display IP-Address
221   display MAC-Address

*/
uint8_t ui8_LCDPanelMode = 0;  // ui8_LCDPanelMode for paneloperation

uint8_t ui8_ButtonMirror = 0;
uint8_t ui8_CurrentSoundNumberMirror = 0;
uint16_t ui16_SoundMirror = 65535;
uint16_t ui16_StatusMirror = 65535;
uint16_t ui16_PlayerMirror = 65535;
uint16_t ui16_StoerungMirror = 65535;
uint16_t ui16_PortExpanderMirror_24 = 65535;
uint16_t ui16_PortExpanderMirror_25 = 65535;

static const uint8_t MAX_MODE = 19 + GetCVCount();

uint16_t ui16_EditValue = 0;

boolean b_Edit = false;

boolean b_IBN = false;

uint8_t ui8_CursorX = 0;

//=== functions ==================================================
boolean IBNbyLCD() { return b_IBN; }
boolean isPlayerControlActiveFrom_LCD() { return (ui8_LCDPanelMode == 18); }
boolean isButtonOKPressed() { return (lcd.readButtons() & BUTTON_SELECT); }

void CheckAndInitLCDPanel()
{
  boolean b_LCDPanelDetected(lcd.detect_i2c(MCP23017_ADDRESS) == 0);
  if(!b_LCDPanelDetected && ui8_LCDPresent)
    // LCD was present is now absent:
    b_IBN = false;
  
  if(b_LCDPanelDetected && !ui8_LCDPresent)
  {
    // LCD (newly) found:
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
  
#if defined DEBUG
    Serial.println(F("LCD-Panel found..."));
    Serial.print(F("..."));
    Serial.println(GetSwTitle());
#endif

    if(AlreadyCVInitialized())
      OutTextTitle();

    ui8_LCDPanelMode = ui8_ButtonMirror = ui8_CurrentSoundNumberMirror = 0;
    ui16_EditValue = 0;
    ui16_StatusMirror = 65535;
    ui16_PlayerMirror = 65535;
    ui16_StoerungMirror = 65535;
    ui16_PortExpanderMirror_24 = 65535;
    ui16_PortExpanderMirror_25 = 65535;

  } // if(b_LCDPanelDetected && !ui8_LCDPresent)
  ui8_LCDPresent = (b_LCDPanelDetected ? 1 : 0);
}

void SetLCDPanelModeStatus()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Status?"));
  ui8_LCDPanelMode = 1;
}

void SetLCDPanelModeIBN()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Inbetriebnahme?"));
  ui8_LCDPanelMode = 2;
}

void SetLCDPanelModePlayer()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Steuerung?"));
  ui8_LCDPanelMode = 8;
}

void SetLCDPanelModeCV()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("CV?"));
  ui8_LCDPanelMode = 200;
}

void SetLCDPanelModeScan()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("I2C-Scan?"));
  ui8_LCDPanelMode = 210;
}

#if defined ETHERNET_BOARD
void SetLCDPanelModeIpAdr()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("IP-Adresse"));
  lcd.setCursor(0, 1);  // set the cursor to column x, line y
  lcd.print(Ethernet.localIP());
  ui8_LCDPanelMode = 220;
}

void SetLCDPanelModeMacAdr()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("MAC-Adresse"));
  lcd.setCursor(0, 1);  // set the cursor to column x, line y
  uint8_t *mac(GetMACAddress());
  for(uint8_t i = 0; i < 6; i++)
  {
    hexout(lcd, (uint8_t)(mac[i]), 2);
    if((i == 1) || (i == 3))
      lcd.print('.');
  }

  ui8_LCDPanelMode = 221;
}
#endif

void InitScan()
{
  lcd.setCursor(8, 0);  // set the cursor to column x, line y
  lcd.print(' ');       // clear '?'
  ui16_EditValue = -1;
  NextScan();
}

void NextScan()
{
  ++ui16_EditValue;
  lcd.setCursor(0, 1);  // set the cursor to column x, line y

  if (ui16_EditValue > 127)
  {
    OutTextFertig();
    return;
  }

  uint8_t error(0);
  do
  {
    Wire.beginTransmission((uint8_t)(ui16_EditValue));
    error = Wire.endTransmission();

    if ((error == 0) || (error == 4))
    {
      lcd.print(F("0x"));
      hexout(lcd, (uint8_t)(ui16_EditValue), 2);
      lcd.print((error == 4) ? '?' : ' ');
      break;
    }

    ++ui16_EditValue;
    if (ui16_EditValue > 127)
    { // done:
      OutTextFertig();
      break;
    }
  } while (true);
}

void OutTextFertig() { lcd.print(F("fertig")); }
void OutTextOhne() { lcd.print(F("ohne")); }
void OutTextStoerung() { lcd.print(F("Stoerung")); }
void OutTextPortexpander()
{ 
  lcd.print(F("PortExpander "));
  if(IsPortExpanderPresent_24() ||
     IsPortExpanderPresent_25())
  lcd.print(ui8_LCDPanelMode - 10);
}

void OutTextWiedergabe() { lcd.print(F(" Wiedergabe")); }

void OutTextTitle()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(GetSwTitle()); 

  lcd.setCursor(0, 1);
  lcd.print(F("Version "));
  lcd.print(GetCV(VERSION_NUMBER));
#if defined DEBUG
  lcd.print(F(" Deb"));
#endif
#if defined TELEGRAM_FROM_SERIAL
  lcd.print(F("Sim"));
#endif
}

void OutTextPlayerStatus()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Player-Status"));
}

void OutTextPlayerControl()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Player-Control"));
}

void DisplayCV(uint16_t ui16_Value)
{
  lcd.noCursor();
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("CV"));
  lcd.setCursor(3, 0);
  uint8_t ui8CvNr(ui8_LCDPanelMode - 19);
  if (ui8CvNr < 10)
    lcd.print(' ');
  lcd.print(ui8CvNr);
  --ui8CvNr;

  // show shortname:
  lcd.setCursor(6, 0);
  lcd.print(GetCVName(ui8CvNr));

  lcd.setCursor(0, 1);
  if (b_Edit)
    lcd.print('>');
  if (IsCVBinary(ui8CvNr))
    binout(lcd, ui16_Value);
  else
    decout(lcd, ui16_Value, GetCountOfDigits(ui8CvNr));
  if (!b_Edit && !CanEditCV(ui8CvNr))
  {
    lcd.setCursor(14, 1);
    lcd.print(F("ro"));
  }
  if (b_Edit && CanEditCV(ui8CvNr))
  {
    lcd.setCursor(ui8_CursorX, 1);
    lcd.cursor();
  }
}

uint8_t GetCountOfDigits(uint8_t ui8CvNr)
{
  uint16_t ui16MaxCvValue(GetCVMaxValue(ui8CvNr));
  if (ui16MaxCvValue > 9999)
    return 5;
  if (ui16MaxCvValue > 999)
    return 4;
  if (ui16MaxCvValue > 99)
    return 3;
  if (ui16MaxCvValue > 9)
    return 2;
  return 1;
}

uint16_t GetFactor(uint8_t ui8CvNr)
{
  uint16_t ui16_faktor(0);
  uint8_t ui8_Position(GetCountOfDigits(ui8CvNr));
  switch (ui8_Position - ui8_CursorX)
  {
  case 0: ui16_faktor = 1; break;
  case 1: ui16_faktor = 10; break;
  case 2: ui16_faktor = 100; break;
  case 3: ui16_faktor = 1000; break;
  case 4: ui16_faktor = 10000; break;
  }
  return ui16_faktor;
}

void HandleLCDPanel()
{
  CheckAndInitLCDPanel();

  if(ui8_LCDPresent != 1)
    return;

  uint8_t ui8_bs = 1;
  if(lcd.readButtonA5() == BUTTON_A5)
    ui8_bs = 0;
  lcd.setBacklight(ui8_bs);

  uint8_t ui8_buttons = lcd.readButtons();  // reads only 5 buttons (A0...A4)
  if(!ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = 0;
    return;
  }

  if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
#if defined DEBUG
    Serial.print(F("Buttons:"));
    Serial.println(ui8_buttons, BIN);
#endif
    ui8_ButtonMirror = ui8_buttons;
    if(ui8_LCDPanelMode == 0)
    {
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetLCDPanelModeStatus();  // mode = 1
      }
      return;
    } // if(ui8_LCDPanelMode == 0)
    //------------------------------------
    if(ui8_LCDPanelMode == 1)
    {
      // actual ui8_LCDPanelMode = "Status?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Title
        OutTextTitle();
        ui8_LCDPanelMode = 0;
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetLCDPanelModeIBN(); // mode = 2
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current player state
        OutTextPlayerStatus();
        ui8_LCDPanelMode = 10;
        return;
      }
    } // if(ui8_LCDPanelMode == 1)
    //------------------------------------
    if(ui8_LCDPanelMode == 2)
    {
      // actual ui8_LCDPanelMode = "Inbetriebnahme?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Status
        SetLCDPanelModeStatus();  // mode = 1
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to Player-Control
        SetLCDPanelModePlayer(); // mode = 8
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to ask CV / I²C-Scan
        SetLCDPanelModeCV();  // mode = 200;
        return;
      }
    } // if(ui8_LCDPanelMode == 2)
    //------------------------------------
    if(ui8_LCDPanelMode == 5)
    {
      // actual ui8_LCDPanelMode = "Stoerung"
      if (ui8_buttons & BUTTON_FCT_BACK)
      { // any key returns:
        lcd.clear();
        lcd.setCursor(0, 0);  // set the cursor to column x, line y
        if(IsPortExpanderPresent_25())
          ui8_LCDPanelMode = 12;
        else
          ui8_LCDPanelMode = 11;
        OutTextPortexpander();
        return;
      }
    } // if(ui8_LCDPanelMode == 5)
    //------------------------------------
    if(ui8_LCDPanelMode == 7)
    {
      // actual ui8_LCDPanelMode = "FastClock"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Player-Control
        SetLCDPanelModePlayer(); // mode = 8
        return;
      }
    } // if(ui8_LCDPanelMode == 7)
    //------------------------------------
    if(ui8_LCDPanelMode == 8)
    {
      // actual ui8_LCDPanelMode = "Steuerung?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to IBN
        SetLCDPanelModeIBN(); // mode = 2
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to Player-Control
        ui16_EditValue = GetCV(SOUND_NO);
        OutTextPlayerControl();
        ui8_LCDPanelMode = 18;
        stopSound();
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to FastClock
        if(ENABLE_LN_FC_MODUL)
        {
          ui8_LCDPanelMode = 7;
          lcd.clear();
          lcd.setCursor(0, 0);  // set the cursor to column x, line y
          lcd.print(F("FastClock"));
        }
        return;
      }
    } // if(ui8_LCDPanelMode == 8)
    //------------------------------------
    if(ui8_LCDPanelMode == 10)
    {
      // actual ui8_LCDPanelMode = view current player state
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current CV's
        SetLCDPanelModeStatus();  // mode = 1
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // show portexpander 1
        lcd.clear();
        lcd.setCursor(0, 0);  // set the cursor to column x, line y
        ui8_LCDPanelMode = 11;
        OutTextPortexpander();
        return;
      } // if (ui8_buttons & BUTTON_RIGHT)
    } // if(ui8_LCDPanelMode == 10)
    //------------------------------------
    if(ui8_LCDPanelMode == 11)
    {
      // actual ui8_LCDPanelMode = view current player state
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current player state
        OutTextPlayerStatus();
        ui8_LCDPanelMode = 10;
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // show portexpander 2 or Disturbance
        lcd.clear();
        lcd.setCursor(0, 0);  // set the cursor to column x, line y
        if(IsPortExpanderPresent_25())
        {
          ui8_LCDPanelMode = 12;
          OutTextPortexpander();
        }
        else
        {
          OutTextStoerung();
          ui8_LCDPanelMode = 5;
        }
        return;
      } // if (ui8_buttons & BUTTON_RIGHT)
    } // if(ui8_LCDPanelMode == 10)
    //------------------------------------
    if(ui8_LCDPanelMode == 12)
    {
      // actual ui8_LCDPanelMode = view current player state
      if (ui8_buttons & BUTTON_LEFT)
      { // show portexpander 1
        lcd.clear();
        lcd.setCursor(0, 0);  // set the cursor to column x, line y
        ui8_LCDPanelMode = 11;
        OutTextPortexpander();
        return;
      } // if (ui8_buttons & BUTTON_RIGHT)
      if (ui8_buttons & BUTTON_RIGHT)
      { // show disturbance
        lcd.clear();
        lcd.setCursor(0, 0);  // set the cursor to column x, line y
        OutTextStoerung();
        ui8_LCDPanelMode = 5;
        return;
      } // if (ui8_buttons & BUTTON_RIGHT)
    } // if(ui8_LCDPanelMode == 10)
    //------------------------------------
    if(ui8_LCDPanelMode == 18)
    {
      // actual ui8_LCDPanelMode = Player-Control
      if (ui8_buttons & BUTTON_LEFT)
      { // any key returns:
        stopSound();
        SetLCDPanelModePlayer(); // mode = 8
        return;
      }
      if (ui8_buttons & BUTTON_UP)
      {
          stopSound();
          --ui16_EditValue;
          if(ui16_EditValue == 0)
            ui16_EditValue = UINT8_MAX;
          return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
          stopSound();
          ++ui16_EditValue;
          if(ui16_EditValue == 0)
            ui16_EditValue = 1;
          return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      {
          startSound(ui16_EditValue);
          return;
      }
      if (ui8_buttons & BUTTON_SELECT)
      {
          stopSound();
          return;
      }
    } // if(ui8_LCDPanelMode == 18)
    //------------------------------------
    if ((ui8_LCDPanelMode >= 20) && (ui8_LCDPanelMode <= MAX_MODE))
    {
      uint8_t ui8CurrentCvNr(ui8_LCDPanelMode - 20);
      // actual ui8_LCDPanelMode = view CV
      if (ui8_buttons & BUTTON_SELECT)
      { // save value for current CV
        if (b_Edit)
        {
          // save current value
          boolean b_OK(CheckAndWriteCVtoEEPROM(ui8CurrentCvNr, ui16_EditValue));
          DisplayCV(GetCV(ui8CurrentCvNr));
          lcd.setCursor(10, 1);  // set the cursor to column x, line y
          lcd.print(b_OK ? F("stored") : F("failed"));
          lcd.setCursor(ui8_CursorX, 1);
        }
        return;
      }
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current CV's
        if (b_Edit)
        {
          // leave edit-mode (without save)
          b_Edit = false;
          DisplayCV(GetCV(ui8CurrentCvNr));
          return;
        }
        SetLCDPanelModeCV();
        b_IBN = false;
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to edit current CV's
        if (!b_Edit && CanEditCV(ui8CurrentCvNr))
        {
          // enter edit-mode
          ui16_EditValue = GetCV(ui8CurrentCvNr);
          b_Edit = true;
          ui8_CursorX = 0;
          DisplayCV(ui16_EditValue);
        }
        if (b_Edit)
        { // move cursor only in one direction
          ++ui8_CursorX;

          // rollover:
          if (IsCVBinary(ui8CurrentCvNr))
          {
            if (ui8_CursorX == 9)
              ui8_CursorX = 1;
          }
          else
          {
            if (ui8_CursorX == (GetCountOfDigits(ui8CurrentCvNr) + 1))
              ui8_CursorX = 1;
          }

          lcd.setCursor(ui8_CursorX, 1);
          lcd.cursor();
        }
        return;
      }
      if (ui8_buttons & BUTTON_UP)
      {
        if (b_Edit)
        { // Wert vergrößern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (IsCVUI8(ui8CurrentCvNr) || IsCVUI16(ui8CurrentCvNr))
          {
            ui16_EditValue += GetFactor(ui8CurrentCvNr);
            if (ui16_EditValue < ui16_CurrentValue)
              ui16_EditValue = ui16_CurrentValue;  // Überlauf
          }

          if (IsCVBinary(ui8CurrentCvNr))
            ui16_EditValue |= (1 << (8 - ui8_CursorX));

          if (ui16_EditValue > GetCVMaxValue(ui8CurrentCvNr))
            ui16_EditValue = ui16_CurrentValue;  // Überlauf

          DisplayCV(ui16_EditValue);
          return;
        }
        // switch to next CV
        ++ui8_LCDPanelMode;
        if (ui8_LCDPanelMode >= MAX_MODE)
          ui8_LCDPanelMode = MAX_MODE;
        DisplayCV(GetCV(ui8_LCDPanelMode - 20)); // ui8_LCDPanelMode has changed :-)
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        if (b_Edit)
        { // Wert verkleinern:
          uint16_t ui16_CurrentValue(ui16_EditValue);
          if (IsCVUI8(ui8CurrentCvNr) || IsCVUI16(ui8CurrentCvNr))
          {
            ui16_EditValue -= GetFactor(ui8CurrentCvNr);
            if (ui16_EditValue > ui16_CurrentValue)
              ui16_EditValue = ui16_CurrentValue; // Unterlauf
          }

          if (IsCVBinary(ui8CurrentCvNr))
            ui16_EditValue &= ~(1 << (8 - ui8_CursorX));

          if (ui16_EditValue < GetCVMinValue(ui8CurrentCvNr))
            ui16_EditValue = ui16_CurrentValue; // Unterlauf

          if (ui16_EditValue == UINT16_MAX) // 0 -> 255/65535
            ui16_EditValue = ui16_CurrentValue; // Unterlauf

          DisplayCV(ui16_EditValue);
          return;
        }
        // switch to previous CV
        --ui8_LCDPanelMode;
        if (ui8_LCDPanelMode < 20)
          ui8_LCDPanelMode = 20;
        DisplayCV(GetCV(ui8_LCDPanelMode - 20)); // ui8_LCDPanelMode has changed :-)
        return;
      }
    } // if((ui8_LCDPanelMode >= 20) && (ui8_LCDPanelMode <= MAX_MODE))
    //------------------------------------
    if(ui8_LCDPanelMode == 200)
    {
      // actual ui8_LCDPanelMode = "CV?"
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to I²C-Scan
        SetLCDPanelModeScan();  // mode = I²C-Scan
        return;
      }
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to "IBN?"
        SetLCDPanelModeIBN();
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current CV's
        ui8_LCDPanelMode = 20;
        DisplayCV(GetCV(ID_DEVICE));
        b_IBN = true;
        return;
      }
    } // if(ui8_LCDPanelMode == 200)
    //------------------------------------
    if(ui8_LCDPanelMode == 210)
    {
      // actual ui8_LCDPanelMode = "I²C-Scan?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "CV?"
        SetLCDPanelModeCV();
        return;
      }
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to "Inbetriebnahme?"
        SetLCDPanelModeIBN();
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display I²C-Addresses
        ui8_LCDPanelMode = 211;
        InitScan();
        return;
      }
#if defined ETHERNET_BOARD
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to display IP-Address
        SetLCDPanelModeIpAdr();
        return;
      }
#endif
    } // if(ui8_LCDPanelMode == 210)
    //------------------------------------
    if(ui8_LCDPanelMode == 211)
    {
      // actual ui8_LCDPanelMode = "I²C-Scan"
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to "I²C-Scan ?"
        SetLCDPanelModeScan();
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        NextScan();
        return;
      }
    } // if(ui8_LCDPanelMode == 211)
    //------------------------------------
#if defined ETHERNET_BOARD
    if(ui8_LCDPanelMode == 220)
    {
      // actual ui8_LCDPanelMode = "IP-Address"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "I²C-Scan ?"
        SetLCDPanelModeScan();
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to "MAC-Address"
        SetLCDPanelModeMacAdr();
        return;
      }
    } // if(ui8_LCDPanelMode == 220)
    if(ui8_LCDPanelMode == 221)
    {
      // actual ui8_LCDPanelMode = "MAC-Address"
      if (ui8_buttons & BUTTON_FCT_BACK)
      { // any key returns to "IP-Address"
        SetLCDPanelModeIpAdr();
        return;
      }
    } // if(ui8_LCDPanelMode == 221)
#endif
  } // if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  //========================================================================
  if(ui8_LCDPanelMode == 5)
  {
    uint16_t i = GetStoerung();
    if(ui16_StoerungMirror != i)
    {  
      ui16_StoerungMirror = i;
      lcd.setCursor(0, 1);  // set the cursor to column x, line y
      binout(lcd, i >> 8);
      binout(lcd, i & 0x00FF);
   }
  } // if(ui8_LCDPanelMode == 5)
  else
    ui16_StoerungMirror = 65535;
  //====================================
  if(ui8_LCDPanelMode == 7)
  { // display FastClock time in bottom row:
    lcd.setCursor(0, 1);  // set the cursor to column x, line y
    uint8_t ui8_Hour;
    uint8_t ui8_Minute;
    if(GetFastClock(&ui8_Hour, &ui8_Minute))
    {
      if (ui8_Hour < 23)
        decout(lcd, (uint8_t)ui8_Hour, 2);
      else
        lcd.print("??");
      lcd.print(':');
      if (ui8_Minute < 60)
        decout(lcd, (uint8_t)ui8_Minute, 2);
      else
        lcd.print("??");
    }
    else
      lcd.print(F("--:--"));
  } // if(ui8_LCDPanelMode == 7) 
  //====================================
  if(ui8_LCDPanelMode == 10)
  {
    lcd.setCursor(0, 1);  // set the cursor to column x, line y
    uint16_t i = GetPlayerStatus() | (GetValueFromDIPSwitch() << 8);
		uint16_t ii = GetPlayState() | (GetOutState() << 8);
    uint8_t iii = GetCurrentSoundPlaying();
    if((ui16_StatusMirror != i) || (ui16_PlayerMirror != ii) || (ui8_CurrentSoundNumberMirror != iii))
    {
      ui16_StatusMirror = i;
      OutTextPlayerStatus();
      ui16_PlayerMirror = 65535; // force to show state-values (at position 9.1, see below)
      lcd.setCursor(0, 1);  // set the cursor to column x, line y
      binout(lcd, GetPlayerStatus());

      lcd.setCursor(12, 1);  // set the cursor to column x, line y
      uint8_t ui8_ValueFromDIPSwitch(GetValueFromDIPSwitch());
			if(IsSoundFred())
			{
		    if(ui8_ValueFromDIPSwitch & 0x01)
		      ui8_ValueFromDIPSwitch = 1;
		    else if(ui8_ValueFromDIPSwitch & 0x02)
		      ui8_ValueFromDIPSwitch = 2;
		    else if(ui8_ValueFromDIPSwitch & 0x04)
		      ui8_ValueFromDIPSwitch = 3;
		    else if(ui8_ValueFromDIPSwitch & 0x08)
		      ui8_ValueFromDIPSwitch = 4;
        lcd.print('f');
			}
      else if(ENABLE_SOUND_FROM_S2 && !ENABLE_LEVEL_FROM_S2)
      {
        ++ui8_ValueFromDIPSwitch;
        lcd.print('s');
      }
      else if(ENABLE_LEVEL_FROM_S2)
      {
        ui8_ValueFromDIPSwitch <<= 1;
        lcd.print('l');
      }
      else
        lcd.print('.');
      decout(lcd, ui8_ValueFromDIPSwitch, 3);

			lcd.setCursor(9, 1);  // set the cursor to column x, line y
			ui16_PlayerMirror = ii;
			lcd.print(GetPlayState());
			lcd.print(GetOutState());

      lcd.setCursor(14, 0);  // set the cursor to column x, line y
      decout(lcd, iii, 3); // show playing current Sound-No.
      ui8_CurrentSoundNumberMirror = iii;
    } // if((ui16_StatusMirror != i) || (ui16_PlayerMirror != ii) || (ui8_CurrentSoundNumberMirror != iii))
  } // if(ui8_LCDPanelMode == 10)
  else
	{
	  ui16_StatusMirror = 65535;
		ui16_PlayerMirror = 65535;
		ui8_CurrentSoundNumberMirror = 255;
	}
  //====================================
  if(ui8_LCDPanelMode == 11)
  {
    uint16_t i = GetPortExpander_24();
    if(ui16_PortExpanderMirror_24 != i)
    {  
      ui16_PortExpanderMirror_24 = i;
      lcd.setCursor(0, 1);  // set the cursor to column x, line y
      if(IsPortExpanderPresent_24())
      {
        binout(lcd, i >> 8);
        binout(lcd, i & 0x00FF);
      }
      else
        OutTextOhne();
   }
  } // if(ui8_LCDPanelMode == 11)
  else
    ui16_PortExpanderMirror_24 = 65535;
  //====================================
  if(ui8_LCDPanelMode == 12)
  {
    uint16_t i = GetPortExpander_25();
    if(ui16_PortExpanderMirror_25 != i)
    {  
      ui16_PortExpanderMirror_25 = i;
      lcd.setCursor(0, 1);  // set the cursor to column x, line y
      if(IsPortExpanderPresent_25())
      {
        binout(lcd, i >> 8);
        binout(lcd, i & 0x00FF);
      }
      else
        OutTextOhne();
   }
  } // if(ui8_LCDPanelMode == 12)
  else
    ui16_PortExpanderMirror_25 = 65535;
  //====================================
  if(ui8_LCDPanelMode == 18)
  {
    lcd.setCursor(0, 1);  // set the cursor to column x, line y
    uint16_t i = GetPlayerStatus() | (ui16_EditValue << 8);
    if(ui16_SoundMirror != i)
    {
      ui16_SoundMirror = i;
      OutTextPlayerControl();
      lcd.setCursor(0, 1);  // set the cursor to column x, line y
      lcd.print('>');
      decout(lcd, ui16_EditValue, 3);
      if(GetPlayerStatus() & 0x08)
        OutTextWiedergabe();
    } // if(ui16_SoundMirror != i) 
  } // if(ui8_LCDPanelMode == 18)
  else
    ui16_SoundMirror = 65535;
  //====================================
}
