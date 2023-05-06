/*
 This sketch is designed to run on a Teensy LC in the wand of the proton pack.
*/

#include <ProtonPackCommon.h>
#include <Encoder.h>
#include <avdweb_Switch.h>
#include <TimerEvent.h>
#include <lcdgfx.h>
#include <lcdgfx_gui.h>
#include <nano_engine_v2.h>
#include <nano_gfx_types.h>
#include <Adafruit_NeoPixel.h>
#include <CmdMessenger.h>
#include <HT16K33.h>

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
const int BARREL_LED_PIN = 22;// A8;
const int BARREL_LED_COUNT=  7;
const int TIP_LED_PIN = 23; // A9;
const int TIP_LED_COUNT = 1;
const int BODY_LED_PIN = 21; // A7;
const int BODY_LED_COUNT = 5;
const int VENT_INDEX = 1;
const int FRONT_INDEX = 0;
const int TOP_FORWARD_INDEX = 2;
const int TOP_BACK_INDEX = 3;
const int SLO_BLO_INDEX = 4;

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

// Bar graph
HT16K33 bargraph = HT16K33();

// Serial Command Controller - Use hardware serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial1);

Encoder ROTARY(ROT_A_PIN, ROT_B_PIN);

Switch actSwitch = Switch(SW_ACTIVATE_PIN);
Switch intButton = Switch(BTN_INTENSIFY_PIN, INPUT_PULLUP, LOW, 50, 1000);
Switch lowerSwitch = Switch(SW_LOWER_PIN);
Switch upperSwitch = Switch(SW_UPPER_PIN);
Switch tipButton = Switch(BTN_TIP_PIN, INPUT_PULLUP, LOW, 50, 1000);
Switch encoderButton = Switch(ROT_BTN_PIN);

/*  ----------------------
	Timers
----------------------- */
TimerEvent bodyTimer;
int bodyPeriod = 25;
TimerEvent tipTimer;
int tipPeriod = 50;
TimerEvent barrelTimer;
int barrelPeriod = 50;

/*  ----------------------
	State control
----------------------- */
int selectedIndex = 0;
displayStates displayState;

State state = OFF;

bool overheatMode = false;
bool bluetoothMode = false;
bool musicPlaying = false;
bool packConnected = false;
int trackNumber = defaultTrack;
unsigned long stateLastChanged;

uint32_t savedTipColour;
unsigned long lastTipChange;
uint32_t savedBodyColours[BODY_LED_COUNT];
unsigned long lastBodyChange[BODY_LED_COUNT];
unsigned long lastBarrelChange;

const float powerDownLEDDecayRate = (float)255 / (0.8 * POWERDOWN_TIME);

const int numFiringLEDColours = 10;
const uint32_t firingLEDColours[numFiringLEDColours] = {
	Adafruit_NeoPixel::Color(0,0,0,0),
	Adafruit_NeoPixel::Color(0,0,255,0),
	Adafruit_NeoPixel::Color(5,0,0,50),
	Adafruit_NeoPixel::Color(0,0,150,0),
	Adafruit_NeoPixel::Color(0,0,0,255),
	Adafruit_NeoPixel::Color(5,0,100,255),
	Adafruit_NeoPixel::Color(0,0,200,100),
	Adafruit_NeoPixel::Color(0,0,5,255),
	Adafruit_NeoPixel::Color(0,0,175,100),
	Adafruit_NeoPixel::Color(0,0,0,50)
};

const int firingBlinkMS = 500;
const int warningBlinkMS = 150;
const int ventFlickerPercentage = 15;

const uint32_t ventColour = Adafruit_NeoPixel::Color(250, 250, 250, 250);
const uint32_t ventFlickerColour = Adafruit_NeoPixel::Color(0, 0, 250, 100);
const uint32_t tipColour = Adafruit_NeoPixel::Color(0, 0, 0, 100);
const uint32_t frontColour = Adafruit_NeoPixel::Color(200, 200, 200, 200);
const uint32_t topForwardBatteryColour = Adafruit_NeoPixel::Color(0, 0, 50, 0);
const uint32_t topForwardColour = Adafruit_NeoPixel::Color(200, 200, 200, 0);
const uint32_t topBackColour = Adafruit_NeoPixel::Color(0, 0, 0, 100);
const uint32_t sloBloColour = Adafruit_NeoPixel::Color(255, 0, 0, 0);

const int bargraphLEDs = 28;
int bargraphLitCells = 0;
bool bargraphClimbing = true;
const unsigned int bargraphIdlePeriod = 1500;
unsigned int bargraphPeriod = 1500;

unsigned long lastBargraphChange;
int bargraphCache[bargraphLEDs];

const unsigned int bargraphFiringStartPeriod = 1000;
const unsigned int bargraphFiringMinPeriod = 100;
const unsigned int bargraphFiringStopPeriod = 1200;
const int bargraphFiringWidth = 3;
const float bargraphFiringAcceleration = 0.5;

