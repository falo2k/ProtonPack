// Simple WAV file player example
//
// Three types of output may be used, by configuring the code below.
//
//   1: Digital I2S - Normally used with the audio shield:
//         http://www.pjrc.com/store/teensy3_audio.html
//
//   2: Digital S/PDIF - Connect pin 22 to a S/PDIF transmitter
//         https://www.oshpark.com/shared_projects/KcDBKHta
//
//   3: Analog DAC - Connect the DAC pin to an amplified speaker
//         http://www.pjrc.com/teensy/gui/?info=AudioOutputAnalog
//
// To configure the output type, first uncomment one of the three
// output objects.  If not using the audio shield, comment out
// the sgtl5000_1 lines in setup(), so it does not wait forever
// trying to configure the SGTL5000 codec chip.
//
// The SD card may connect to different pins, depending on the
// hardware you are using.  Uncomment or configure the SD card
// pins to match your hardware.
//
// Data files to put on your SD card can be downloaded here:
//   http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ProtonPackCommon.h>
#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>

#include <CmdMessenger.h>

AudioPlaySdWav           playWav1;
// Use one of these 3 output types: Digital I2S, Digital S/PDIF, or Analog DAC
AudioOutputI2S           audioOutput;
//AudioOutputSPDIF       audioOutput;
//AudioOutputAnalog      audioOutput;
//On Teensy LC, use this for the Teensy Audio Shield:
//AudioOutputI2Sslave    audioOutput;

AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

//const char *tracks[] = {"MUSIC/THEME.WAV", "MUSIC/CLEANIN.WAV" ,"MUSIC/SAVIN.WAV" ,"MUSIC/MAGICCUT.WAV", "MUSIC/RGBTHEME.WAV", "MUSIC/TITLE.WAV", "MUSIC/ONOUROWN.WAV" };
//int trackCount = 7;

const int PCELL_LED_PIN = 14;
const int CYCLO_LED_PIN = 16;
const int VENT_LED_PIN = 17;

const int POWER_CELL_LEDS = 15;
const int CYCLOTRON_LEDS = 4;
const int PACK_LED_COUNT = POWER_CELL_LEDS + CYCLOTRON_LEDS;
const int POWER_CELL_RANGE[2] = { 0, POWER_CELL_LEDS - 1 };
// Note that the last index is the central LED
const int CYCLOTRON_RANGES[4][2] = { {POWER_CELL_LEDS ,POWER_CELL_LEDS + 6},{POWER_CELL_LEDS + 7,POWER_CELL_LEDS + 13},{POWER_CELL_LEDS + 14,POWER_CELL_LEDS + 20},{POWER_CELL_LEDS + 21,POWER_CELL_LEDS + 27} };
const int VENT_LEDS = 1;
const int WAND_LEDS = 0;
const int VENT_LED_LENGTH = VENT_LEDS + WAND_LEDS;
// TODO Correct VENT LED Range
const int VENT_LED_RANGE[2] = { 0,VENT_LEDS - 1 };
const int SLO_BLO_INDEX = VENT_LEDS;

Adafruit_NeoPixel pcellLights = Adafruit_NeoPixel(POWER_CELL_LEDS, PCELL_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel cycloLights = Adafruit_NeoPixel(CYCLOTRON_LEDS, CYCLO_LED_PIN, NEO_RGB + NEO_KHZ800);

CmdMessenger cmdMessenger = CmdMessenger(Serial1);

bool SW_UPPER = true;

const int ION_SWITCH_PIN = 22;
const int ROT_SW_PIN = 9;
const int ROT_ENC_A_PIN = 2;
const int ROT_ENC_A_PIN = 4;

const int IO_1_PIN = 5;
const int IO_2_PIN = 3;


// 0x4B is the default i2c address
#define MAX9744_I2CADDR 0x4B

// We'll track the volume level in this variable.
int8_t thevol = 31;

bool musicMode = false;

void setup() {
    Serial.begin(9600);

    Serial1.begin(9600);

    Wire.begin();

    if (!setvolume(thevol)) {
        Serial.println("Failed to set volume, MAX9744 not found!");
        // Should loop here until ready - or loop for a small amount of time?
        // // Error message over serial to display?
        //while (1);
    }
    
    // Read config file from 

    randomSeed(analogRead(A8));

    // Audio connections require memory to work.  For more
    // detailed information, see the MemoryAndCpuUsage example
    AudioMemory(8);

    // Comment these out if not using the audio adaptor board.
    // This may wait forever if the SDA & SCL pins lack
    // pullup resistors
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);

    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
    if (!(SD.begin(SDCARD_CS_PIN))) {
        // stop here, but print a message repetitively
        while (1) {
            Serial.println("Unable to access the SD card");
            delay(500);
        }
    }

    pcellLights.begin();
    cycloLights.begin();
    pcellLights.clear();
    cycloLights.clear();
    pcellLights.show();
    cycloLights.show();

    for (int i = 0; i <= POWER_CELL_LEDS; i++) {
        pcellLights.setPixelColor(i, pcellLights.Color(255, 255, 255));
    }
    for (int i = 0; i <= CYCLOTRON_LEDS; i++) {
        cycloLights.setPixelColor(i, pcellLights.Color(255, 255, 255));
    }
    pcellLights.show();
    cycloLights.show();

    Serial.println("Setting up Serial port");

    cmdMessenger.printLfCr();
    attachCmdMessengerCallbacks();
    Serial.println("Setup Complete");
}

