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
#include <Encoder.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ProtonPackCommon.h>
#include <Bounce2.h>
#include <Adafruit_NeoPixel.h>
#include <TimerEvent.h>

#include <CmdMessenger.h>

/*  ----------------------
    Enums for state control
----------------------- */
State state = OFF;

/*  ----------------------
    Pin & Range Definitions
    : Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int ION_SWITCH_PIN = 22;
const int ION_SWITCH_ON = LOW;
const int ROT_BTN_PIN = 9;
const int ROT_BTN_ON = LOW;
const int ROT_ENC_A_PIN = 2;
const int ROT_ENC_B_PIN = 4;

const int IO_1_PIN = 5;
const int IO_1_ON = HIGH;
const int IO_2_PIN = 3;
const int IO_2_ON = HIGH;

// LEDs
const int PCELL_LED_PIN = 14;
const int CYCLO_LED_PIN = 17;
const int VENT_LED_PIN = 16;

const int PCELL_LED_COUNT = 15;
const int CYCLO_LED_COUNT = 4;
const int VENT_LED_COUNT = 3;

/*  ----------------------
    Library objects
----------------------- */
// Audio System
#define mixerChannel1 0
#define mixerChannel2 1
#define mixerAudioInChannel 2

// GUItool: begin automatically generated code
AudioInputI2S            audioIn;           //xy=303,455
AudioPlaySdWav           sdBackgroundChannel;     //xy=326,249
AudioPlaySdWav           sdFXChannel; //xy=328,337
AudioMixer4              audioInMonoMix;         //xy=517,455
AudioMixer4              audioMixer;         //xy=728,389
AudioOutputI2S           audioOutput;           //xy=930,414
AudioAnalyzeRMS          audioRMS;           //xy=913,594
AudioAnalyzePeak         audioPeak;          //xy=916,517
AudioConnection          patchCord1(audioIn, 0, audioInMonoMix, 0);
AudioConnection          patchCord2(audioIn, 1, audioInMonoMix, 1);
AudioConnection          patchCord3(sdBackgroundChannel, 0, audioMixer, mixerChannel1);
AudioConnection          patchCord4(sdFXChannel, 0, audioMixer, mixerChannel2);
AudioConnection          patchCord5(audioInMonoMix, 0, audioMixer, mixerAudioInChannel);
AudioConnection          patchCord6(audioMixer, 0, audioOutput, 0);
AudioConnection          patchCord7(audioMixer, audioPeak);
AudioConnection          patchCord8(audioMixer, audioRMS);
AudioControlSGTL5000     sgtl5000_1;     //xy=625,590
// GUItool: end automatically generated code

#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Amplifier
#define MAX9744_I2CADDR 0x4B
int8_t thevol = 31;