bool firstLoop = true;

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

	// Set up the bar graph
	graphInit(currentMillis);
	
	// Initiate the input switches
	setupInputs();
	
	DEBUG_SERIAL.println("Setup Complete");
	display.clear();
	displayState = DISPLAY_OFF;

	stateLastChanged = currentMillis;
}

// Tracker time to space out checks for pack connection
unsigned long lastPackCheck = millis();

void loop() {
	unsigned long currentMillis = millis();
	
	// Pack handshake so a demo mode can be used for non-wand setup
	if (!packConnected) {
		if (((currentMillis - lastPackCheck) > 250)) {
			if (!(displayState == WAITING)) {
				displayState = WAITING;
				drawDisplay(currentMillis);
			}
			DEBUG_SERIAL.println("Wand waiting for Pack ...");
			cmdMessenger.sendCmd(eventWandConnect);
			lastPackCheck = currentMillis;
		}
		cmdMessenger.feedinSerialData();
		return;
	}

	if (firstLoop) {
		// Tell the pack to load the configuration file on first loop
		DEBUG_SERIAL.println("First loop in Wand");
		cmdMessenger.sendCmd(eventLoadConfig);
		setStartupState(currentMillis);
		firstLoop = false;
	}

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
	Bargraph Helper Functions
----------------------- */
void setBGLamp(unsigned long currentMillis, int bar) {
	setBGLamp(currentMillis, bar, true);
}

void setBGLamp(unsigned long currentMillis, int bar, bool state) {
	if (bar == 0) {
		return;
	}
	bargraph.setPixel(bgIndexes[bar-1][0], bgIndexes[bar - 1][1], state ? 1 : 0);
	bargraphCache[bar - 1] = state ? 1 : 0;
	lastBargraphChange = currentMillis;
}

void setBGLampRange(unsigned long currentMillis, int from, int to) {
	setBGLampRange(currentMillis, from, to, true);
}

void setBGLampRange(unsigned long currentMillis, int from, int to, bool state) {
	for (int i = from; i <= to; i++) {
		setBGLamp(currentMillis, i, state);
	}
}

void clearBargraph(unsigned long currentMillis) {
	bargraph.clear();

	for (int i = 0; i < bargraphLEDs; i++) {
		bargraphCache[i] = 0;
	}

	lastBargraphChange = currentMillis;	
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
	displayState = BOOT_LOGO;

	display.drawBitmap1(0, 0, 128, 32, bootLogo);

	lastDisplayUpdate = currentMillis;
	displayState = BOOT_LOGO;
}

void drawDisplay(unsigned long currentMillis) {
	display.clear();

	//DEBUG_SERIAL.print("Drawing Display: "); DEBUG_SERIAL.println(displayState);
	//DEBUG_SERIAL.print("Selected Index: "); DEBUG_SERIAL.println(selectedIndex);

	switch (displayState){
		case WAITING:
			display.drawBitmap1(0, 0, 128, 32, waitLogo);
			break;

		case TOP_MENU:
			display.setFixedFont(ssd1306xled_font6x8);
			display.printFixed(10, 0, "Volume", STYLE_BOLD);
			display.printFixed(10, 8, "Track Select", STYLE_BOLD);
			display.printFixed(10, 16, "Load Config", STYLE_BOLD);
			display.printFixed(10, 24, "Save Config", STYLE_BOLD);
			display.printFixed(1, selectedIndex * 8, ">", STYLE_BOLD);
			break;

		case VOLUME_CHANGE:
		case VOLUME_DISPLAY: {
			display.setFixedFont(ssd1306xled_font8x16);
			// XPos for header starts at 64 - (8*3)
			int volPos = 14 - 8 + floor(100.0 * selectedIndex / maxVol);
			char volStr[2];
			sprintf(volStr, "%02d", selectedIndex);
			display.printFixed(40, 0, "VOLUME", STYLE_BOLD);
			display.printFixed(volPos, 16, volStr, STYLE_NORMAL);
			display.drawVLine(4, 17, 31);
			display.drawVLine(124, 17, 31);
			display.drawHLine(5, 24, volPos - 2);
			display.drawHLine(volPos + 16 + 2, 24, 123);
			}
			break;

		case TRACK_SELECT:
			display.setFixedFont(ssd1306xled_font6x8);
			display.printFixed(6, 2, trackList[looparound(selectedIndex-1,trackCount)][1], STYLE_NORMAL);
			display.printFixed(0, 12, ">", STYLE_BOLD);
			display.printFixed(6, 12, trackList[selectedIndex][1], STYLE_BOLD);
			display.printFixed(6, 22, trackList[looparound(selectedIndex + 1, trackCount)][1], STYLE_NORMAL);			
			break;

		case TRACK_DISPLAY:
			display.setFixedFont(ssd1306xled_font6x8);
			DEBUG_SERIAL.print("Displaying Track Name: "); DEBUG_SERIAL.println(trackList[trackNumber][1]);
			display.printFixed(0, 12, trackList[trackNumber][1], STYLE_BOLD);
			break;

		case LOAD_CONFIG:
			display.setFixedFont(ssd1306xled_font8x16);
			display.printFixed(64 - (6 * 8), 0, "Confirm Load", STYLE_BOLD);
			if (selectedIndex == 0) {
				display.printFixed(84 - (1.5 * 8), 16, "YES", STYLE_NORMAL);
				display.printFixed(42 - 8 - (1.5 * 8), 16, ">", STYLE_BOLD);
				display.printFixed(42 + 8 + (0.5 * 8), 16, "<", STYLE_BOLD);
				display.invertColors();
				display.printFixed(42 - 8, 16, "NO", STYLE_BOLD);
				display.invertColors();
			}
			else {
				display.printFixed(42 - 8, 16, "NO", STYLE_BOLD);
				display.printFixed(84 - (1.5 * 8) - (1.5 * 8), 16, ">", STYLE_BOLD);
				display.printFixed(84 + (1.5 * 8) + (0.5 * 8), 16, "<", STYLE_BOLD);
				display.invertColors();
				display.printFixed(84 - (1.5 * 8), 16, "YES", STYLE_NORMAL);
				display.invertColors();
			}
			break;

		case SAVE_CONFIG:
			display.setFixedFont(ssd1306xled_font8x16);
			display.printFixed(64 - (6 * 8), 0, "Confirm Save", STYLE_BOLD);
			if (selectedIndex == 0) {
				display.printFixed(84 - (1.5 * 8), 16, "YES", STYLE_NORMAL);
				display.printFixed(42 - 8 - (1.5 * 8), 16, ">", STYLE_BOLD);
				display.printFixed(42 + 8 + (0.5 * 8), 16, "<", STYLE_BOLD);
				display.invertColors();
				display.printFixed(42 - 8, 16, "NO", STYLE_BOLD);
				display.invertColors();
			}
			else {
				display.printFixed(42 - 8, 16, "NO", STYLE_BOLD);
				display.printFixed(84 - (1.5 * 8) - (1.5 * 8), 16, ">", STYLE_BOLD);
				display.printFixed(84 + (1.5 * 8) + (0.5 * 8), 16, "<", STYLE_BOLD);
				display.invertColors();
				display.printFixed(84 - (1.5 * 8), 16, "YES", STYLE_NORMAL);
				display.invertColors();
			}
			break;

		case DISPLAY_OFF:
			break;

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
		if (upperSwitch.on() ^ SW_UPPER_INVERT) {
			switch (state) {
				case MUSIC_MODE:
					// Do nothing.  We're already in MUSIC MODE ...
					break;

				// If we're in any other state, switch immediately to music mode
				default:
					initialiseState(MUSIC_MODE, millis());
			}
		}
		else {
			// Revert to idling from whatever state we're in
			initialiseState(IDLE, millis());
		}
	}	
}

bool intHeld = false;

void intButtonPress(void* ref) {
	DEBUG_SERIAL.println("INT Pressed");
	
	switch (state) {
		case IDLE:
		case FIRING_STOP:
			initialiseState(FIRING, millis());
			break;

		default:
			break;
	}
}

void intButtonLongPress(void* ref) {
	intHeld = true;

	unsigned long currentMillis = millis();

	if ((state == MUSIC_MODE) && (!bluetoothMode)) {
		setSDTrack(currentMillis, looparound(trackNumber + 1,trackCount));
		displayState = TRACK_DISPLAY;
		drawDisplay(currentMillis);				
	}

	DEBUG_SERIAL.println("INT Held");
}

void intButtonRelease(void* ref) {
	DEBUG_SERIAL.println("INT Released");
	
	if ((state == MUSIC_MODE) && intHeld) {
		intHeld = false;
		return;
	}

	unsigned long currentMillis = millis();

	switch (state) {
		case MUSIC_MODE:
			DEBUG_SERIAL.println("Play/Pause SD Track");
			cmdMessenger.sendCmd(eventPlayPauseSDTrack);
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

	switch (state) {
	case IDLE:
	case FIRING_STOP:
		initialiseState(FIRING, millis());
		break;

	default:
		break;
	}
}

void tipButtonLongPress(void* ref) {
	tipHeld = true;

	unsigned long currentMillis = millis();

	if ((state == MUSIC_MODE) && (!bluetoothMode)) {
		setSDTrack(currentMillis, looparound(trackNumber - 1, trackCount));
		displayState = TRACK_DISPLAY;
		drawDisplay(currentMillis);
	}

	DEBUG_SERIAL.println("Tip Held");
}

void tipButtonRelease(void* ref) {
	DEBUG_SERIAL.println("Tip Released");

	if ((state == MUSIC_MODE) && tipHeld) {
		tipHeld = false;
		return;
	}

	unsigned long currentMillis = millis();

	// If it was a short press, VENT
	if ((state == FIRING) && !tipHeld) {
		initialiseState(VENTING, currentMillis);
		return;
	}

	switch (state) {
	case MUSIC_MODE:
		cmdMessenger.sendCmd(eventStopSDTrack);		
		break;

	case IDLE:
	case FIRING:
	case FIRING_WARN:
		initialiseState(FIRING_STOP, currentMillis);
		break;

	default:
		break;
	}
	
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
		break;

	case TOP_MENU:
		displayState = DISPLAY_OFF;
		selectedIndex = 0;
		break;

	case VOLUME_DISPLAY:
		displayState = VOLUME_CHANGE;
		break;

	case TRACK_DISPLAY:
		displayState = TRACK_SELECT;
		selectedIndex = trackNumber;
		break;

	case VOLUME_CHANGE:
		displayState = TOP_MENU;
		selectedIndex = 0;
		break;

	case TRACK_SELECT:
		displayState = TOP_MENU;
		selectedIndex = 1;
		break;
	
	case LOAD_CONFIG:
		displayState = TOP_MENU;
		selectedIndex = 2;
		break;

	case SAVE_CONFIG:
		displayState = TOP_MENU;
		selectedIndex = 3;
		break;
		
	default:
		break;
	}

	drawDisplay(currentMillis);

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
			DEBUG_SERIAL.print("Selected Top Menu Item: "); DEBUG_SERIAL.println(selectedIndex);
			switch (selectedIndex) {
				case 0:
					selectedIndex = theVol;
					displayState = VOLUME_CHANGE;
					break;

				case 1:
					selectedIndex = trackNumber;
					displayState = TRACK_SELECT;
					break;

				case 2:
					selectedIndex = 0;
					displayState = LOAD_CONFIG;
					break;

				case 3:
					selectedIndex = 0;
					displayState = SAVE_CONFIG;
					break;

				default:
					break;
			}
			break;

		case VOLUME_CHANGE:
			setVolume(currentMillis, selectedIndex);
			selectedIndex = 0;
			displayState = TOP_MENU;			
			break;

		case TRACK_SELECT:
			setSDTrack(currentMillis, selectedIndex);
			selectedIndex = 1;
			displayState = TOP_MENU;
			break;

		case LOAD_CONFIG:
			if (selectedIndex == 1) {
				cmdMessenger.sendCmd(eventLoadConfig);
			}

			selectedIndex = 2;
			displayState = TOP_MENU;
			break;
		case SAVE_CONFIG:
			if (selectedIndex == 1) {
				cmdMessenger.sendCmd(eventWriteConfig);
			}

			selectedIndex = 3;
			displayState = TOP_MENU;
			break;

		case DISPLAY_OFF:
		case VOLUME_DISPLAY:
		case TRACK_DISPLAY:
		default:
			break;
	}

	drawDisplay(currentMillis);
}

