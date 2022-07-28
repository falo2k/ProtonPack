// Includes
#include <Adafruit_NeoPixel.h>
#include <GBLEDPatternsJewel.h>

#include <Adafruit_Soundboard.h>
#include <SoftwareSerial.h>
#include <BGSequence.h>

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial
//#define TEST_PIXELS
#define SFX
//#define BARGRAPH

/*  ----------------------
	Pin & Range Definitions
	: Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int SW_ACTIVATE_PIN = 4;
const int SW_ACTIVATE_ON = LOW;
const int BTN_INTENSIFY_PIN = 5;
const int BTN_INTENSIFY_ON = LOW;
const int SW_LOWER_PIN = 6;
const int SW_LOWER_ON = LOW;
const int SW_UPPER_PIN = 7;
const int SW_UPPER_ON = HIGH;
const int BTN_TIP_PIN = 8;
const int BTN_TIP_ON = LOW;

const int SW_BT_PIN = A0;
const int SW_BT_ON = LOW;
const int SW_ION_PIN = A3;
const int SW_ION_ON = LOW;

const int SLD_A_PIN = A7;
const int SLD_B_PIN = A6;
const int SLD_LOW = 250;
const int SLD_MID = 750;

// LEDs
const int PACK_LED_PIN = 2;
const int POWER_CELL_LEDS = 15;
const int CYCLOTRON_LEDS = 7 * 4;
const int PACK_LED_LENGTH = POWER_CELL_LEDS + CYCLOTRON_LEDS;
const int POWER_CELL_RANGE[2] = { 0, POWER_CELL_LEDS };
const int CYCLOTRON_RANGES[4][2] = { {POWER_CELL_LEDS + 1,POWER_CELL_LEDS + 7},{POWER_CELL_LEDS + 8,POWER_CELL_LEDS + 14},{POWER_CELL_LEDS + 15,POWER_CELL_LEDS + 21},{POWER_CELL_LEDS + 22,POWER_CELL_LEDS + 28} };

const int VENT_LED_PIN = 3;
const int VENT_LEDS = 4;
const int WAND_LEDS = 13;
const int VENT_LED_LENGTH = VENT_LEDS + WAND_LEDS;
// TODO Correct VENT LED Range
const int VENT_LED_RANGE[2] = { 0,4 };
const int SLO_BLO_INDEX = 5;
// TODO Remaining light indexes for wand

// Audio FX Board
const int SFX_RX = 10;
const int SFX_TX = 11;
const int SFX_RST = 12;
const int SFX_ACT = 13;

// Relays
const int VENT_RELAY = 9;
const int BT_RELAY1 = A1;
const int BT_RELAY2 = A2;

// Bargraph Controller
const int BAR_SDA = A5;
const int BAR_SCL = A4;

/*  ----------------------
	Audio Track Names

	T00 - Power Up (6s)
	T01 - Fire (22s)
	T02 - Stop Fire (1s)
	T03 - Idle Hum (30s)
	T03_short - Idle Hum (2s)
	T03_VG - Idle Hum Video Game (6s)
	T04 - Power Down (3s)
	T05 - Click (0.5s)
	T06 - Charge (1s)
	T07 - Overheat Fire Warning (22s)
	T08 - Vent-> Idle (6s)
	T09 - Fire End with Dialog (remove)
	T10 -
	T11 -
	T12 -
	T13 - Theme
	T14 - Idling Music
	T15 - Cleaning Up the Town
	T16 - Savin The Day
	T17 - Magic (Edit)
	T18 - Instrumental Theme
	T19 - Bustin
----------------------- */
const char* SFX_music_tracks[9] = { "T13     OGG","T14     OGG","T15     OGG","T16     OGG","T17     OGG","T18     OGG","T19     OGG", "", "" };
const char* SFX_power_up	= "T00     WAV";
const char* SFX_fire		= "T01     WAV";
const char* SFX_ceasefire	= "T02     WAV";
const char* SFX_idle		= "T03     WAV";
const char* SFX_power_down	= "T04     WAV";
const char* SFX_click		= "T05     WAV";
const char* SFX_charge		= "T06     WAV";
const char* SFX_warn		= "T07     WAV";
const char* SFX_vent		= "T08     WAV";
const char* SFX_firedialog	= "T09     WAV";
const char* SFX_idleTrack	= "T03     WAV";

