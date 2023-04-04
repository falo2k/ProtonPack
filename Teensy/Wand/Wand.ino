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

enum displayStates { DISPLAY_OFF, TOP_MENU, VOLUME_CHANGE, VOLUME_DISPLAY, TRACK_SELECT, TRACK_DISPLAY, VENTMODE, INPUTSTATE, BOOT_LOGO, SAVE_SETTINGS };
int selectedIndex = 0;
displayStates displayState;

State state = OFF;

bool overheatMode = false;
bool bluetoothMode = false;
bool musicPlaying = false;
int trackNumber = defaultTrack;

/*  ----------------------
	Library objects
----------------------- */
// Display setup
DisplaySSD1306_128x32_I2C display(-1);
unsigned long displayIdleTimeout = 10000;
unsigned long lastDisplayUpdate = 0;
bool lastDisplayMove = true;

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

	// Delay 500ms to wait for pack to be up first
	delay(500);

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
	setStartupState(currentMillis);
	
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
void checkDisplayTimeout(unsigned long currentMillis) {
	if ((displayState != DISPLAY_OFF) && (currentMillis > lastDisplayUpdate) && (currentMillis - lastDisplayUpdate > displayIdleTimeout)) {
		DEBUG_SERIAL.println("Display timeout");
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

void drawDisplay(unsigned long currentMillis) {
	display.clear();

	switch (displayState){
		case TOP_MENU:
			display.setFixedFont(ssd1306xled_font8x16);
			display.printFixed(10, 0, "Volume", STYLE_BOLD);
			display.printFixed(10, 16, "Track Select", STYLE_BOLD);
			display.printFixed(1, selectedIndex * 16, ">", STYLE_BOLD);
			break;

		case VOLUME_CHANGE:
			display.setFixedFont(ssd1306xled_font8x16);
			// XPos for header starts at 64 - (8*3)
			char volStr[2];
			itoa(selectedIndex, volStr, 10);
			display.printFixed(40, 0, "VOLUME", STYLE_BOLD);
			display.printFixed(56, 16, volStr, STYLE_BOLD);
			break;

		case TRACK_SELECT:
			display.setFixedFont(ssd1306xled_font6x8);
			display.printFixed(6, 2, trackList[limit(selectedIndex-1,trackCount)][1], STYLE_NORMAL);
			display.printFixed(0, 12, ">", STYLE_BOLD);
			display.printFixed(6, 12, trackList[selectedIndex][1], STYLE_BOLD);
			display.printFixed(6, 22, trackList[limit(selectedIndex + 1, trackCount)][1], STYLE_NORMAL);
			setSDTrack(currentMillis, selectedIndex);
			break;

		case VOLUME_DISPLAY:
			display.setFixedFont(ssd1306xled_font8x16);
			char line[21];
			sprintf(line, " Volume: %i", thevol);
			display.printFixed(10, 8, line, STYLE_BOLD);
			break;

		case TRACK_DISPLAY:
			display.setFixedFont(ssd1306xled_font6x8);
			display.printFixed(0, 12, trackList[trackNumber][1], STYLE_BOLD);
			// TODO: Track Select, need text truncate function
			break;

		case DISPLAY_OFF:
		default:
			display.clear();
			break;
	}

	lastDisplayUpdate = currentMillis;
}

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
	intButton.setLongPressCallback(&intButtonLongPress);
	tipButton.setPushedCallback(&tipButtonPress);
	tipButton.setReleasedCallback(&tipButtonRelease);
	tipButton.setLongPressCallback(&tipButtonLongPress);
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
		lastRotaryPosition = newRotaryPosition;

		DEBUG_SERIAL.print("Rotary Movement: "); DEBUG_SERIAL.println(movement);

		rotaryMove(currentMillis, movement);
	}
}

