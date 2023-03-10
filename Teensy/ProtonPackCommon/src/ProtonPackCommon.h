#pragma once

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial

const char* trackList[][2] = {
	{"MUSIC/THEME.WAV", "Ghostbusters Theme"}, 
	{"MUSIC/CLEANIN.WAV", "Cleanin' Up The Town"} ,
	{"MUSIC/SAVIN.WAV", "Savin' The Day"},
	{"MUSIC/TITLE.WAV", "Main Title Theme"}, 
	{"MUSIC/MAGICCUT.WAV", "Magic (Cut Version)"}, 
	{"MUSIC/MAGICCUT.WAV", "Magic (Full Version)"}, 
	{"MUSIC/RGBTHEME.WAV", "Real Ghostbusters Theme"}, 
	{"MUSIC/INSTRUMENTAL.WAV", "Instrumental Theme"},
	{"MUSIC/HIGHER.WAV", "Higher and Higher"},
	{"MUSIC/ONOUROWN.WAV", "On Our Own"}, 
	{"MUSIC/BUSTIN.WAV", "Bustin'"},
	{"MUSIC/RUNDMC.WAV", "Ghostbusters (Run D.M.C.)"},
	{"MUSIC/EPICSAX.WAV", "Epic Sax Guy"},
	{"MUSIC/RICK.WAV", "Never Gonna Give You Up"} };
const int trackCount = 14;
const int defaultTrack = 0;


enum State { OFF, WANDON, BOOTING, IDLE, POWERDOWN, FIRING_HEAT, FIRING_NOHEAT, FIRING_OHWARNING, OVERHEAT, FIRING_STOP, MUSIC_MODE };

enum Button { ACTIVATE, INTENSIFY, UPPER, LOWER, ROT, TIP};

// Hardware Serial Comms 
// inintialize hardware constants
const long BAUDRATE = 9600; // speed of serial connection
const long CHARACTER_TIMEOUT = 500; // wait max 500 ms between single chars to be received

// initialize command constants
const byte COMMAND_ID_RECEIVE = 'r';
const byte COMMAND_ID_SEND = 's';

enum SerialCommands {
	eventStatus,
	eventIntPressed,
	eventActToggle,
	eventButtonChange,
	eventUpdateState,
	increaseVolume,
	decreaseVolume,
	eventSetVolume,
	eventChangeState,
	eventSaveConfig,
	eventEnableBluetooth,
	eventSetTrack,
	eventPlayTrack,
	eventPauseTrack,
	eventStopTrack,
};