// Neopixels
Adafruit_NeoPixel pcellLights = Adafruit_NeoPixel(PCELL_LED_COUNT, PCELL_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel cycloLights = Adafruit_NeoPixel(CYCLO_LED_COUNT, CYCLO_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ventLights = Adafruit_NeoPixel(VENT_LED_COUNT, VENT_LED_PIN, NEO_GRBW + NEO_KHZ800);

// Serial Comms to Wand
CmdMessenger cmdMessenger = CmdMessenger(Serial1);

/*  ----------------------
    State
----------------------- */
bool ION_SWITCH = false;
bool BTN_ROT = false;
int DIR_ROT = 0;
Encoder ROTARY(ROT_ENC_A_PIN, ROT_ENC_B_PIN);
int lastRotaryPosition = 0;

/*  ----------------------
    Timers
----------------------- */
TimerEvent pcellTimer;
int pcellPeriod = 100;
TimerEvent cycloTimer;
int cycloPeriod = 100;
TimerEvent ventTimer;
int ventPeriod = 100;

/*  ----------------------
    Setup and Loops
----------------------- */
void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    // Initiate the input switches
    // Input switches - Slider Pins Automatically Analogue Read
    pinMode(ION_SWITCH_PIN, INPUT_PULLUP);
    pinMode(ROT_BTN_PIN, INPUT_PULLUP);

    // Set up the amplifier
    Wire.begin();
    if (!setvolume(thevol)) {
        DEBUG_SERIAL.println("Failed to set volume, MAX9744 not found!");
        // Should loop here until ready - or loop for a small amount of time?
        // // Error message over serial to display?
        //while (1);
    }
    
    // Read config file from SD Card
    // TODO: Config File Support

    // Initialise the audio board
    // Audio connections require memory to work.  For more
    // detailed information, see the MemoryAndCpuUsage example
    AudioMemory(16);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.unmuteLineout();
    sgtl5000_1.lineOutLevel(21);

    // Ensure the mono-mixer maxes out at 1
    audioInMonoMix.gain(0, 0.5);
    audioInMonoMix.gain(1, 0.5);

    // Set up the balance of gains initially for two channel FX/MUSIC
    audioMixer.gain(mixerChannel1, 0.5);
    audioMixer.gain(mixerChannel2, 0.5);
    audioMixer.gain(mixerAudioInChannel, 0.0);

    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
    if (!(SD.begin(SDCARD_CS_PIN))) {
        // stop here, but print a message repetitively
        while (1) {
            DEBUG_SERIAL.println("Unable to access the SD card");
            delay(500);
       }
    }

    // Enable Wand Serial Comms
    DEBUG_SERIAL.println("Setting up Serial port");
    cmdMessenger.printLfCr();
    attachCmdMessengerCallbacks();
    DEBUG_SERIAL.println("Serial Setup Complete");

    // Initialise LEDs and Timers
    pcellLights.begin();
    cycloLights.begin();
    ventLights.begin();
    
    cycloLights.clear();
    ventLights.clear();
    
    pcellInit();
    pcellTimer.set(pcellPeriod, pcellUpdate);
    cycloInit();
    cycloTimer.set(cycloPeriod, cycloUpdate);
    ventInit();
    ventTimer.set(ventPeriod, ventUpdate);
    ventTimer.disable();


    pcellLights.show();
    cycloLights.show();
    ventLights.show();

    DEBUG_SERIAL.println("setup() Complete");
}

void attachCmdMessengerCallbacks() {
    cmdMessenger.attach(onUnknownCommand);
    cmdMessenger.attach(eventButtonChange, onSwitchChange);
}

void loop() {
    unsigned long currentMillis = millis();

    //DEBUG_SERIAL.println("Checking Inputs");
    checkInputs(currentMillis);

    if (ION_SWITCH) {
        if (!sdFXChannel.isPlaying()) {
            //DEBUG_SERIAL.println("Playing Track");
            playRandomTrack();
        }
    } else if (sdFXChannel.isPlaying()) {
        //DEBUG_SERIAL.println("Stopping Track");
        sdFXChannel.stop();
    }
    
    //DEBUG_SERIAL.println("Checking Serial");
    cmdMessenger.feedinSerialData();

    /*for (int i = 0; i < CYCLO_LED_COUNT; i++) {
        cycloLights.setPixelColor(i, cycloLights.Color(255, 255, 255, 255));
    }

    cycloLights.show();*/

    // Update timers
    //DEBUG_SERIAL.println("Update Timers");
    pcellTimer.update();
    cycloTimer.update();
    ventTimer.update();
}

/*  ----------------------
    Input Management
----------------------- */
int checkInputs(unsigned long currentMillis) {
    //int initialState = ((int)SW_ACTIVATE * SW_ACTIVATE_BIT) + ((int)BTN_INTENSIFY * BTN_INTENSIFY_BIT) + ((int)SW_LOWER * SW_LOWER_BIT) + ((int)SW_UPPER * SW_UPPER_BIT)
        //+ ((int)BTN_TIP * BTN_TIP_BIT) + ((int)BTN_ROT * BTN_ROT_BIT);

    bool change = false;

    ION_SWITCH = digitalRead(ION_SWITCH_PIN) == ION_SWITCH_ON;
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

    //int newState = SW_ACTIVATE + (BTN_INTENSIFY << 1) + (SW_LOWER << 2) + (SW_UPPER << 3) + (BTN_TIP << 4) + (BTN_ROT << 5);

    //change |= (initialState ^ newState);

    //return initialState ^ newState;

    return 0;
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

        for (int i = 0; i <= PCELL_LED_COUNT; i++) {
            pcellLights.setPixelColor(i, pcellLights.Color(sinVal, cosVal, sinVal));
        }

        pcellLights.show();
    }

    if (currentMillis - lastLEDCycle > ledCycleInterval) {
        lastLEDCycle = currentMillis;
    }    
}