void actSwitchToggle(void* ref) {
	DEBUG_SERIAL.print("ACT Switch: "); DEBUG_SERIAL.println(actSwitch.on() ^ SW_ACTIVATE_INVERT);

	if (actSwitch.on() ^ SW_ACTIVATE_INVERT) {
		// Whether we're going into GB mode or Music mode
		if (upperSwitch.on() ^ SW_UPPER_INVERT) {
			initialiseState(MUSIC_MODE, millis());
		}
		else {
			initialiseState(BOOTING, millis());
		}
	}
	else {
		if (state == MUSIC_MODE) {
			initialiseState(OFF, millis());
		}
		else {
			initialiseState(POWERDOWN, millis());
		}
		
	}
}

void lowerSwitchToggle(void* ref) {
	DEBUG_SERIAL.print("LOWER Switch: "); DEBUG_SERIAL.println(lowerSwitch.on() ^ SW_LOWER_INVERT);

	if (lowerSwitch.on() ^ SW_LOWER_INVERT) {
		overheatMode = true;
		bluetoothMode = true;
	}
	else {
		overheatMode = false;
		bluetoothMode = false;
	}

	cmdMessenger.sendCmdStart(eventSetBluetoothMode);
	cmdMessenger.sendCmdArg(bluetoothMode);
	cmdMessenger.sendCmdEnd();
}

void upperSwitchToggle(void* ref) {
	DEBUG_SERIAL.print("UPPER Switch: "); DEBUG_SERIAL.println(upperSwitch.on() ^ SW_UPPER_INVERT);

	// Only do something if the pack is on, otherwise it doesn't matter what mode we're in
	if (actSwitch.on() ^ SW_ACTIVATE_INVERT) {
		// TODO: MUSIC MODE TOGGLE
	}
	
}

bool intHeld = false;

void intButtonPress(void* ref) {
	DEBUG_SERIAL.println("INT Pressed");

	}

void intButtonLongPress(void* ref) {
	intHeld = true;

	unsigned long currentMillis = millis();

	if ((state == MUSIC_MODE) && (!bluetoothMode)) {
		setSDTrack(currentMillis, limit(trackNumber + 1,trackCount));
		drawDisplay(currentMillis);				
	}

	DEBUG_SERIAL.println("INT Held");
}

void intButtonRelease(void* ref) {
	DEBUG_SERIAL.println("INT Released");
	
	if ((state == MUSIC_MODE) && intHeld) {
		return;
	}

	unsigned long currentMillis = millis();

	switch (state) {
		case MUSIC_MODE:
			// TODO: Play pause handling
			break;

		case IDLE:
		case FIRING:
		case FIRING_WARN:
			initialiseState(FIRING_STOP, currentMillis);
			break;

		default:
			break;
	}

	if (intHeld) {
		intHeld = false;
	}	
}

bool tipHeld = false;

void tipButtonPress(void* ref) {
	DEBUG_SERIAL.println("Tip Pressed");
}

void tipButtonLongPress(void* ref) {
	tipHeld = true;

	DEBUG_SERIAL.println("Tip Held");
}

void tipButtonRelease(void* ref) {
	DEBUG_SERIAL.println("Tip Released");

	// TODO: Code
	
	if (tipHeld) {
		tipHeld = false;
	}
}

bool encoderHeld = false;

void rotaryButtonPress(void* ref) {
	DEBUG_SERIAL.println("Rotary Pressed");
}

void rotaryButtonLongPress(void* ref) {
	encoderHeld = true;
	
	unsigned long currentMillis = millis();

	switch (displayState) {
	case DISPLAY_OFF:
		displayState = TOP_MENU;
		selectedIndex = 0;
		drawDisplay(currentMillis);
		break;

	case TOP_MENU:
		displayState = DISPLAY_OFF;
		selectedIndex = 0;
		drawDisplay(currentMillis);
		break;

	case VOLUME_DISPLAY:
		displayState = VOLUME_CHANGE;
		drawDisplay(currentMillis);
		break;

	case TRACK_DISPLAY:
		displayState = TRACK_SELECT;
		selectedIndex = trackNumber;
		drawDisplay(currentMillis);
		break;
		
	default:
		break;
	}

	DEBUG_SERIAL.println("Rotary Held");
}

