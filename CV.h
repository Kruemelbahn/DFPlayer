//=== CV-Routines for DFPlayer ===

// give CV a unique name
enum {	ID_DEVICE = 0, SOUND_NO, SOUND_LEVEL, SOUND_MODUS, INP_INVERT, INP_NOT_CONNECTED, VERSION_NUMBER, SOFTWARE_ID, ADD_FUNCTIONS_1, PIN5_LEVEL,
				PIN5_MODUS, PIN9_DELAY_ON, PIN9_DURATION, SOUND_PAUSE, PIN9_LEVEL_ON, PIN9_LEVEL_OFF, LN_FUNCTIONS, LN_ADR_ONOFF, LN_MAX_ADRCNT, SOUND_WAIT,
				DEBOUNCE_ON, LN_SW_ADR_ONOFF, PIN9_DELAY_OFF, SW_WITH_SND, SOUND_ON_STOP
#if defined ETHERNET_BOARD
			, IP_BLOCK_3, IP_BLOCK_4
#endif
};

//=== declaration of var's =======================================
#define PRODUCT_ID SOFTWARE_ID
static const uint8_t DEVICE_ID = 1;     // CV1: Device-ID
static const uint8_t SW_VERSION = 21;   // CV7: Software-Version
static const uint8_t AVRSOUND = 5;      // CV8: Software-ID

static const uint16_t MAX_LN_ADR = 2048;

#if defined ETHERNET_BOARD
  static const uint8_t MAX_CV = 27;
#else
  static const uint8_t MAX_CV = 25;
#endif

uint16_t ui16a_CV[MAX_CV] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
															UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
															UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX
#if defined ETHERNET_BOARD
														, UINT16_MAX, UINT16_MAX
#endif
                          };  // ui16a_CV is a copy from eeprom

struct _CV_DEF // uses 9 Byte of RAM for each CV
{
	uint8_t ID;
	uint16_t DEFLT;
	uint16_t MIN;
	uint16_t MAX;
	uint8_t TYPE;
	boolean RO;
};

enum CV_TYPE { UI8 = 0, UI16 = 1, BINARY = 2 };

#if defined _SPECIAL_SOUND_FRED_
	const struct _CV_DEF cvDefinition[MAX_CV] =
	{ // ID									default value        minValue   maxValue       type							r/o
		 { ID_DEVICE,					DEVICE_ID,           1,         126,           CV_TYPE::UI8,    false}
		,{ SOUND_NO,					1,                   1,					1,						 CV_TYPE::UI8,    true}
		,{ SOUND_LEVEL,				30,                  1,         30,            CV_TYPE::UI8,    true}
		,{ SOUND_MODUS,				4,                   0,         4,             CV_TYPE::UI8,    true}
		,{ INP_INVERT,				0x40,                0,         0x40,					 CV_TYPE::BINARY, true}
		,{ INP_NOT_CONNECTED, 0xB3,                0,         0xB3,					 CV_TYPE::BINARY, true}
		,{ VERSION_NUMBER,		SW_VERSION,          0,         SW_VERSION,    CV_TYPE::UI8,    false}  // normally r/o
		,{ SOFTWARE_ID,				AVRSOUND,						 AVRSOUND,  AVRSOUND,      CV_TYPE::UI8,    true}   // always r/o
		,{ ADD_FUNCTIONS_1,		0x00,                0,         0,						 CV_TYPE::BINARY, true}
		,{ PIN5_LEVEL,				0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ PIN5_MODUS,				0,                   0,         0,             CV_TYPE::UI8,    true}
		,{ PIN9_DELAY_ON,			0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ PIN9_DURATION,			0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ SOUND_PAUSE,				0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ PIN9_LEVEL_ON,			0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ PIN9_LEVEL_OFF,		0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ LN_FUNCTIONS,			0x00,                0,         0x00,  				 CV_TYPE::BINARY, false}
		,{ LN_ADR_ONOFF,			0,                   0,         0,						 CV_TYPE::UI16,   true}
		,{ LN_MAX_ADRCNT,			0,                   0,         0,     				 CV_TYPE::UI8,    true}
		,{ SOUND_WAIT,				0,                   0,         0,						 CV_TYPE::UI8,    true}
	  ,{ DEBOUNCE_ON,       0,                   0,         0,						 CV_TYPE::UI8,    true}
		,{ LN_SW_ADR_ONOFF,		0,                   0,         0,						 CV_TYPE::UI16,   true}
		,{ PIN9_DELAY_OFF,		0,                   0,         0,						 CV_TYPE::UI8,    true}
    ,{ SW_WITH_SND,			  0,                   0,         0,						 CV_TYPE::BINARY, true}
		,{ SOUND_ON_STOP,			0,									 0,					0,						 CV_TYPE::UI8,    true}
	#if defined ETHERNET_BOARD
		,{ IP_BLOCK_3,				2,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}  // IP-Address part 3
		,{ IP_BLOCK_4,				106,                 0,         UINT8_MAX,     CV_TYPE::UI8,    false}  // IP-Address part 4
	#endif
	};
