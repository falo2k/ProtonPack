/*
 This sketch is designed to run on a Teensy LC in the wand of the proton pack.
*/

#include <Bounce2.h>
#include <Encoder.h>
#include <TimerEvent.h>
#include <lcdgfx.h>
#include <lcdgfx_gui.h>
#include <nano_engine_v2.h>
#include <nano_gfx_types.h>
#include "BGSequence.h"
#include <Adafruit_NeoPixel.h>
#include <ProtonPackCommon.h>

#include <CmdMessenger.h>

/*  ----------------------
	Enums for state control 
----------------------- */
enum BarGraphSequences { START, ACTIVE, FIRE1, FIRE2, BGVENT };
BarGraphSequences BGMODE;

enum displayStates {DISPLAY_OFF, VOLUME, TRACK, VENTMODE, INPUTSTATE, BOOT_LOGO};
displayStates displayState;

State state = OFF;

/*  ----------------------
	Pin & Range Definitions
	: Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int SW_ACTIVATE_PIN = A3;
const int SW_ACTIVATE_ON = HIGH;
const int BTN_INTENSIFY_PIN = A2;
const int BTN_INTENSIFY_ON = LOW;
const int SW_LOWER_PIN = A0;
const int SW_LOWER_ON = HIGH;
const int SW_UPPER_PIN = A6;
const int SW_UPPER_ON = HIGH;
const int BTN_TIP_PIN = 2;
const int BTN_TIP_ON = LOW;
const int ROT_BTN_PIN = 3;
const int ROT_A_PIN = 5;
const int ROT_B_PIN = 6;
const int ROT_BTN_ON = LOW;

const int SW_ACTIVATE_BIT = 0b0000001;
const int BTN_INTENSIFY_BIT = 0b0000010;
const int SW_LOWER_BIT = 0b0000100;
const int SW_UPPER_BIT = 0b0001000;
const int BTN_TIP_BIT = 0b0010000;
const int BTN_ROT_BIT = 0b0100000;

// LEDS
const int BARREL_LED_PIN = A8;
const int BARREL_LED_COUNT=  7;
const int TIP_LED_PIN = A9;
const int TIP_LED_COUNT = 1;
const int BODY_LED_PIN = A7;
const int BODY_LED_COUNT = 5;
const int SLO_BOW_INDEX = 0;
const int VENT_INDEX = 1;
const int FRONT_INDEX = 2;
const int TOP1_INDEX = 3;
const int TOP2_INDEX = 4;

/*  ----------------------
	Library objects
----------------------- */
// Display setup
DisplaySSD1306_128x32_I2C display(-1);
unsigned long displayIdleTimeout = 10000;
unsigned long lastDisplayUpdate = 0;

