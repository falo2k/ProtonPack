#pragma once

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial

const char* trackList[][2] = { //XXXXXX20CHARXXXXXXXX
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

int8_t thevol = 31;
int8_t maxvol = 63;

enum State { OFF, BOOTING, IDLE, POWERDOWN, FIRING, FIRING_WARN, OVERHEAT, VENTING, FIRING_STOP, MUSIC_MODE };

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
	eventSetVolume,
	eventChangeState,
	eventSaveConfig,
	eventSetBluetoothMode,
	eventSetSDTrack,
	eventPlaySDTrack,
	eventPauseSDTrack,
	eventStopSDTrack,
	eventNextSDTrack,
	eventPreviousSDTrack,
	eventUpdateMusicPlayingState,
	// TODO: Request and save config
	// TODO: Force State change
};