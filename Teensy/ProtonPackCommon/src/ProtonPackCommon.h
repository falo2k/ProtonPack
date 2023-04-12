#pragma once

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial

#include <Adafruit_NeoPixel.h>
							   
const char* trackList[][2] = { //XXXXXX20CHARXXXXXXXX 
	{"MUSIC/THEME.WAV",			"Ghostbusters Theme"}, 
	{"MUSIC/CLEANIN.WAV",		"Cleanin' Up The Town"} ,
	{"MUSIC/SAVIN.WAV",			"Savin' The Day"},
	{"MUSIC/TITLE.WAV",			"Main Title Theme"}, 
	{"MUSIC/MAGICCUT.WAV",		"Magic (Cut Version)"}, 
	{"MUSIC/MAGIC.WAV",	     	"Magic (Full Version)"}, 
	{"MUSIC/RGBTHEME.WAV",		"Real Ghostbusters"}, 
	{"MUSIC/INSTRUM.WAV",   	"Instrumental Theme"},
	{"MUSIC/HIGHER.WAV",		"Higher and Higher"},
	{"MUSIC/ONOUROWN.WAV",		"On Our Own"}, 
	{"MUSIC/BUSTIN.WAV",		"Bustin'"},
	{"MUSIC/RUNDMC.WAV",		"Run DMC:Ghostbusters"},
	{"MUSIC/EPICSAX.WAV",		"Epic Sax Guy"},
	{"MUSIC/RICK.WAV",			"Rickroll"} };
const int trackCount = 14;
const int defaultTrack = 0;

// Timings for routines in milliseconds
#define BOOTING_TIME 3000
#define BOOTING_HUM_START 500
#define POWERDOWN_TIME 3100
#define OVERHEAT_TIME 7500
#define FIRING_WARN_TIME 5000
#define FIRING_STOP_TIME 2500
#define VENT_LIGHT_FADE_TIME 300
#define VENT_TIME 3000

// Tracks (comments are ms length)
const char* sfxBoot = "SFX/powerup.wav"; // 3907
const char* sfxHum = "SFX/amb_hum.wav"; // 2995
const char* sfxDryVent = "SFX/dry_vent.wav"; // 528
const char* sfxFireHead = "SFX/firehead.wav"; // 650
const char* sfxFireLoop = "SFX/fireloop.wav"; // 10700
const char* sfxFireTail = "SFX/firetail.wav"; // 1.550
const char* sfxAltBoot = "SFX/kjhstart.mp3"; // 
const char* sfxAltTail = "SFX/kjhstop.mp3"; //
const char* sfxOverheat = "SFX/overheat.wav"; // 2641
const char* sfxPowerDown = "SFX/powdown.wav"; // 3100
const char* sfxShutdown = "SFX/shutdown.wav"; // 2937
const char* sfxSmoke = "SFX/smoke_01.wav"; // 2682
const char* sfxVent = "SFX/vent.wav"; // 3495

int8_t thevol = 31;
int8_t maxvol = 63;

const uint32_t ledOff = Adafruit_NeoPixel::Color(0, 0, 0, 0);

enum State { OFF = 'O',
	BOOTING = 'B',
	IDLE = 'I',
	POWERDOWN = 'S',
	FIRING = 'F',
	FIRING_WARN = 'W',
	VENTING = 'V',
	FIRING_STOP = 'H',
	MUSIC_MODE = 'M' };

enum ButtonEvent { PRESSED, HELD, RELEASED };

enum displayStates { DISPLAY_OFF, TOP_MENU, VOLUME_CHANGE, VOLUME_DISPLAY, TRACK_SELECT, TRACK_DISPLAY, VENTMODE, INPUTSTATE, BOOT_LOGO, SAVE_SETTINGS };

// Hardware Serial Comms 
// inintialize hardware constants
const long BAUDRATE = 9600; // speed of serial connection
const long CHARACTER_TIMEOUT = 500; // wait max 500 ms between single chars to be received

// initialize command constants
const byte COMMAND_ID_RECEIVE = 'r';
const byte COMMAND_ID_SEND = 's';

enum SerialCommands {
	eventPackIonSwitch,
	eventPackEncoderButton,
	eventPackEncoderTurn,
	eventUpdateMusicPlayingState,
	eventSetVolume,
	eventChangeState,
	eventSetBluetoothMode,
	eventSetSDTrack,
	eventPlayPauseSDTrack,
	eventStopSDTrack,	
	// TODO: Request and save config
	// TODO: Force State change?
};

/*  ----------------------
	Helper Functions
	- Some of these are pulled from the adafruit neopixel 
----------------------- */
// Returns the Red component of a 32-bit color
uint8_t White(uint32_t color)
{
	return (color >> 24) & 0xFF;
}

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color)
{
	return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color)
{
	return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color)
{
	return color & 0xFF;
}

// Interpolate between two colours
uint32_t colourInterpolate(uint32_t c1, uint32_t c2, int step, int totalSteps) {
	uint8_t c1w = (uint8_t)(c1 >> 24), c1r = (uint8_t)(c1 >> 16), c1g = (uint8_t)(c1 >> 8), c1b = (uint8_t)c1;
	uint8_t c2w = (uint8_t)(c2 >> 24), c2r = (uint8_t)(c2 >> 16), c2g = (uint8_t)(c2 >> 8), c2b = (uint8_t)c2;

	return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color((c1r * (totalSteps - step) + c2r * step) / totalSteps,
		(c1g * (totalSteps - step) + c2g * step) / totalSteps,
		(c1b * (totalSteps - step) + c2b * step) / totalSteps,
		(c1w * (totalSteps - step) + c2w * step) / totalSteps));
}


// Lazy colour multiplication - will max out anything that clips
uint32_t colourMultiply(uint32_t c, float multiplier) {
	return Adafruit_NeoPixel::Color(
		min(255, Red(c) * multiplier),
		min(255, Green(c) * multiplier),
		min(255, Blue(c) * multiplier),
		min(255, White(c) * multiplier)
	);
}

// Return color, dimmed by 75%
int32_t DimColor(uint32_t color)
{
	uint32_t dimColor = Adafruit_NeoPixel::Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
	return dimColor;
}

// Limit with a loop around for negative changes
int looparound(int input, int limit) {
	while (input < 0) {
		input += limit;
	}

	return input % limit;
}

const uint8_t bgIndexes[28][2] = {
  {0, 0},
  {1, 0},
  {2, 0},
  {3, 0},
  {0, 1},
  {1, 1},
  {2, 1},
  {3, 1},
  {0, 2},
  {1, 2},
  {2, 2},
  {3, 2},
  {0, 3},
  {1, 3},
  {2, 3},
  {3, 3},
  {0, 4},
  {1, 4},
  {2, 4},
  {3, 4},
  {0, 5},
  {1, 5},
  {2, 5},
  {3, 5},
  {0, 6},
  {1, 6},
  {2, 6},
  {3, 6},
};


//switch (state) {
//case OFF:
//	break;
//
//case BOOTING:
//	break;
//
//case IDLE:
//	break;
//
//case POWERDOWN:
//	break;
//
//case FIRING:
//	break;
//
//case FIRING_WARN:
//	break;
//
//case VENTING:
//	break;
//
//case FIRING_STOP:
//	break;
//
//case MUSIC_MODE:
//	break;
//
//default:
//	break;
//}