// Includes
#include <Adafruit_NeoPixel.h>
#include <GBLEDPatternsJewel.h>

#include <Adafruit_Soundboard.h>
#include <SoftwareSerial.h>
#include <BGSequence.h>

#define DEBUG
//#define SFX
//#define BARGRAPH

/*  ----------------------
Pin & Range Definitions
: Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int SW_ACTIVATE_PIN = 4;
const int SW_INTENSIFY_PIN = 5;
const int SW_LOWER_PIN = 6;
const int SW_UPPER_PIN = 7;
const int SW_TIP_PIN = 8;

const int SW_BT_PIN = A0;
const int SW_ION_PIN = A3;

const int SLD_A_PIN = A7;
const int SLD_B_PIN = A6;

// LEDs
const int PACK_LED_PIN = 2;
const int POWER_CELL_RANGE[2] = { 0, 16 };
const int CYCLOTRON_RANGES[4][2] = { {17,23},{24,30},{31,37},{38,44} };

const int VENT_LED_PIN = 3;
// TODO: Correct VENT LED Range
const int VENT_LED_RANGE[2] = { 0,4 };
const int WAND_LED_PIN = 3;
const int SLO_BLO_INDEX = 5;
// TODO: Remaining light indexes for wand



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
bool SW_INTENSIFY = false;
bool SW_LOWER = false;
bool SW_UPPER = false;
bool SW_TIP = false;
bool SW_BT = false;
bool SW_ION = false;
int SLD_A = 0;
int SLD_B = 0;


void setup() {
	#ifdef DEBUG
		// Set up Serial Logging
	Serial.begin(9600);
	#endif

	// Set up input switches



	#ifdef SFX
	// Set up Audio FX Serial

	#endif

	#ifdef BARGRAPH
	// Set up Bargraph Serial
	#endif

}

void loop() {
	// put your main code here, to run repeatedly:

}