void rotaryButtonRelease(void* ref) {
	DEBUG_SERIAL.println("Rotary Released");

	if (encoderHeld) {
		encoderHeld = false;

		return;
	}

	unsigned long currentMillis = millis();

	switch (displayState){
		case TOP_MENU:
			switch (selectedIndex) {
				case 0:
					selectedIndex = thevol;
					displayState = VOLUME_CHANGE;
					break;

				case 1:
					selectedIndex = trackNumber;
					displayState = TRACK_SELECT;
					break;

				default:
					break;
			}
			
			drawDisplay(currentMillis);
			break;

		case VOLUME_CHANGE:
			setVolume(currentMillis, selectedIndex);
			selectedIndex = 0;
			displayState = TOP_MENU;			
			drawDisplay(currentMillis);
			break;

		case TRACK_SELECT:
			setSDTrack(currentMillis, selectedIndex);
			selectedIndex = 0;
			displayState = TOP_MENU;
			drawDisplay(currentMillis);
			break;

		case DISPLAY_OFF:
		case VOLUME_DISPLAY:
		case TRACK_DISPLAY:
		default:
			break;
	}
}

void rotaryMove(unsigned long currentMillis, int movement) {
	if (displayState != DISPLAY_OFF) {
		switch (displayState) {
			case TOP_MENU:
				selectedIndex = limit((selectedIndex + movement) , 2);
				break;

			case VOLUME_CHANGE:
				selectedIndex = (selectedIndex + movement) % maxvol;
				break;

			case TRACK_SELECT:
				selectedIndex = limit((selectedIndex + movement) , trackCount);
				break;

			default:
				break;
		}

		lastDisplayMove = movement > 0;
		drawDisplay(currentMillis);
	}
}


/*  ----------------------
	State Management
----------------------- */
void setStartupState(unsigned long currentMillis) {
	if (actSwitch.on() ^ SW_ACTIVATE_INVERT) {
		if (upperSwitch.on() ^ SW_UPPER_INVERT) {
			initialiseState(MUSIC_MODE, currentMillis);
		}
		else {
			initialiseState(BOOTING, currentMillis);
		}
	}
	else {
		initialiseState(OFF, currentMillis);
	}

	if (lowerSwitch.on() ^ SW_LOWER_INVERT) {
		overheatMode = true;
		bluetoothMode = true;
	}
	else {
		overheatMode = false;
		bluetoothMode = false;
	}

	cmdMessenger.sendCmdStart(eventSetBluetoothMode);
	cmdMessenger.sendCmdArg(bluetoothMode);
	cmdMessenger.sendCmdEnd();
}

void initialiseState(State newState, unsigned long currentMillis) {

	state = newState;

	cmdMessenger.sendCmdStart(eventChangeState);
	cmdMessenger.sendCmdArg(newState);
	cmdMessenger.sendCmdEnd();

}

void stateUpdate(unsigned long currentMillis) {

}

void setSDTrack(unsigned long currentMillis, int newTrackNumber) {
	// TODO: Change the track, change playing if need be, inform pack of change
	trackNumber = newTrackNumber;
}

void setVolume(unsigned long currentMillis, int newVolume) {
	// TODO: Change the pack volume
	thevol = newVolume;
}


/*  ----------------------
	Serial Command Handling
----------------------- */
void attachCmdMessengerCallbacks() {
	cmdMessenger.attach(onUnknownCommand);
}

void onUnknownCommand()
{
	Serial.print("Command without attached callback: "); Serial.println(cmdMessenger.commandID());
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

/*  ----------------------
	Helper Functions
----------------------- */
// Limit with a loop around for negative changes
int limit(int input, int limit) {
	while (input < 0) {
		input += limit;
	}

	return input % limit;
}