void attachCmdMessengerCallbacks() {
    cmdMessenger.attach(onUnknownCommand);
    cmdMessenger.attach(eventIntPressed, onButtonPress);
    cmdMessenger.attach(eventActToggle, onActSwitch);
    cmdMessenger.attach(eventButtonChange, onSwitchChange);
}

void loop() {
    unsigned long currentMillis = millis();

    //while (!playWav1.isPlaying()) {
    //    playRandomTrack();
    //}
    /*if (SW_UPPER) {
        updateLeds(currentMillis);
    }*/

    //Serial.println("Checking Serial");
    cmdMessenger.feedinSerialData();
}

/*  ----------------------
    LED Functions
----------------------- */
unsigned long lastLEDCycle = 0;
unsigned long ledCycleInterval = 10000;
unsigned long lastLEDUpdate = 0;
unsigned long ledUpdateRate = 100;


void updateLeds(unsigned long currentMillis) {

    if (currentMillis - lastLEDUpdate > ledUpdateRate) {
        pcellLights.clear();

        float rotation = 2.0 * M_PI * (((float)currentMillis - lastLEDCycle) / ledCycleInterval);

        int sinVal = ((int)floor(((1.0 + sin(rotation)) / 2) * 255));
        int cosVal = ((int)floor(((1.0 + cos(rotation)) / 2) * 255));

        for (int i = 0; i <= POWER_CELL_LEDS; i++) {
            pcellLights.setPixelColor(i, pcellLights.Color(sinVal, cosVal, sinVal));
        }

        pcellLights.show();
    }

    if (currentMillis - lastLEDCycle > ledCycleInterval) {
        lastLEDCycle = currentMillis;
    }    
}

void playRandomTrack() {
    if (!musicMode) return;

    if (playWav1.isPlaying()) {
        playWav1.stop();
    }

    playWav1.play(trackList[random(0, 6)][0]);
    delay(100);
}

void onButtonPress() {
    bool buttonState = cmdMessenger.readBoolArg();

    Serial.print("Received message: "); Serial.println(buttonState);
    

    if (buttonState) {
        playRandomTrack();
    }
}

void onActSwitch() {
    bool buttonState = cmdMessenger.readBoolArg();

    Serial.print("Received message: "); Serial.println(buttonState);


    if (buttonState) {
        musicMode = true;
        playRandomTrack();
    }
    else {
        musicMode = false;
        if (playWav1.isPlaying()) {
            playWav1.stop();
        }
    }
}

void onSwitchChange() {
    Button button = (Button) cmdMessenger.readInt16Arg();
    bool value = cmdMessenger.readBoolArg();

    switch (button) {
    case UPPER:
        SW_UPPER = value;
        if (SW_UPPER) {
            pcellLights.clear();
            cycloLights.clear();
            pcellLights.show();
            cycloLights.show();
        }
        else {
            for (int i = 0; i <= POWER_CELL_LEDS; i++) {
                pcellLights.setPixelColor(i, pcellLights.Color(255, 255, 255));
            }
            for (int i = 0; i <= CYCLOTRON_LEDS; i++) {
                cycloLights.setPixelColor(i, pcellLights.Color(255, 255, 255));
            }
            pcellLights.show();
            cycloLights.show();
        }
        break;
    default:
        return;
    }
}

// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!
boolean setvolume(int8_t v) {
    // cant be higher than 63 or lower than 0
    if (v > 63) v = 63;
    if (v < 0) v = 0;

    Serial.print("Setting volume to ");
    Serial.println(v);
    Wire.beginTransmission(MAX9744_I2CADDR);
    Wire.write(v);
    Serial.print("Volume Set");
    if (Wire.endTransmission() == 0)
        return true;
    else
        return false;
}

void onUnknownCommand()
{
    Serial.println("Command without attached callback");
}