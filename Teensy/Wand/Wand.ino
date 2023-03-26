/*
 This sketch is designed to run on a Teensy LC in the wand of the proton pack.
*/

#include <Encoder.h>
#include <avdweb_Switch.h>

#include <TimerEvent.h>

#include <lcdgfx.h>
#include <lcdgfx_gui.h>
#include <nano_engine_v2.h>
#include <nano_gfx_types.h>

#include "BGSequence.h"
#include <Adafruit_NeoPixel.h>
#include <CmdMessenger.h>

#include <ProtonPackCommon.h>

/*  ----------------------
	Variables to flip switch directions depending on
	your wiring implementation
----------------------- */
// Use this to flip the rotary switch direction in code
int rotaryDirection = -1;
const bool SW_ACTIVATE_INVERT = false;
const bool SW_LOWER_INVERT = false;
const bool SW_UPPER_INVERT = false;

/*  ----------------------
	Pin & Range Definitions
	: Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int SW_ACTIVATE_PIN = A3;
const int BTN_INTENSIFY_PIN = A2;
const int SW_LOWER_PIN = A0;
const int SW_UPPER_PIN = A6;
const int BTN_TIP_PIN = 2;
const int ROT_BTN_PIN = 3;
const int ROT_A_PIN = 5;
const int ROT_B_PIN = 6;

// LEDS
const int BARREL_LED_PIN = A8;
const int BARREL_LED_COUNT=  7;
const int TIP_LED_PIN = A9;
const int TIP_LED_COUNT = 1;
const int BODY_LED_PIN = A7;
const int BODY_LED_COUNT = 5;
const int SLO_BLO_INDEX = 0;
const int VENT_INDEX = 1;
const int FRONT_INDEX = 2;
const int TOP1_INDEX = 3;
const int TOP2_INDEX = 4;

/*  ----------------------
	State control
----------------------- */
enum BarGraphSequences { START, ACTIVE, FIRE1, FIRE2, BGVENT };
BarGraphSequences BGMODE;

enum displayStates { DISPLAY_OFF, VOLUME, TRACK, VENTMODE, INPUTSTATE, BOOT_LOGO, SAVE_SETTINGS };
displayStates displayState;

State state = OFF;

bool overheatMode = false;
int trackNumber = defaultTrack;

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

Encoder ROTARY(ROT_A_PIN, ROT_B_PIN);

Switch actSwitch = Switch(SW_ACTIVATE_PIN);
Switch intButton = Switch(BTN_INTENSIFY_PIN);
Switch lowerSwitch = Switch(SW_LOWER_PIN);
Switch upperSwitch = Switch(SW_UPPER_PIN);
Switch tipButton = Switch(BTN_TIP_PIN);
Switch encoderButton = Switch(ROT_BTN_PIN);

/*  ----------------------
	Timers
----------------------- */
TimerEvent bodyTimer;
int bodyPeriod = 100;
TimerEvent tipTimer;
int tipPeriod = 100;
TimerEvent barrelTimer;
int barrelPeriod = 100;

/*  ----------------------
	Setup and Loops
----------------------- */
void setup() {
	unsigned long currentMillis = millis();

	// Initialise Serial for Debug + Pack Connection
	Serial.begin(9600);
	Serial1.begin(9600);

	DEBUG_SERIAL.println("Starting Setup");

	// Initialise the display
	display.setFixedFont(ssd1306xled_font6x8);
	display.begin();
	displayBoot(currentMillis);

	// Initialise the pack serial
	cmdMessenger.printLfCr();
	attachCmdMessengerCallbacks();

	// Initialise LEDs and Timers
	barrelLights.begin();
	tipLights.begin();
	bodyLights.begin();
	//delay(250);
	
	tipLights.clear();
	bodyLights.clear();
	barrelLights.clear();

	barrelLights.show();
	bodyLights.show();
	tipLights.show();

	bodyInit(currentMillis);
	bodyTimer.set(bodyPeriod, bodyUpdate);
	barrelInit(currentMillis);
	barrelTimer.set(barrelPeriod, barrelUpdate);
	tipInit(currentMillis);
	tipTimer.set(tipPeriod, tipUpdate);

	// Initiate the input switches
	setupInputs();
	
	// Set up the bar graph
	graphInit(currentMillis);

	DEBUG_SERIAL.println("Setup Complete");
	display.clear();
	displayState = DISPLAY_OFF;
}

int currentBGIndex = 0;
long lastBGUpdate = 0;
long bgInterval = 1000;

