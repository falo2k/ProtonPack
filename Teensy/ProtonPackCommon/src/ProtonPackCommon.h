#pragma once

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial
                              //XXXXXX20CHARXXXXXXXX
const char* trackList[][2] = { 
	{"MUSIC/THEME.WAV",			"Ghostbusters Theme"}, 
	{"MUSIC/CLEANIN.WAV",		"Cleanin' Up The Town"} ,
	{"MUSIC/SAVIN.WAV",			"Savin' The Day"},
	{"MUSIC/TITLE.WAV",			"Main Title Theme"}, 
	{"MUSIC/MAGICCUT.WAV",		"Magic (Cut Version)"}, 
	{"MUSIC/MAGICCUT.WAV",		"Magic (Full Version)"}, 
	{"MUSIC/RGBTHEME.WAV",		"Real Ghostbusters"}, 
	{"MUSIC/INSTRUMENTAL.WAV",	"Instrumental Theme"},
	{"MUSIC/HIGHER.WAV",		"Higher and Higher"},
	{"MUSIC/ONOUROWN.WAV",		"On Our Own"}, 
	{"MUSIC/BUSTIN.WAV",		"Bustin'"},
	{"MUSIC/RUNDMC.WAV",		"Run DMC:Ghostbusters"},
	{"MUSIC/EPICSAX.WAV",		"Epic Sax Guy"},
	{"MUSIC/RICK.WAV",			"Rickroll"} };
const int trackCount = 14;
const int defaultTrack = 0;

// Timings for routines in milliseconds
#define BOOTING_TIME 3900
#define BOOTING_HUM_START 500
#define POWERDOWN_TIME 3100
#define OVERHEAT_TIME 10000
#define FIRING_WARN_TIME 7500
#define FIRING_STOP_TIME 2500
#define VENT_TIME 5000

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
	// TODO: Force State change
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