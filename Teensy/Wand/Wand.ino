/*
 Name:		Wand.ino
 Created:	1/13/2023 5:10:39 PM
 Author:	oliwr
*/

#include <Sprite16.h>
#include "BGSequence.h"
#include <Adafruit_NeoPixel.h>
#include <ProtonPackCommon.h>
BGSequence BarGraph;

// **** Different Bargraph sequence modes **** //
enum BarGraphSequences { START, ACTIVE, FIRE1, FIRE2, BGVENT };
BarGraphSequences BGMODE;


#define BARREL_LED_PIN 11
#define BARREL_LED_COUNT 7
#define TIP_LED_PIN 12
#define TIP_LED_COUNT 1
#define BODY_LED_PIN 10
#define BODY_LED_COUNT 5
#define SLO_BOW_INDEX 0
#define VENT_INDEX 1
#define FRONT_INDEX 2
#define TOP1_INDEX 3
#define TOP2_INDEX 4

Adafruit_NeoPixel barrelLights = Adafruit_NeoPixel(BARREL_LED_COUNT, BARREL_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel tipLights = Adafruit_NeoPixel(TIP_LED_COUNT, TIP_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel bodyLights = Adafruit_NeoPixel(BODY_LED_COUNT, BODY_LED_PIN, NEO_GRBW + NEO_KHZ800);

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600);
	//while (!Serial) {
	//	delay(10);
	//}

	// Initiate and clear LEDs
	barrelLights.begin();
	barrelLights.clear();
	barrelLights.show();
	tipLights.begin();
	tipLights.clear();
	tipLights.show();
	bodyLights.begin();
	bodyLights.clear();
	bodyLights.show();

}

unsigned long CuurentBGInterval = BarGraph.IntervalBG;
bool first = true;


// the loop function runs over and over again until power down or reset
void loop() {
	unsigned long currentMillis = millis();

	update_leds(currentMillis);
}

unsigned long lastLEDUpdate = 0;
unsigned long interval = 500;
int currentBarrel = BARREL_LED_COUNT;
int currentBody = BODY_LED_COUNT;
int currentTip = TIP_LED_COUNT;

void update_leds(unsigned long currentMillis) {

	if (currentMillis - lastLEDUpdate > interval) {
		currentBarrel = (currentBarrel + 1) % (BARREL_LED_COUNT + 1);
		currentBody = (currentBody + 1) % (BODY_LED_COUNT + 1);
		currentTip = (currentTip + 1) % (TIP_LED_COUNT + 1);

		barrelLights.clear();
		bodyLights.clear();
		tipLights.clear();

		for (int i = 0; i <= BARREL_LED_COUNT; i++) {
			if (i == currentBarrel && i < BARREL_LED_COUNT)	barrelLights.setPixelColor(i, barrelLights.Color(25, 0, 0, 0));
		}

		for (int i = 0; i <= BODY_LED_COUNT; i++) {
			if (i == currentBody && i < BODY_LED_COUNT)	bodyLights.setPixelColor(i, bodyLights.Color(0, 0, 25, 0));
		}

		for (int i = 0; i <= TIP_LED_COUNT; i++) {
			if (i == currentTip && i < TIP_LED_COUNT)	tipLights.setPixelColor(i, tipLights.Color(0, 25, 0, 0));
		}

		lastLEDUpdate = currentMillis;

		barrelLights.show();
		tipLights.show();
		bodyLights.show();
	}

}