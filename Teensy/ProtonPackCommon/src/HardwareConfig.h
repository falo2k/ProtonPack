#pragma once

/*------------------------------------------
--
-- Hardware configuration variables for wand and pack
-- Review any merges to this file when updating the codebase
-- in order to ensure your configuration remains consistent
-- with your hardware setup (e.g. pin settings).
--
-- The git version should align with the latest PCB versions.
--
-------------------------------------------*/

/*============== WAND ==============*/
/*  ----------------------
	Variables to flip switch directions depending on
	your wiring implementation
----------------------- */
// Use this to flip the rotary switch direction in code
int rotaryDirectionWand = -1;
const bool SW_WAND_ACT_INVERT = false;
const bool SW_WAND_LO_INVERT = false;
const bool SW_WAND_UP_INVERT = false;

/*  ----------------------
	Pins
----------------------- */
const int BARREL_LED_COUNT = 7;
const int TIP_LED_COUNT = 1;
const int BODY_LED_COUNT = 5;
const int VENT_INDEX = 1;
const int FRONT_INDEX = 0;
const int TOP_FORWARD_INDEX = 2;
const int TOP_BACK_INDEX = 3;
const int SLO_BLO_INDEX = 4;
// Switches
const int SW_WAND_ACT_PIN = A3;
const int BTN_WAND_INT_PIN = A2;
const int SW_WAND_LO_PIN = A0;
const int SW_WAND_UP_PIN = A6;
const int BTN_WAND_TIP_PIN = 2;
const int WAND_ROT_BTN_PIN = 3;
const int WAND_ROT_A_PIN = 5;
const int WAND_ROT_B_PIN = 6;

// LEDS
const int WAND_BARREL_LED_PIN = 22;// A8;
const int WAND_TIP_LED_PIN = 23; // A9;
const int WAND_BODY_LED_PIN = 21; // A7;

/*  ----------------------
	LED Lengths
----------------------- */
const int BARREL_LED_COUNT = 7;
const int TIP_LED_COUNT = 1;
const int BODY_LED_COUNT = 5;
const int VENT_INDEX = 1;
const int FRONT_INDEX = 0;
const int TOP_FORWARD_INDEX = 2;
const int TOP_BACK_INDEX = 3;
const int SLO_BLO_INDEX = 4;

/*----- PACK -----*/