/*  ----------------------
	State
----------------------- */
bool SW_ACTIVATE = false;
bool BTN_INTENSIFY = false;
bool SW_LOWER = false;
bool SW_UPPER = false;
bool BTN_TIP = false;
bool SW_BT = false;
bool SW_ION = false;
int SLD_A = 0;
int SLD_B = 0;
bool SFX_PLAYING = false;

enum PackState { OFF, BOOTING, BOOTED, POWERDOWN };
enum PackState STATUS;

enum WandState { WANDOFF, ON, ON_PACK };
enum WandState WANDSTATUS;

enum WandSLEDState { ALLOFF, WANDONLY, NORMAL, FIRING, WARNING, FASTWARNING, VENTSTATE, STREAMCROSS };
enum WandSLEDState WANDLEDSTATUS;

/*  ----------------------
	Global Objects
----------------------- */
SoftwareSerial sfx_serial = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&sfx_serial, NULL, SFX_RST);

Adafruit_NeoPixel wandLights = Adafruit_NeoPixel(VENT_LED_LENGTH, VENT_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel packLights = Adafruit_NeoPixel(PACK_LED_LENGTH, PACK_LED_PIN, NEO_GRBW + NEO_KHZ800);

void setup() {
	// Set up debug logging
	DEBUG_SERIAL.begin(115200);

	randomSeed((unsigned long)(micros() % millis()));

	DEBUG_SERIAL.println("Setting up input pins ...");
	// Input switches - Slider Pins Automatically Analogue Read
	pinMode(SW_ACTIVATE_PIN, INPUT_PULLUP);
	pinMode(BTN_INTENSIFY_PIN, INPUT_PULLUP);
	pinMode(SW_LOWER_PIN, INPUT_PULLUP);
	pinMode(SW_UPPER_PIN, INPUT_PULLUP);
	pinMode(BTN_TIP_PIN, INPUT_PULLUP);

	pinMode(SW_BT_PIN, INPUT_PULLUP);
	pinMode(SW_ION_PIN, INPUT_PULLUP);

	DEBUG_SERIAL.println("Setting up output pins ...");
	// Relay outputs
	pinMode(VENT_RELAY, OUTPUT);
	pinMode(BT_RELAY1, OUTPUT);
	pinMode(BT_RELAY2, OUTPUT);

	pinMode(SLD_A_PIN, INPUT);
	pinMode(SLD_B_PIN, INPUT);

	// Initiate neopixels and turn off ASAP
	wandLights.begin();
	wandLights.clear();
	wandLights.show();
	packLights.begin();
	packLights.clear();
	packLights.show();

	#ifdef SFX
		// Set up Audio FX Serial
		DEBUG_SERIAL.println("Setting up SFX Board ...");
		sfx_serial.begin(9600);
		if (!sfx.reset()) {
			DEBUG_SERIAL.println("Problem resetting SFX board");
		}
		else {
			#ifdef DEBUG
			uint8_t files = sfx.listFiles();

			DEBUG_SERIAL.println("File Listing");
			DEBUG_SERIAL.println("========================");
			DEBUG_SERIAL.println();
			DEBUG_SERIAL.print("Found "); DEBUG_SERIAL.print(files); DEBUG_SERIAL.println(" Files");
			for (uint8_t f = 0; f < files; f++) {
				DEBUG_SERIAL.print(f);
				DEBUG_SERIAL.print("\tname: "); DEBUG_SERIAL.print(sfx.fileName(f));
				DEBUG_SERIAL.print("\tsize: "); DEBUG_SERIAL.println(sfx.fileSize(f));
		}
			DEBUG_SERIAL.println("========================");
			#endif
		}
	#endif

	#ifdef BARGRAPH
	// Set up Bargraph Serial
	#endif

}

// Spare looping variables
int i = 0;
int j = 0;

void loop() {
	#ifdef TEST_PIXELS

	i = i + 256 % 65536;
	packLights.rainbow(i, -1, 255, 175, true);
	packLights.show();
	wandLights.rainbow(i, -1, 255, 175, true);
	wandLights.show();

	delay(10);
	return;
	#endif

	// put your main code here, to run repeatedly:
	unsigned long currentMillis = millis();


	// TODO Implement a pack harness testing loop

	/* --------
	Activate ON/OFF: Pack Power On/Off

	SW_UPPER ON/OFF: Music Mode toggle

	Sliders: Track selection
	
	-------- */

	if (checkInputs()) {
		char strBuf[256];
		sprintf(strBuf, "ACTIVATE=%i, INTENSIFY=%i, LOWER=%i, UPPER=%i, TIP=%i, BT=%i, ION=%i, SLDA=%i, SLDB=%i, PLAYING=%i",
			SW_ACTIVATE, BTN_INTENSIFY, SW_LOWER, SW_UPPER, BTN_TIP, SW_BT, SW_ION, SLD_A, SLD_B, SFX_PLAYING);
		DEBUG_SERIAL.println(strBuf);
	}

	#ifdef SFX
	if (BTN_INTENSIFY) {
		playTrack(SFX_music_tracks[SLD_A + (3*SLD_B)], true);
	}
	#endif

	delay(1);
	
}


bool checkInputs() {
	int initialState = 0;
	bitWrite(initialState, 0, SW_ACTIVATE);
	bitWrite(initialState, 1, BTN_INTENSIFY);
	bitWrite(initialState, 2, SW_LOWER);
	bitWrite(initialState, 3, SW_UPPER);
	bitWrite(initialState, 4, BTN_TIP);
	bitWrite(initialState, 5, SW_BT);
	bitWrite(initialState, 6, SW_ION);
	bitWrite(initialState, 7, bitRead(SLD_A, 0));
	bitWrite(initialState, 8, bitRead(SLD_A, 1));
	bitWrite(initialState, 9, bitRead(SLD_B, 0));
	bitWrite(initialState, 10, bitRead(SLD_B, 1));
	bitWrite(initialState, 11, SFX_PLAYING);

	SW_ACTIVATE = digitalRead(SW_ACTIVATE_PIN) == SW_ACTIVATE_ON;
	BTN_INTENSIFY = digitalRead(BTN_INTENSIFY_PIN) == BTN_INTENSIFY_ON;
	SW_LOWER = digitalRead(SW_LOWER_PIN) == SW_LOWER_ON;
	SW_UPPER = digitalRead(SW_UPPER_PIN) == SW_UPPER_ON;
	BTN_TIP = digitalRead(BTN_TIP_PIN) == BTN_TIP_ON;

	SW_BT = digitalRead(SW_BT_PIN) == SW_BT_ON;
	SW_ION = digitalRead(SW_ION_PIN) == SW_ION_ON;

	SFX_PLAYING = digitalRead(SFX_ACT) == LOW;

	int SLD_A_val = analogRead(SLD_A_PIN);
	int SLD_B_val = analogRead(SLD_B_PIN);

	if (SLD_A_val < SLD_LOW) {
		SLD_A = 0;
	}
	else if (SLD_A_val < SLD_MID) {
		SLD_A = 1;
	}
	else {
		SLD_A = 2;
	}

	if (SLD_B_val < SLD_LOW) {
		SLD_B = 0;
	}
	else if (SLD_B_val < SLD_MID) {
		SLD_B = 1;
	}
	else {
		SLD_B = 2;
	}

	int newState = 0;
	bitWrite(newState, 0, SW_ACTIVATE);
	bitWrite(newState, 1, BTN_INTENSIFY);
	bitWrite(newState, 2, SW_LOWER);
	bitWrite(newState, 3, SW_UPPER);
	bitWrite(newState, 4, BTN_TIP);
	bitWrite(newState, 5, SW_BT);
	bitWrite(newState, 6, SW_ION);
	bitWrite(newState, 7, bitRead(SLD_A, 0));
	bitWrite(newState, 8, bitRead(SLD_A, 1));
	bitWrite(newState, 9, bitRead(SLD_B, 0));
	bitWrite(newState, 10, bitRead(SLD_B, 1));
	bitWrite(newState, 11, SFX_PLAYING);

	return initialState != newState;
}

bool playTrack(const char* trackName, bool stop) {
	return playTrack((char*)trackName, stop);
}

bool playTrack(char* trackName, bool stop) {
	if (SFX_PLAYING && stop) {
		sfx.stop();
	}

	DEBUG_SERIAL.print("Playing track: "); DEBUG_SERIAL.println(trackName);

	sfx.playTrack(trackName);	
}

void clearLEDs() {
	wandLights.clear();
	packLights.clear();
}