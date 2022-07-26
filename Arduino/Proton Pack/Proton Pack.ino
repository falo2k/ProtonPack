// Includes
#include <Adafruit_NeoPixel.h>
#include <GBLEDPatternsJewel.h>

#include <Adafruit_Soundboard.h>
#include <SoftwareSerial.h>
#include <BGSequence.h>

#define DEBUG true
#define DEBUG_SERIAL if(DEBUG)Serial
//#define SFX
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
const int SLD_LOW = 300;
const int SLD_MID = 600;

// LEDs
const int PACK_LED_PIN = 2;
const int POWER_CELL_RANGE[2] = { 0, 16 };
const int CYCLOTRON_RANGES[4][2] = { {17,23},{24,30},{31,37},{38,44} };

const int VENT_LED_PIN = 3;
// TODO Correct VENT LED Range
const int VENT_LED_RANGE[2] = { 0,4 };
const int WAND_LED_PIN = 3;
const int SLO_BLO_INDEX = 5;
// TODO Remaining light indexes for wand

// Audio FX Board
const int SFX_TX = 10;
const int SFX_RX = 11;
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


void setup() {
	// Set up debug logging
	DEBUG_SERIAL.begin(115200);

	// Input switches - Slider Pins Automatically Analogue Read
	pinMode(SW_ACTIVATE_PIN, INPUT_PULLUP);
	pinMode(BTN_INTENSIFY_PIN, INPUT_PULLUP);
	pinMode(SW_LOWER_PIN, INPUT_PULLUP);
	pinMode(SW_UPPER_PIN, INPUT_PULLUP);
	pinMode(BTN_TIP_PIN, INPUT_PULLUP);

	pinMode(SW_BT_PIN, INPUT_PULLUP);
	pinMode(SW_ION_PIN, INPUT_PULLUP);

	// Relay outputs
	pinMode(VENT_RELAY, OUTPUT);
	pinMode(BT_RELAY1, OUTPUT);
	pinMode(BT_RELAY2, OUTPUT);

	#ifdef SFX
	// Set up Audio FX Serial

	#endif

	#ifdef BARGRAPH
	// Set up Bargraph Serial
	#endif

}

void loop() {
	// put your main code here, to run repeatedly:
	if (checkInputs()) {
		char strBuf[128];
		sprintf(strBuf, "ACTIVATE=%i, INTENSIFY=%i, LOWER=%i, UPPER=%i, TIP=%i, BT=%i, ION=%i, SLDA=%i, SLDB=%i",
			SW_ACTIVATE, BTN_INTENSIFY, SW_LOWER, SW_UPPER, BTN_TIP, SW_BT, SW_ION, SLD_A, SLD_B);
		DEBUG_SERIAL.println(strBuf);
	}

	delay(100);
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

	SW_ACTIVATE = digitalRead(SW_ACTIVATE_PIN) == SW_ACTIVATE_ON;
	BTN_INTENSIFY = digitalRead(BTN_INTENSIFY_PIN) == BTN_INTENSIFY_ON;
	SW_LOWER = digitalRead(SW_LOWER_PIN) == SW_LOWER_ON;
	SW_UPPER = digitalRead(SW_UPPER_PIN) == SW_UPPER_ON;
	BTN_TIP = digitalRead(BTN_TIP_PIN) == BTN_TIP_ON;

	SW_BT = digitalRead(SW_BT_PIN) == SW_BT_ON;
	SW_ION = digitalRead(SW_ION_PIN) == SW_ION_ON;

	// TODO Slider reading

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

	return initialState != newState;
}