void rotaryMove(unsigned long currentMillis, int movement) {
	if (displayState != DISPLAY_OFF) {
		switch (displayState) {
			case TOP_MENU:
				selectedIndex = looparound((selectedIndex + movement) , 4);
				break;

			case VOLUME_CHANGE:
				selectedIndex = constrain(selectedIndex + movement, 0, maxVol);
				setVolume(currentMillis, selectedIndex);
				break;

			case TRACK_SELECT:
				selectedIndex = looparound((selectedIndex + movement) , trackCount);
				break;

			case LOAD_CONFIG:
			case SAVE_CONFIG:
				selectedIndex = looparound((selectedIndex + movement), 2);
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

unsigned long firingStart = 0;

void initialiseState(State newState, unsigned long currentMillis) {
	DEBUG_SERIAL.print("Initialising State: "); DEBUG_SERIAL.println((char) newState);

	switch (newState) {
		case OFF:
			tipLights.clear();
			bodyLights.clear();
			barrelLights.clear();			
			clearBargraph(currentMillis);
			bargraph.write();
			bodyLights.setPixelColor(TOP_FORWARD_INDEX, topForwardBatteryColour);
			bodyLights.show();
			break;

		case BOOTING:			
			tipLights.clear();
			bodyLights.clear();
			barrelLights.clear();
			clearBargraph(currentMillis);
			bargraph.write();
			bargraphLitCells = 0;
			bargraphClimbing = true;
			break;

		case IDLE:
			if (overheatMode) {
				tipLights.setPixelColor(0, tipColour);
				lastTipChange = currentMillis;
			}
			// Vent light to max, top forward light on (mid brightness), slo-blo to red
			bodyLights.setPixelColor(VENT_INDEX, ventColour);
			bodyLights.setPixelColor(TOP_FORWARD_INDEX, topForwardColour);
			bodyLights.setPixelColor(SLO_BLO_INDEX, sloBloColour);
			lastBodyChange[VENT_INDEX] = currentMillis;
			lastBodyChange[TOP_FORWARD_INDEX] = currentMillis;
			lastBodyChange[SLO_BLO_INDEX] = currentMillis;

			clearBargraph(currentMillis);
			bargraph.write();
			bargraphLitCells = 0;
			bargraphClimbing = true;
			bargraphPeriod = bargraphIdlePeriod;
			break;

		case POWERDOWN:
			savedTipColour = tipLights.getPixelColor(0);
			for (int i = 0; i < BODY_LED_COUNT; i++) {
				savedBodyColours[i] = bodyLights.getPixelColor(i);
			}

			barrelLights.clear();
			
			bargraphClimbing = false;
			
			bargraphLitCells = bargraphLEDs;
			while (bargraphLitCells > 0) {
				if (bargraphCache[bargraphLitCells - 1] == 1) {
					break;
				}
				bargraphLitCells--;
			}
			bargraphPeriod = 0.25 * POWERDOWN_TIME;
			break;

		case FIRING:
			randomSeed(currentMillis);
			bargraphLitCells = ((float)bargraphLEDs / 2) + 1;
			bargraphClimbing = true;
			firingStart = currentMillis;
			break;

		case FIRING_WARN:
			// Bargraph should continue from previous
			// Set rear top to orange
			bodyLights.setPixelColor(TOP_BACK_INDEX, topBackColour);
			lastBodyChange[TOP_BACK_INDEX] = currentMillis;
			break;

		case VENTING:
			tipLights.clear();
			bodyLights.clear();
			barrelLights.clear();

			setBGLampRange(currentMillis, 0, bargraphLEDs);
			bargraph.write();
			break;

		case FIRING_STOP:
			bodyLights.setPixelColor(TOP_FORWARD_INDEX, topForwardColour);
			bodyLights.setPixelColor(FRONT_INDEX, ledOff);
			bodyLights.setPixelColor(TOP_BACK_INDEX, ledOff);
			lastBodyChange[TOP_FORWARD_INDEX] = currentMillis;
			lastBodyChange[FRONT_INDEX] = currentMillis;
			lastBodyChange[TOP_BACK_INDEX] = currentMillis;
			barrelLights.clear();
			// Bargraph should continue from previous
			bargraphPeriod = bargraphFiringStopPeriod;
			break;

		case MUSIC_MODE:
			tipLights.clear();
			bodyLights.clear();
			barrelLights.clear();
			clearBargraph(currentMillis);
			bargraph.write();

			// Update the track number on the Pack.  Note this is really just used in testing
			// When I have been starting each component out of sync
			setSDTrack(currentMillis, trackNumber);
			break;

		default:
			break;
	}

	//DEBUG_SERIAL.println("Updating LEDs");

	tipLights.show();
	bodyLights.show();
	barrelLights.show();

	//DEBUG_SERIAL.println("Updating Pack State");

	cmdMessenger.sendCmdStart(eventChangeState);
	cmdMessenger.sendCmdArg(newState);
	cmdMessenger.sendCmdEnd();

	stateLastChanged = millis();

	state = newState;
}

void stateUpdate() {
	stateUpdate(millis());
}

unsigned long lastStateUpdate = 0;

void stateUpdate(unsigned long currentMillis) {
	switch (state) {
		case OFF:
			break;

		case BOOTING:
			if (currentMillis > stateLastChanged + BOOTING_TIME) {
				initialiseState(IDLE, currentMillis);
			}
			break;

		case IDLE:
			break;

		case POWERDOWN:
			if (currentMillis > stateLastChanged + POWERDOWN_TIME) {
				initialiseState(OFF, currentMillis);
			}
			break;

		case FIRING:
			if (overheatMode &&  (currentMillis > stateLastChanged + OVERHEAT_TIME)) {
				initialiseState(FIRING_WARN, currentMillis);
			}
			bargraphPeriod = max(bargraphFiringMinPeriod, bargraphFiringStartPeriod - (bargraphFiringAcceleration * (currentMillis - firingStart)));
			break;

		case FIRING_WARN:
			if (currentMillis > stateLastChanged + FIRING_WARN_TIME) {
				initialiseState(VENTING, currentMillis);
			}

			bargraphPeriod = max(bargraphFiringMinPeriod, bargraphFiringStartPeriod - (bargraphFiringAcceleration * (currentMillis - firingStart)));
			break;

		case VENTING:
			if (currentMillis > stateLastChanged + VENT_TIME) {
				initialiseState(BOOTING, currentMillis);
			}
			break;

		case FIRING_STOP:
			if (currentMillis > stateLastChanged + FIRING_STOP_TIME) {
				initialiseState(IDLE, currentMillis);
			}
			break;

		case MUSIC_MODE:
			break;

		default:
			break;
	}

	lastStateUpdate = currentMillis;
}

void setSDTrack(unsigned long currentMillis, int newTrackNumber) {
	trackNumber = newTrackNumber;

	// Let the pack handle whether or not to change an in-flight track and update isPlaying if need be (shouldn't be
	// necessary)
	cmdMessenger.sendCmdStart(eventSetSDTrack);
	cmdMessenger.sendCmdArg(trackNumber);
	cmdMessenger.sendCmdEnd();

}

void setVolume(unsigned long currentMillis, int newVolume) {
	theVol = newVolume;

	cmdMessenger.sendCmdStart(eventSetVolume);
	cmdMessenger.sendCmdArg(theVol);
	cmdMessenger.sendCmdEnd();
}


/*  ----------------------
	Serial Command Handling
----------------------- */
void attachCmdMessengerCallbacks() {
	cmdMessenger.attach(onUnknownCommand);
	cmdMessenger.attach(eventPackIonSwitch, onPackIonSwitch);
	cmdMessenger.attach(eventPackEncoderButton, onPackEncoderButton);
	cmdMessenger.attach(eventPackEncoderTurn, onPackEncoderTurn);
	cmdMessenger.attach(eventSetVolume, onPackSetVolume);
	cmdMessenger.attach(eventSetSDTrack, onPackSetTrack);

	cmdMessenger.attach(eventUpdateMusicPlayingState, onUpdateMusicPlayingState);
	cmdMessenger.attach(eventPackConnect, onPackConnect);
}

void onUnknownCommand()
{
	Serial.print("Command without attached callback: "); Serial.println(cmdMessenger.commandID());
}

// Handle these events in wand
//eventPackIonSwitch,
//eventPackEncoderButton,
//eventPackEncoderTurn,
//eventSetVolume,
//eventSetSDTrack,
//eventUpdateMusicPlayingState,

void onPackIonSwitch() {
	bool toggle = cmdMessenger.readBoolArg();
	DEBUG_SERIAL.print("Pack Ion Switch Toggled: "); DEBUG_SERIAL.println(toggle ? "ON" : "OFF");
}

void onPackEncoderButton() {
	DEBUG_SERIAL.println("Pack Encoder Pressed");
	ButtonEvent event = (ButtonEvent)cmdMessenger.readInt16Arg();

	switch (event) {
		case PRESSED:
			rotaryButtonPress(NULL);
			break;

		case HELD:
			rotaryButtonLongPress(NULL);
			break;

		case RELEASED:
			rotaryButtonRelease(NULL);
			break;

		default:
			break;
	}
}

void onPackEncoderTurn() {
	DEBUG_SERIAL.println("Pack Encoder Turn");
	int movement = cmdMessenger.readInt32Arg();
	rotaryMove(millis(), movement);
}

void onPackSetTrack() {
	DEBUG_SERIAL.println("Pack Set Track");
	int newTrack = cmdMessenger.readInt16Arg();
	DEBUG_SERIAL.printf("Pack Requested Track Change To: %s\n", trackList[newTrack][1]);
	setSDTrack(millis(), newTrack);
}

void onPackSetVolume() {
	unsigned long currentMillis = millis();
	
	DEBUG_SERIAL.println("Pack Set Volume");
	int newVol = cmdMessenger.readInt16Arg();
	DEBUG_SERIAL.printf("Pack Requested Volume Change To: %i\n", newVol);
	displayState = VOLUME_DISPLAY;
	selectedIndex = newVol;
	lastDisplayUpdate = currentMillis;
	drawDisplay(currentMillis);
	setVolume(currentMillis, newVol);
}

void onUpdateMusicPlayingState() {
	DEBUG_SERIAL.println("Pack Music State Update");
	bool playingState = cmdMessenger.readBoolArg();
	musicPlaying = playingState;
}

void onPackConnect() {
	DEBUG_SERIAL.println("Pack Connection to Wand");
	packConnected = true;
	displayState = DISPLAY_OFF;
	drawDisplay(millis());
}


/*  ----------------------
	Body LED Management
----------------------- */
void bodyInit(unsigned long currentMillis) {
	for (int i = 0; i < BODY_LED_COUNT; i++) {
		lastBodyChange[i] = currentMillis;
	}
}

void bodyUpdate() {
	unsigned long currentMillis = millis();

	switch (state) {
	case OFF:
		break;

	case BOOTING:{
			float lightIntensity = (((float)(max(stateLastChanged, currentMillis) - stateLastChanged)) / BOOTING_TIME);
			bodyLights.setPixelColor(VENT_INDEX, Adafruit_NeoPixel::gamma32(colourMultiply(ventColour, lightIntensity)));
			bodyLights.setPixelColor(SLO_BLO_INDEX, Adafruit_NeoPixel::gamma32(colourMultiply(sloBloColour, lightIntensity)));
			lastBodyChange[VENT_INDEX] = currentMillis;
			lastBodyChange[SLO_BLO_INDEX] = currentMillis;

			bodyLights.show();
		}
		break;

	case IDLE:
		break;

	case POWERDOWN: {
			unsigned long timeSinceShutdown = max(currentMillis, stateLastChanged) - stateLastChanged;
			for (int i = 0; i < BODY_LED_COUNT; i++) {
				bodyLights.setPixelColor(i,
					Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(
						max(0, Red(savedBodyColours[i]) - ((int)(powerDownLEDDecayRate * timeSinceShutdown))),
						max(0, Green(savedBodyColours[i]) - ((int)(powerDownLEDDecayRate * timeSinceShutdown))),
						max(0, Blue(savedBodyColours[i]) - ((int)(powerDownLEDDecayRate * timeSinceShutdown))),
						max(0, White(savedBodyColours[i]) - ((int)(powerDownLEDDecayRate * timeSinceShutdown)))
					))
				);
				lastBodyChange[i] = currentMillis;
			}
			bodyLights.show();
		}
		break;

	case FIRING_WARN: {
		unsigned long timeSinceFiringStart = max(currentMillis, stateLastChanged) - stateLastChanged;
		bool warnLEDLit = (timeSinceFiringStart % (2 * warningBlinkMS)) < warningBlinkMS;
		bodyLights.setPixelColor(TOP_BACK_INDEX, warnLEDLit ? topBackColour : ledOff);
		savedBodyColours[TOP_BACK_INDEX] = warnLEDLit ? topBackColour : ledOff;
		uint32_t thisColour = random(0, 100) < ventFlickerPercentage ? ventFlickerColour : ventColour;
		savedBodyColours[VENT_INDEX] = thisColour;
		bodyLights.setPixelColor(VENT_INDEX, thisColour);
		lastBodyChange[TOP_BACK_INDEX] = currentMillis;
		lastBodyChange[VENT_INDEX] = currentMillis;
	}
	case FIRING: {
		unsigned long timeSinceFiringStart = max(currentMillis, stateLastChanged) - stateLastChanged;
		bool topLEDLit = (timeSinceFiringStart % (2 * firingBlinkMS)) < firingBlinkMS;
		bodyLights.setPixelColor(TOP_FORWARD_INDEX, topLEDLit ? topForwardColour : ledOff);
		bodyLights.setPixelColor(FRONT_INDEX, topLEDLit ? ledOff : frontColour);
		lastBodyChange[TOP_FORWARD_INDEX] = currentMillis;
		lastBodyChange[FRONT_INDEX] = currentMillis;
		bodyLights.show();
	}
		break;

	case VENTING:
		uint32_t thisColour = random(0, 100) < 2 * ventFlickerPercentage ? ventFlickerColour : ventColour;
		savedBodyColours[VENT_INDEX] = thisColour;
		lastBodyChange[VENT_INDEX] = currentMillis;
		bodyLights.setPixelColor(VENT_INDEX, thisColour);
		bodyLights.show();
		break;

	case FIRING_STOP:
		break;

	case MUSIC_MODE:
		break;

	default:
		break;
	}
}

/*  ----------------------
	Barrel LED Management
----------------------- */
void barrelInit(unsigned long currentMillis) {
	lastBarrelChange = currentMillis;
}

void barrelUpdate() {
	switch (state) {
	case OFF:
		break;

	case VENTING:
	case FIRING_STOP:
	case MUSIC_MODE:
	case BOOTING:
	case IDLE:
	case POWERDOWN:			  
		break;

	case FIRING:
	case FIRING_WARN: {
			int offset = random(numFiringLEDColours);
			for (int i = 0; i < BARREL_LED_COUNT; i++) {
				barrelLights.setPixelColor(i, firingLEDColours[(i + offset) % numFiringLEDColours]);
			}
			barrelLights.show();
		}
		break;

	default:
		break;
	}
}

/*  ----------------------
	Tip LED Management
----------------------- */
void tipInit(unsigned long currentMillis) {

}

void tipUpdate() {
	unsigned long currentMillis = millis();

	switch (state) {
		case OFF:
			break;

		case BOOTING:
			if (overheatMode) {
				float lightIntensity = (((float)(max(stateLastChanged, currentMillis) - stateLastChanged)) / BOOTING_TIME);
				tipLights.setPixelColor(0, Adafruit_NeoPixel::gamma32(colourMultiply(tipColour, lightIntensity)));
				lastTipChange = currentMillis;
				tipLights.show();
			}
			break;

		case POWERDOWN: {
				unsigned long timeSinceShutdown = max(currentMillis, stateLastChanged) - stateLastChanged;
				tipLights.setPixelColor(0,
					Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(
							max(0, Red(savedTipColour) - ((int)(powerDownLEDDecayRate * timeSinceShutdown))),
							max(0, Green(savedTipColour) - ((int)(powerDownLEDDecayRate * timeSinceShutdown))),
							max(0, Blue(savedTipColour) - ((int)(powerDownLEDDecayRate * timeSinceShutdown))),
							max(0, White(savedTipColour) - ((int)(powerDownLEDDecayRate * timeSinceShutdown)))
						))
					);
				lastTipChange = currentMillis;				
				tipLights.show();
			}
			break;

		case IDLE:
		case FIRING_STOP:
		case FIRING_WARN:
		case FIRING:
			if (overheatMode) {
				tipLights.setPixelColor(0, tipColour);
				lastTipChange = currentMillis;
				tipLights.show();
			}
			else {
				tipLights.setPixelColor(0, ledOff);
				lastTipChange = currentMillis;
				tipLights.show();
			}
			break;

		case VENTING:
			break;

		case MUSIC_MODE:
			break;

		default:
			break;
	}
}

/*  ----------------------
	Bar Graph Management
----------------------- */
void graphInit(unsigned long currentMillis) {
	bargraph.init(0x70);
	bargraph.setBrightness(10);
	bargraphLitCells = 0;

	clearBargraph(currentMillis);
	bargraph.write();
}

void graphUpdate(unsigned long currentMillis) {
	switch (state) {
		case OFF:
		case MUSIC_MODE:
			break;

		case BOOTING: {
				int newLitCells = round(2 * (((float)max(stateLastChanged, currentMillis) - stateLastChanged) / BOOTING_TIME) * bargraphLEDs);
				bargraphLitCells = max(0, newLitCells < bargraphLEDs ? newLitCells : (2 * bargraphLEDs) - newLitCells);
				clearBargraph(currentMillis);
				if (bargraphLitCells > 0) {
					setBGLampRange(currentMillis, 0, bargraphLitCells);
					bargraph.write();
				}				
			}
			break;

		case IDLE:
			if ((currentMillis - lastBargraphChange) > (((float)bargraphPeriod) / bargraphLEDs)) {
                if ((bargraphClimbing && (bargraphLitCells == bargraphLEDs)) || (!bargraphClimbing && (bargraphLitCells == 0))) {
					bargraphClimbing = !bargraphClimbing;
                } 
				bargraphLitCells = bargraphLitCells + (bargraphClimbing ? 1 : -1);

				clearBargraph(currentMillis);
				setBGLampRange(currentMillis, 0, bargraphLitCells);
				bargraph.write();
				lastBargraphChange = currentMillis;
            }
			break;

		case POWERDOWN:
			// Powerdown sequence will wipe from the highest lamp downwards, regardless of current animation
			if ((currentMillis - lastBargraphChange) > ((float)bargraphPeriod / bargraphLEDs)) {
				if (bargraphLitCells > 0) {
					setBGLampRange(currentMillis, bargraphLitCells, bargraphLEDs, false);
					bargraphLitCells--;
				}
				else {
					clearBargraph(currentMillis);
				}
				bargraph.write();
			}
			break;

		case FIRING_STOP:
			// Terminate the animation during a ceasefire on the final climb
			if (bargraphClimbing && (bargraphLitCells == (bargraphLEDs - bargraphFiringWidth + 1))) {
				clearBargraph(currentMillis);
				bargraph.write();
				break;
			}
		case FIRING:
		case FIRING_WARN:
			if ((currentMillis - lastBargraphChange) > ((float)bargraphPeriod / ((0.5 * (float)bargraphLEDs) - bargraphFiringWidth + 1))) {
				if ((bargraphClimbing && (bargraphLitCells == (bargraphLEDs - bargraphFiringWidth + 1))) || 
					(!bargraphClimbing && (bargraphLitCells == 1 + (float)bargraphLEDs/2))) {
					bargraphClimbing = !bargraphClimbing;
				}

				bargraphLitCells = bargraphLitCells + (bargraphClimbing ? 1 : -1);

				clearBargraph(currentMillis);
				setBGLampRange(currentMillis, bargraphLitCells, bargraphLitCells + bargraphFiringWidth - 1);
				setBGLampRange(currentMillis, (bargraphLEDs - (bargraphLitCells + 1)), (bargraphLEDs - (bargraphLitCells + 1) + (bargraphFiringWidth - 1)));
				bargraph.write();
				lastBargraphChange = currentMillis;
			}
			break;

		// Clears a random scattering of LEDs 10 times over the course of the venting
		case VENTING:			
			if ((max(stateLastChanged,currentMillis) - stateLastChanged) < VENT_LIGHT_FADE_TIME) {

				if ((currentMillis - lastBargraphChange) > ((float)VENT_LIGHT_FADE_TIME / 10)) {
					for (int i = 0; i < ((float)bargraphLEDs / random(4,10)); i++) {
						setBGLamp(currentMillis, random(1, bargraphLEDs + 1), false);
					}
					bargraph.write();
					lastBargraphChange = currentMillis;
				}
			}
			else {
				clearBargraph(currentMillis);
				bargraph.write();
			}
			break;

		default:
			break;
	}
}