uint32_t colourInterpolate(uint32_t c1, uint32_t c2, int step, int totalSteps) {
    uint8_t c1w = (uint8_t)(c1 >> 24), c1r = (uint8_t)(c1 >> 16), c1g = (uint8_t)(c1 >> 8), c1b = (uint8_t)c1;
    uint8_t c2w = (uint8_t)(c2 >> 24), c2r = (uint8_t)(c2 >> 16), c2g = (uint8_t)(c2 >> 8), c2b = (uint8_t)c2;

    return pcellLights.gamma32(pcellLights.Color((c1r * (totalSteps - step) + c2r * step) / totalSteps,
        (c1g * (totalSteps - step) + c2g * step) / totalSteps,
        (c1b * (totalSteps - step) + c2b * step) / totalSteps,
        (c1w * (totalSteps - step) + c2w * step) / totalSteps));
}


/*  ----------------------
    Music Functions
----------------------- */
void playRandomTrack() {
    if (sdFXChannel.isPlaying()) {
        sdFXChannel.stop();
    }

    sdFXChannel.play(trackList[random(0, 6)][0]);
    delay(10);
}

void onSwitchChange() {
    Button button = (Button) cmdMessenger.readInt16Arg();
    bool value = cmdMessenger.readBoolArg();

    switch (button) {
    case UPPER:
        break;
    default:
        return;
    }
}



void onUnknownCommand()
{
    DEBUG_SERIAL.println("Command without attached callback");
}

// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!
boolean setvolume(int8_t v) {
    // cant be higher than 63 or lower than 0
    if (v > 63) v = 63;
    if (v < 0) v = 0;

    Serial.print("Setting volume to ");
    DEBUG_SERIAL.println(v);
    Wire.beginTransmission(MAX9744_I2CADDR);
    Wire.write(v);
    Serial.print("Volume Set");
    if (Wire.endTransmission() == 0)
        return true;
    else
        return false;
}



/*  ----------------------
    Power Cell Management
----------------------- */
int powerCellLitIndex = 0;
const int powerCellMinPeriod = 1;
const int powerCellIdlePeriod = 1000;
uint32_t pCellColour = pcellLights.Color(150, 150, 150);

void pcellInit() {
    // Idle Animation
    powerCellLitIndex = 0;
    pcellPeriod = powerCellIdlePeriod / PCELL_LED_COUNT;
    pcellLights.clear();
    pcellTimer.setPeriod(pcellPeriod);
}

void pcellUpdate() {
    if (powerCellLitIndex == PCELL_LED_COUNT) {
        powerCellLitIndex = 0;
        pcellLights.clear();
    }
    else {
        pcellLights.setPixelColor(powerCellLitIndex, pCellColour);
        powerCellLitIndex++;
    }    

    pcellLights.show();
}

/*  ----------------------
    Cyclotron Management
----------------------- */
int cyclotronLitIndex = 0;
int cyclotronStep = 0;
int cyclotronFadeInSteps = 5;
int cyclotronFadeOutSteps = 30;
int cyclotronSolidSteps = 200;
const int cyclotronMinPeriod = 1;
const int cyclotronIdlePeriod = 1000;

uint32_t cyclotronLow = cycloLights.Color(25, 0, 0);
uint32_t cyclotronFull = cycloLights.Color(255, 0, 0);

void cycloInit() {
    // Basic rotation animation
    cyclotronLitIndex = 0;
    cyclotronStep = 0;
    cycloPeriod = cyclotronIdlePeriod / cyclotronSolidSteps;
    cycloTimer.setPeriod(cycloPeriod);
}

void cycloUpdate() {
    cycloLights.setPixelColor(cyclotronLitIndex, cyclotronFull);

    if (cyclotronStep < cyclotronFadeOutSteps) {
        cycloLights.setPixelColor((cyclotronLitIndex + 3) % 4, colourInterpolate(cyclotronFull, cyclotronLow, cyclotronStep, cyclotronFadeOutSteps));
    }
    else if (cyclotronStep == cyclotronFadeOutSteps) {
        cycloLights.setPixelColor((cyclotronLitIndex + 3) % 4, 0);
    }

    cyclotronStep++;
    
    if (cyclotronStep >= cyclotronSolidSteps) {
        cyclotronStep = 0;
        cyclotronLitIndex = (cyclotronLitIndex + 1) % 4;
    }

    cycloLights.show();

    //cycloTimer.setPeriod(cycloPeriod);
}

/*  ----------------------
    Vent Management
----------------------- */
void ventInit() {

}

void ventUpdate() {

}