void loop() {
	unsigned long currentMillis = millis();

	// Get any updates from pack
	cmdMessenger.feedinSerialData();

	// Process Switch Updates
	updateInputs(currentMillis);

	// Process State Updates
	stateUpdate(currentMillis);

	// Process Display Updates
	checkDisplayTimeout(currentMillis);

	// Process LED Updates
	bodyTimer.update();
	barrelTimer.update();
	tipTimer.update();
	graphUpdate(currentMillis);
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

void checkDisplayTimeout(unsigned long currentMillis) {
	if (currentMillis - lastDisplayUpdate > displayIdleTimeout) {
		display.clear();
		lastDisplayUpdate = currentMillis;
		displayState = DISPLAY_OFF;
	}
}

void displayBoot(unsigned long currentMillis) {
	display.setFixedFont(ssd1306xled_font8x16);
	display.clear();

	display.printFixed(8, 7, "Booting Pack ..", STYLE_BOLD);

	lastDisplayUpdate = currentMillis;
	displayState = BOOT_LOGO;
}

//void displayInputs(unsigned long currentMillis) {
//	displayState = INPUTSTATE;
//	display.setFixedFont(ssd1306xled_font6x8);
//	display.clear();
//
//	char line[21];
//	sprintf(line, " ACT: %i  INT: %i", SW_ACTIVATE, BTN_INTENSIFY);
//	display.printFixed(0, 0, line, STYLE_BOLD);
//	sprintf(line, " UP:  %i  DWN: %i", SW_UPPER, SW_LOWER);
//	display.printFixed(0, 8, line, STYLE_BOLD);
//	sprintf(line, " TIP: %i  ROT: %i", BTN_TIP, BTN_ROT);
//	display.printFixed(0, 16, line, STYLE_BOLD);
//	sprintf(line, " DIR: %i  VAL: %i", DIR_ROT, round(0.25 * lastRotaryPosition));
//	display.printFixed(0, 24, line, STYLE_BOLD);
//
//
//	/*char strBuf[256];
//	sprintf(strBuf, "ACTIVATE = %i, INTENSIFY = %i, LOWER = %i, UPPER = %i, TIP = %i, ROT = %i, DIR = %i, POS = %i",
//		SW_ACTIVATE, BTN_INTENSIFY, SW_LOWER, SW_UPPER, BTN_TIP, BTN_ROT, DIR_ROT, lastRotaryPosition);
//	Serial.println(strBuf);*/
//
//	lastDisplayUpdate = currentMillis;
//}

/*  ----------------------
	Input Management
----------------------- */
void setupInputs() {
	actSwitch.setPushedCallback(&actSwitchToggle);
	actSwitch.setReleasedCallback(&actSwitchToggle);
	lowerSwitch.setPushedCallback(&lowerSwitchToggle);
	lowerSwitch.setReleasedCallback(&lowerSwitchToggle);
	upperSwitch.setPushedCallback(&upperSwitchToggle);
	upperSwitch.setReleasedCallback(&upperSwitchToggle);
	intButton.setPushedCallback(&intButtonPress);
	intButton.setReleasedCallback(&intButtonRelease);
	tipButton.setPushedCallback(&tipButtonPress);
	tipButton.setReleasedCallback(&tipButtonRelease);
	encoderButton.setPushedCallback(&rotaryButtonPress);
	encoderButton.setReleasedCallback(&rotaryButtonRelease);
	encoderButton.setLongPressCallback(&rotaryButtonLongPress);
}

int lastRotaryPosition = 0;
void updateInputs(unsigned long currentMillis) {
	// Handle Switches
	actSwitch.poll();
	intButton.poll();
	lowerSwitch.poll();
	upperSwitch.poll();
	tipButton.poll();
	encoderButton.poll();

	// Handle Rotary Encoder Movements
	// TODO: Rotary Encoder
	int newRotaryPosition = ROTARY.read();
	if (abs(newRotaryPosition - lastRotaryPosition) >= 4) {
		int movement = rotaryDirection * round(0.25 * (newRotaryPosition - lastRotaryPosition));
		DEBUG_SERIAL.print("Rotary Movement: "); DEBUG_SERIAL.println(movement);
		lastRotaryPosition = newRotaryPosition;
	}
}

void actSwitchToggle(void* ref) {
	DEBUG_SERIAL.print("ACT Switch: "); DEBUG_SERIAL.println(actSwitch.on() ^ SW_ACTIVATE_INVERT);
}

void lowerSwitchToggle(void* ref) {
	DEBUG_SERIAL.print("LOWER Switch: "); DEBUG_SERIAL.println(lowerSwitch.on() ^ SW_LOWER_INVERT);
}

void upperSwitchToggle(void* ref) {
	DEBUG_SERIAL.print("UPPER Switch: "); DEBUG_SERIAL.println(upperSwitch.on() ^ SW_UPPER_INVERT);
}

void intButtonPress(void* ref) {
	DEBUG_SERIAL.println("INT Pressed");
}

void intButtonRelease(void* ref) {
	DEBUG_SERIAL.println("INT Released");
}

void tipButtonPress(void* ref) {
	DEBUG_SERIAL.println("Tip Pressed");
}

void tipButtonRelease(void* ref) {
	DEBUG_SERIAL.println("Tip Released");
}

void rotaryButtonPress(void* ref) {
	DEBUG_SERIAL.println("Rotary Pressed");
}

void rotaryButtonRelease(void* ref) {
	DEBUG_SERIAL.println("Rotary Released");
}

void rotaryButtonLongPress(void* ref) {
	DEBUG_SERIAL.println("Rotary Held");
}


/*  ----------------------
	State Management
----------------------- */
void initialiseState(State newState, unsigned long currentMillis) {

	state = newState;

	cmdMessenger.sendCmdStart(eventChangeState);
	cmdMessenger.sendCmdArg(newState);
	cmdMessenger.sendCmdEnd();

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


/*  ----------------------
	Body LED Management
----------------------- */
void bodyInit(unsigned long currentMillis) {

}

void bodyUpdate() {

}

/*  ----------------------
	Barrel LED Management
----------------------- */
void barrelInit(unsigned long currentMillis) {

}

void barrelUpdate() {

}

/*  ----------------------
	Tip LED Management
----------------------- */
void tipInit(unsigned long currentMillis) {

}

void tipUpdate() {

}

/*  ----------------------
	Bar Graph Management
----------------------- */
void graphInit(unsigned long currentMillis) {
	BarGraph.BGSeq();
	BarGraph.initiateVariables(OFF);
}

void graphUpdate(unsigned long currentMillis) {

}