// Neopixels
Adafruit_NeoPixel barrelLights = Adafruit_NeoPixel(BARREL_LED_COUNT, BARREL_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel tipLights = Adafruit_NeoPixel(TIP_LED_COUNT, TIP_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel bodyLights = Adafruit_NeoPixel(BODY_LED_COUNT, BODY_LED_PIN, NEO_GRBW + NEO_KHZ800);

// Bar graph control
BGSequence BarGraph;
unsigned long CuurentBGInterval = BarGraph.IntervalBG;
bool first = true;

// Serial Command Controller - Use hardware serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial1);

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
bool BTN_ROT = false;
int DIR_ROT = 0;
Encoder ROTARY(ROT_A_PIN, ROT_B_PIN);
int lastRotaryPosition = 0;

/*  ----------------------
	Setup and Loops
----------------------- */
void setup() {
	unsigned long currentMillis = millis();

	Serial.begin(9600);

	Serial1.begin(9600);

	//while (!Serial) {
	//	delay(10);
	//}

	// Initialise the display
	display.setFixedFont(ssd1306xled_font6x8);

	display.begin();	
	//display.getInterface().flipVertical(1);
	//display.getInterface().flipHorizontal(1);
	displayBoot(currentMillis);

	// Initiate and clear LEDs
	barrelLights.begin();
	tipLights.begin();
	bodyLights.begin();
	delay(250);
	
	tipLights.clear();
	bodyLights.clear();
	barrelLights.clear();

	barrelLights.show();
	bodyLights.show();
	tipLights.show();

	// Initiate the input switches
	// Input switches - Slider Pins Automatically Analogue Read
	pinMode(SW_ACTIVATE_PIN, INPUT_PULLUP);
	pinMode(BTN_INTENSIFY_PIN, INPUT_PULLUP);
	pinMode(SW_LOWER_PIN, INPUT_PULLUP);
	pinMode(SW_UPPER_PIN, INPUT_PULLUP);
	pinMode(BTN_TIP_PIN, INPUT_PULLUP);
	pinMode(ROT_BTN_PIN, INPUT_PULLUP);
	//pinMode(ROT_A_PIN, INPUT_PULLUP);
	//pinMode(ROT_B_PIN, INPUT_PULLUP);	

	cmdMessenger.printLfCr();
	attachCmdMessengerCallbacks();

	// Set up the bar graph
	BarGraph.BGSeq();
	BarGraph.initiateVariables(ACTIVE);

	
}


int currentBGIndex = 0;
long lastBGUpdate = 0;
long bgInterval = 1000;

void loop() {
	unsigned long currentMillis = millis();

	// Get any updates from pack
	cmdMessenger.feedinSerialData();

	// If state change, updated enum, initliaseState()

	bool inputChanged = checkInputs(currentMillis);

	if (inputChanged) {
		displayInputs(currentMillis);
		if (BTN_INTENSIFY) {
			Serial.println("Sending Intensify Message");
			cmdMessenger.sendCmd(eventIntPressed, BTN_INTENSIFY);
		}

		cmdMessenger.sendCmd(eventActToggle, SW_ACTIVATE);

		cmdMessenger.sendCmdStart(eventButtonChange);
		cmdMessenger.sendCmdArg<int16_t>(UPPER);
		cmdMessenger.sendCmdArg(SW_UPPER);
		cmdMessenger.sendCmdEnd();
		cmdMessenger.sendCmdStart(eventButtonChange);
		cmdMessenger.sendCmdArg<int16_t>(LOWER);
		cmdMessenger.sendCmdArg(SW_LOWER);
		cmdMessenger.sendCmdEnd();

	}

	stateUpdate(currentMillis);

	if (SW_LOWER) {
		updateLeds(currentMillis);
	}
	else {
		barrelLights.clear();
		bodyLights.clear();
		tipLights.clear();
		barrelLights.show();
		bodyLights.show();
		tipLights.show();
	}

	checkDisplayTimeout(currentMillis);

	//BarGraph.sequencePackOn(currentMillis);
	if (currentMillis - lastBGUpdate > bgInterval) {
		BarGraph.clearLEDs();
		BarGraph.lightBar(currentBGIndex);

		currentBGIndex = (currentBGIndex + 1) % 28;

		lastBGUpdate = currentMillis;

	}

}


/*  ----------------------
	LED Functions
----------------------- */
unsigned long lastLEDUpdate = 0;
unsigned long ledCycleInterval = 10000;
int currentBarrel = BARREL_LED_COUNT;
int currentBody = BODY_LED_COUNT;
int currentTip = TIP_LED_COUNT;

void updateLeds(unsigned long currentMillis) {

	if (1) { //currentMillis - lastLEDUpdate > interval) {
		currentBarrel = (currentBarrel + 1) % (BARREL_LED_COUNT + 1);
		currentBody = (currentBody + 1) % (BODY_LED_COUNT + 1);
		currentTip = (currentTip + 1) % (TIP_LED_COUNT + 1);

		barrelLights.clear();
		bodyLights.clear();
		tipLights.clear();

		float rotation = 2.0 * M_PI * (((float)currentMillis - lastLEDUpdate) / ledCycleInterval);
		//Serial.println(rotation);

		int sinVal = ((int)floor(((1.0 + sin(rotation)) / 2) * 255));
		int cosVal = ((int)floor(((1.0 + cos(rotation)) / 2) * 255));

		for (int i = 0; i <= BARREL_LED_COUNT; i++) {
			//if (i == currentBarrel && i < BARREL_LED_COUNT)	barrelLights.setPixelColor(i, barrelLights.Color(10, 0, 0, 0));
			barrelLights.setPixelColor(i, barrelLights.Color(sinVal,cosVal,sinVal));
		}

		for (int i = 0; i <= BODY_LED_COUNT; i++) {
			//if (i == currentBody && i < BODY_LED_COUNT)	bodyLights.setPixelColor(i, bodyLights.Color(0, 0, 10, 0));
			bodyLights.setPixelColor(i, barrelLights.Color(sinVal, cosVal, sinVal));
		}

		for (int i = 0; i <= TIP_LED_COUNT; i++) {
			//if (i == currentTip && i < TIP_LED_COUNT)	tipLights.setPixelColor(i, tipLights.Color(0, 10, 0, 0));
			tipLights.setPixelColor(i, barrelLights.Color(sinVal, cosVal, sinVal));
		}

		if (currentMillis - lastLEDUpdate > ledCycleInterval) {
			lastLEDUpdate = currentMillis;
		}

		//lastLEDUpdate = currentMillis;

		barrelLights.show();
		tipLights.show();
		bodyLights.show();
	}	



}

/*  ----------------------
	LED Functions
----------------------- */
void checkDisplayTimeout(unsigned long currentMillis) {
	if (currentMillis - lastDisplayUpdate > displayIdleTimeout) {
		display.clear();
		lastDisplayUpdate = currentMillis;
		displayState = DISPLAY_OFF;
	}
}

/*  ----------------------
	Display Functions
----------------------- */
//void updateDisplay(unsigned long currentMillis) {
//	display.setFixedFont(ssd1306xled_font6x8);
//	display.clear();
//
//	display.printFixed(0, 8, "Normal text", STYLE_NORMAL);
//	display.printFixed(0, 16, "Bold text", STYLE_BOLD);
//	display.printFixed(0, 24, "Italic text", STYLE_ITALIC);
//	display.invertColors();
//	display.printFixed(0, 0, "Inverted bold", STYLE_BOLD);
//	display.invertColors();
//}

void displayBoot(unsigned long currentMillis) {
	display.setFixedFont(ssd1306xled_font8x16);
	display.clear();

	display.printFixed(8, 7, "Booting Pack ..", STYLE_BOLD);

	lastDisplayUpdate = currentMillis;
	displayState = BOOT_LOGO;
}

void displayInputs(unsigned long currentMillis) {
	displayState = INPUTSTATE;
	display.setFixedFont(ssd1306xled_font6x8);
	display.clear();

	char line[21];
	sprintf(line, " ACT: %i  INT: %i", SW_ACTIVATE, BTN_INTENSIFY);
	display.printFixed(0, 0, line, STYLE_BOLD);
	sprintf(line, " UP:  %i  DWN: %i", SW_UPPER, SW_LOWER);
	display.printFixed(0, 8, line, STYLE_BOLD);
	sprintf(line, " TIP: %i  ROT: %i", BTN_TIP, BTN_ROT);
	display.printFixed(0, 16, line, STYLE_BOLD);
	sprintf(line, " DIR: %i", DIR_ROT);
	display.printFixed(0, 24, line, STYLE_BOLD);

	/*char strBuf[256];
	sprintf(strBuf, "ACTIVATE = %i, INTENSIFY = %i, LOWER = %i, UPPER = %i, TIP = %i, ROT = %i, DIR = %i, POS = %i",
		SW_ACTIVATE, BTN_INTENSIFY, SW_LOWER, SW_UPPER, BTN_TIP, BTN_ROT, DIR_ROT, lastRotaryPosition);
	Serial.println(strBuf);*/

	lastDisplayUpdate = currentMillis;
}

/*  ----------------------
	Input Management
----------------------- */
int checkInputs(unsigned long currentMillis) {
	int initialState = ((int)SW_ACTIVATE * SW_ACTIVATE_BIT) + ((int)BTN_INTENSIFY * BTN_INTENSIFY_BIT) + ((int)SW_LOWER * SW_LOWER_BIT) + ((int)SW_UPPER * SW_UPPER_BIT)
		+ ((int)BTN_TIP * BTN_TIP_BIT) + ((int)BTN_ROT * BTN_ROT_BIT);

	bool change = false;

	SW_ACTIVATE = digitalRead(SW_ACTIVATE_PIN) == SW_ACTIVATE_ON;
	BTN_INTENSIFY = digitalRead(BTN_INTENSIFY_PIN) == BTN_INTENSIFY_ON;
	SW_LOWER = digitalRead(SW_LOWER_PIN) == SW_LOWER_ON;
	SW_UPPER = digitalRead(SW_UPPER_PIN) == SW_UPPER_ON;
	BTN_TIP = digitalRead(BTN_TIP_PIN) == BTN_TIP_ON;
	BTN_ROT = digitalRead(ROT_BTN_PIN) == ROT_BTN_ON;

	int rotaryPosition = ROTARY.read();
	if (rotaryPosition != lastRotaryPosition) {
		// Ignore noisy single steps
		if (abs(rotaryPosition - lastRotaryPosition) > 1) {

			DIR_ROT = (rotaryPosition < lastRotaryPosition) ? -1 : 1;

			lastRotaryPosition = rotaryPosition;

			if (abs(lastRotaryPosition > 10000)) {
				lastRotaryPosition = 0;
				ROTARY.write(0);
			}

			change = true;
		}
	}
	else {
		if (DIR_ROT != 0) {
			change = true;
		}

		DIR_ROT = 0;
	}

	int newState = SW_ACTIVATE + (BTN_INTENSIFY << 1) + (SW_LOWER << 2) + (SW_UPPER << 3) + (BTN_TIP << 4) + (BTN_ROT << 5);

	change |= (initialState ^ newState);

	if (change) {
		displayInputs(currentMillis);
	}

	return initialState ^ newState;
}

/*  ----------------------
	State Management
----------------------- */
void initialiseState(unsigned long currentMillis) {

}

void stateUpdate(unsigned long currentMillis) {

}



/*  ----------------------
	Serial Command Handling
----------------------- */

void attachCmdMessengerCallbacks() {
	cmdMessenger.attach(onUnknownCommand);
}

void onUnknownCommand()
{
	Serial.println("Command without attached callback");
}