#else	// normal modes
	const struct _CV_DEF cvDefinition[MAX_CV] =
	{ // ID									default value        minValue   maxValue       type							r/o
		 { ID_DEVICE,					DEVICE_ID,           1,         126,           CV_TYPE::UI8,    false}
		,{ SOUND_NO,					1,                   1,					UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ SOUND_LEVEL,				30,                  1,         30,            CV_TYPE::UI8,    false}
		,{ SOUND_MODUS,				0,                   0,         4,             CV_TYPE::UI8,    false}
		,{ INP_INVERT,				0x53,                0,         UINT8_MAX,     CV_TYPE::BINARY, false}
		,{ INP_NOT_CONNECTED, 0x80,                0,         UINT8_MAX,     CV_TYPE::BINARY, false}
		,{ VERSION_NUMBER,		SW_VERSION,          0,         SW_VERSION,    CV_TYPE::UI8,    false}  // normally r/o
		,{ SOFTWARE_ID,				AVRSOUND,						 AVRSOUND,  AVRSOUND,      CV_TYPE::UI8,    true}   // always r/o
		,{ ADD_FUNCTIONS_1,		0x10,                0,         UINT8_MAX,     CV_TYPE::BINARY, false}
		,{ PIN5_LEVEL,				0,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ PIN5_MODUS,				0,                   0,         3,             CV_TYPE::UI8,    false}
		,{ PIN9_DELAY_ON,			0,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ PIN9_DURATION,			0,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ SOUND_PAUSE,				0,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ PIN9_LEVEL_ON,			188,                 0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ PIN9_LEVEL_OFF,		166,                 0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ LN_FUNCTIONS,			0x00,                0,         UINT8_MAX,     CV_TYPE::BINARY, false}
		,{ LN_ADR_ONOFF,			0,                   0,         MAX_LN_ADR,    CV_TYPE::UI16,   false}
		,{ LN_MAX_ADRCNT,			0,                   0,         16,     			 CV_TYPE::UI8,    false}
		,{ SOUND_WAIT,				0,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
	  ,{ DEBOUNCE_ON,       2,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
		,{ LN_SW_ADR_ONOFF,		0,                   0,         MAX_LN_ADR,    CV_TYPE::UI16,   false}
		,{ PIN9_DELAY_OFF,		0,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}
    ,{ SW_WITH_SND,			  0,                   0,         UINT8_MAX,     CV_TYPE::BINARY, false}
		,{ SOUND_ON_STOP,			0,									 0,					UINT8_MAX,		 CV_TYPE::UI8,    false}
	#if defined ETHERNET_BOARD
		,{ IP_BLOCK_3,				2,                   0,         UINT8_MAX,     CV_TYPE::UI8,    false}  // IP-Address part 3
		,{ IP_BLOCK_4,				106,                 0,         UINT8_MAX,     CV_TYPE::UI8,    false}  // IP-Address part 4
	#endif
	};
#endif
//=== naming ==================================================
#if defined _SPECIAL_SOUND_FRED_
	const __FlashStringHelper* GetSwTitle() { return F("AVR-Sound-FRED"); }
#else
	const __FlashStringHelper* GetSwTitle() { return F("AVR-Sound DFPlay"); }
#endif
	//========================================================
	const __FlashStringHelper *GetCVName(uint8_t ui8_Index)
{ 
  // each string should have max. 10 chars
  const __FlashStringHelper *cvName[MAX_CV] = { F("DeviceID")
                                              , F("Snd-No.")
                                              , F("Snd-Level")
                                              , F("Snd-Modus")
                                              , F("Pin inv")
                                              , F("Pin nc.")
                                              , F("Version")
                                              , F("Softw.-ID")
                                              , F("Config")
                                              , F("PD5-Level")
                                              , F("SW-Modus")
                                              , F("SW-DelON")
                                              , F("SW-Durat")
                                              , F("Snd-Pause")
                                              , F("Lev SW-On")
                                              , F("Lev SW-Off")
                                              , F("LN-Func")
                                              , F("Adr LN-Snd")
                                              , F("LN max Adr")
                                              , F("SND-Wait")
                                              , F("Debounc.ON")
                                              , F("Adr LN-SW")
																							, F("SW-DelOFF")
																							, F("SW&Snd-No.")
																							, F("Snd on Stp")
#if defined ETHERNET_BOARD
                                              , F("IP-Part 3")
																						  , F("IP-Part 4")
#endif
                                                };
                                   
  if(ui8_Index < MAX_CV)
    return cvName[ui8_Index];
  return F("???");
}

//=== functions ==================================================
boolean AlreadyCVInitialized() { return (ui16a_CV[SOFTWARE_ID] == AVRSOUND); }
