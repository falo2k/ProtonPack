/*
 This sketch is designed to run on a Teensy 4.0 / Audio Board in the proton pack.
*/

#include <ProtonPackCommon.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Adafruit_NeoPixel.h>
#include <TimerEvent.h>
#include <CmdMessenger.h>
#include <avdweb_Switch.h>
#include <Encoder.h>

/*  ----------------------
    Variables to flip switch directions depending on
    your wiring implementation
----------------------- */
int rotaryDirection = -1;
const bool SW_ION_INVERT = false;
int cyclotronDirection = 1;

/*  ----------------------
    Pin & Range Definitions
    : Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int ION_SWITCH_PIN = 22;
const int ROT_BTN_PIN = 9;
const int ROT_ENC_A_PIN = 2;
const int ROT_ENC_B_PIN = 4;

const int IO_1_PIN = 5;
const int IO_2_PIN = 3;

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
AudioAnalyzeFFT256       audioFFT;
AudioConnection          patchCord1(audioIn, 0, audioInMonoMix, 0);
AudioConnection          patchCord2(audioIn, 1, audioInMonoMix, 1);
AudioConnection          patchCord3(sdBackgroundChannel, 0, audioMixer, mixerChannel1);
AudioConnection          patchCord4(sdFXChannel, 0, audioMixer, mixerChannel2);
AudioConnection          patchCord5(audioInMonoMix, 0, audioMixer, mixerAudioInChannel);
AudioConnection          patchCord6(audioMixer, 0, audioOutput, 0);
AudioConnection          patchCord7(audioMixer, audioPeak);
AudioConnection          patchCord8(audioMixer, audioRMS);
AudioConnection          patchCord9(audioMixer, audioFFT);
AudioControlSGTL5000     sgtl5000_1;     //xy=625,590
// GUItool: end automatically generated code

#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Amplifier
#define MAX9744_I2CADDR 0x4B

// Neopixels
Adafruit_NeoPixel pcellLights = Adafruit_NeoPixel(PCELL_LED_COUNT, PCELL_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel cycloLights = Adafruit_NeoPixel(CYCLO_LED_COUNT, CYCLO_LED_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ventLights = Adafruit_NeoPixel(VENT_LED_COUNT, VENT_LED_PIN, NEO_GRBW + NEO_KHZ800);

// Serial Comms to Wand
CmdMessenger cmdMessenger = CmdMessenger(Serial1);

// Switches and buttons
Encoder ROTARY(ROT_ENC_A_PIN, ROT_ENC_B_PIN);
Switch ionSwitch = Switch(ION_SWITCH_PIN);
Switch encoderButton = Switch(ROT_BTN_PIN);

/*  ----------------------
    Timers
----------------------- */
TimerEvent pcellTimer;
int pcellTimerPeriod = 50;
TimerEvent cycloTimer;
int cycloTimerPeriod = 50;
TimerEvent ventTimer;
int ventTimerPeriod = 50;

/*  ----------------------
    State control
----------------------- */
State state = OFF;

int lastRotaryPosition = 0;
bool bluetoothMode = false;
int trackNumber = defaultTrack;
unsigned long stateLastChanged;

// POWER CELL STATE
int powerCellLitIndex = 0;
const int powerCellIdlePeriod = 1500;
int powerCellPeriod = 1500;
const int powerDownPCellDecayTickPeriod = (int)(0.025 * POWERDOWN_TIME);
unsigned long lastPowerCellChange;
uint32_t pCellColour = Adafruit_NeoPixel::Color(150, 150, 150);

// CYCLOTRON STATE
int cyclotronLitIndex = 0;
unsigned long lastCycloChange[CYCLO_LED_COUNT];
const int cyclotronMinPeriod = 100;
const int cyclotronIdlePeriod = 4000;
// ms to reduce spin period per ms of firing
const float cyclotronFiringAcceleration = 0.05;
int cyclotronSpinPeriod = 4000;
// Rate for cyclotron lamps to fade under normal operation
const int lampFadeMs = 250;
const float cycloSpinDecayRate = 255.0 / lampFadeMs;

uint32_t savedCyclotronValues[CYCLO_LED_COUNT];

const unsigned long powerDownCycloDecayTickPeriod = (int)(0.1 * POWERDOWN_TIME);
// Decay rate for powercell shutdown in pixel value per ms.  
// At most full lights switch off in 80% of shutdown time
const float powerDownCycloDecayRate = (float)255 / (0.8 * POWERDOWN_TIME);

const int cyclotronLowR = 25;
const int cyclotronLowG = 0;
const int cyclotronLowB = 0;
const int cyclotronLowW = 0;
const int cyclotronFullR = 255;
const int cyclotronFullG = 0;
const int cyclotronFullB = 0;
const int cyclotronFullW = 0;
uint32_t cyclotronLow = Adafruit_NeoPixel::Color(cyclotronLowR, cyclotronLowG, cyclotronLowB, cyclotronLowW);
uint32_t cyclotronFull = Adafruit_NeoPixel::Color(cyclotronFullR, cyclotronFullG, cyclotronFullB, cyclotronFullW);

/*  ----------------------
    Setup and Loops
----------------------- */
void setup() {
    // Initialise Serial for Debug + Wand Connection
    Serial.begin(9600);
    Serial1.begin(9600);

    DEBUG_SERIAL.println("Starting Setup");

    // Enable Wand Serial Comms
    DEBUG_SERIAL.println("Setting up Serial port");
    cmdMessenger.printLfCr();
    attachCmdMessengerCallbacks();
    DEBUG_SERIAL.println("Serial Setup Complete");

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
    AudioMemory(24);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.75);
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    //sgtl5000_1.unmuteLineout();
    //sgtl5000_1.lineOutLevel(21);

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

    // Initialise LEDs and Timers
    pcellLights.begin();
    cycloLights.begin();
    ventLights.begin();
    
    cycloLights.clear();
    ventLights.clear();
    
    pcellInit();
    pcellTimer.set(pcellTimerPeriod, pcellUpdate);
    cycloInit();
    cycloTimer.set(cycloTimerPeriod, cycloUpdate);
    ventInit();
    ventTimer.set(ventTimerPeriod, ventUpdate);
    ventTimer.disable();

    pcellLights.show();
    cycloLights.show();
    ventLights.show();

    // Initiate the input switches
    setupInputs();

    DEBUG_SERIAL.println("setup() Complete");
}


void loop() {
    unsigned long currentMillis = millis();

    // Get any updates from wand
    cmdMessenger.feedinSerialData();

    // Process Switch Updates
    updateInputs(currentMillis);

    // Process State Updates
    stateUpdate(currentMillis);

    // Process LED Updates
    pcellTimer.update();
    cycloTimer.update();
    ventTimer.update();

    // Update the music playing state for the wand
    // TODO: Move this into stateUpdate
    if (state == MUSIC_MODE) {
        cmdMessenger.sendCmd(eventUpdateMusicPlayingState, sdFXChannel.isPlaying());
    }
}

/*  ----------------------
    Input Management
----------------------- */
void setupInputs() {
    ionSwitch.setPushedCallback(&ionSwitchToggle);
    ionSwitch.setReleasedCallback(&ionSwitchToggle);
    encoderButton.setPushedCallback(&rotaryButtonPress);
    encoderButton.setReleasedCallback(&rotaryButtonRelease);
    encoderButton.setLongPressCallback(&rotaryButtonLongPress);
}

void updateInputs(unsigned long currentMillis) {
    // Handle Switches
    ionSwitch.poll();
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

void ionSwitchToggle(void* ref) {
    DEBUG_SERIAL.print("ACT Switch: "); DEBUG_SERIAL.println(ionSwitch.on() ^ SW_ION_INVERT);

    cmdMessenger.sendCmd(eventPackIonSwitch, ionSwitch.on() ^ SW_ION_INVERT);
}

bool encoderHeld = false;
bool encoderPressed = false;

void rotaryButtonPress(void* ref) {
    DEBUG_SERIAL.println("Rotary Pressed");
    encoderPressed = true;
    cmdMessenger.sendCmd(eventPackEncoderButton, PRESSED);
}

void rotaryButtonLongPress(void* ref) {
    DEBUG_SERIAL.println("Rotary Held");
    encoderHeld = true;

    cmdMessenger.sendCmd(eventPackEncoderButton, HELD);    
}

void rotaryButtonRelease(void* ref) {
    // Encoder is bouncing weirdly on startup
    if (!encoderPressed) { return; }
    else {
        encoderPressed = false;
    }

    DEBUG_SERIAL.println("Rotary Released");

    if (encoderHeld) {
        encoderHeld = false;
    }

    cmdMessenger.sendCmd(eventPackEncoderButton, RELEASED);
}

void rotaryMove(unsigned long currentMillis, int movement) {
    cmdMessenger.sendCmd(eventPackEncoderTurn, movement);
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

// Setting the volume is very simple! Just write the 6-bit
// volume to the i2c bus. That's it!
boolean setvolume(int8_t v) {
    // cant be higher than 63 or lower than 0
    if (v > maxvol) v = maxvol;
    if (v < 0) v = 0;

    DEBUG_SERIAL.print("Setting volume to ");
    DEBUG_SERIAL.println(v);
    Wire.beginTransmission(MAX9744_I2CADDR);
    Wire.write(v);
    DEBUG_SERIAL.print("Volume Set");
    if (Wire.endTransmission() == 0)
        return true;
    else
        return false;
}

void toggleBluetoothModule(bool state) {
    // TODO: Bluetooth module management
}

/*  ----------------------
    Serial Command Handling
----------------------- */
// Handle these: 
//eventSetVolume,
//eventChangeState,
//eventSetBluetoothMode,
//eventSetSDTrack,
//eventPlayPauseSDTrack,
//eventStopSDTrack,

void attachCmdMessengerCallbacks() {
    cmdMessenger.attach(onUnknownCommand);
    cmdMessenger.attach(eventSetVolume, onSetVolume);
    cmdMessenger.attach(eventChangeState, onChangeState);
    cmdMessenger.attach(eventSetBluetoothMode, onSetBluetoothMode);
    cmdMessenger.attach(eventSetSDTrack, onSetSDTrack);
    cmdMessenger.attach(eventPlayPauseSDTrack, onPlayPauseSDTrack);
    cmdMessenger.attach(eventStopSDTrack, onStopSDTrack);
}

void onUnknownCommand()
{
    DEBUG_SERIAL.println("Command without attached callback");
}

void onSetVolume() {
    int theVol = cmdMessenger.readInt16Arg();
    setvolume(theVol);
}

void onChangeState() {
    State newState = (State)cmdMessenger.readInt16Arg();
    initialiseState(newState, millis());
}

void onSetBluetoothMode() {
    bool arg = cmdMessenger.readBoolArg();
    
    bluetoothMode = arg;

    // TODO: If music mode, switch off audio (if necessary), change volume on mixer channels,
    toggleBluetoothModule(bluetoothMode);
}

void onSetSDTrack() {
    int newTrackNumber = cmdMessenger.readInt16Arg();

    if (newTrackNumber != trackNumber) {
        trackNumber = newTrackNumber;

        if (sdFXChannel.isPlaying() || sdFXChannel.isPaused()) {
            sdFXChannel.stop();
            sdFXChannel.play(trackList[trackNumber][0]);
        }
    }
}

void onPlayPauseSDTrack() {
    if ((state == MUSIC_MODE) && !bluetoothMode) {
        if (sdFXChannel.isPaused()) {
            sdFXChannel.togglePlayPause();
        }
        else if (sdFXChannel.isPlaying()) {
            sdFXChannel.togglePlayPause();
        }
        else {
            sdFXChannel.play(trackList[trackNumber][0]);
        }
    }
}

void onStopSDTrack() {
    if ((state == MUSIC_MODE) && !bluetoothMode) {
        sdFXChannel.stop();
    }
}

/*  ----------------------
    State Management
----------------------- */
void initialiseState(State newState, unsigned long currentMillis) {
    // TODO: State initlisation

    DEBUG_SERIAL.print("Initialising State: "); DEBUG_SERIAL.println((char)newState);

    switch (newState) {
    case OFF:
        pcellLights.clear();
        cycloLights.clear();
        ventLights.clear();
        sdBackgroundChannel.stop();
        sdFXChannel.stop();
        toggleBluetoothModule(false);
        break;

    case BOOTING:
        // Start from all lights off
        pcellLights.clear();
        cycloLights.clear();
        ventLights.clear();
        sdBackgroundChannel.stop();
        sdFXChannel.play(sfxBoot);
        break;

    case IDLE:
        // Ensure we stop the music if we're coming from music mode,
        // and make sure bluetooth module is shut down
        if (state == MUSIC_MODE) {
            sdFXChannel.stop();
            toggleBluetoothModule(false);
        }
        powerCellPeriod = powerCellIdlePeriod;
        cyclotronSpinPeriod = cyclotronIdlePeriod;        
        for (int i = 0; i < CYCLO_LED_COUNT; i++) {
            lastCycloChange[i] = currentMillis;
        }
        //cycloLights.clear();
        cycloLights.setPixelColor(cyclotronLitIndex, cyclotronFull);
        pcellLights.clear();
        ventLights.clear();
        break;

    case POWERDOWN:
        for (int i = 0; i < CYCLO_LED_COUNT; i++) {
            savedCyclotronValues[i] = cycloLights.getPixelColor(i);
        }
        break;

    case FIRING:
    case FIRING_WARN:
    case FIRING_STOP:         
        break;
        
    case VENTING:
        break;
    
    case MUSIC_MODE:
        pcellLights.clear();
        cycloLights.clear();
        ventLights.clear();
        sdBackgroundChannel.stop();
        sdFXChannel.stop();
        // TODO: Enable music mode, set appropriate mixer controls, set bluetooth if necessary
        toggleBluetoothModule(bluetoothMode);
        break;

    default:
        break;
    }

    pcellLights.show();
    cycloLights.show();
    ventLights.show();

    stateLastChanged = millis();

    state = newState;
}

unsigned long lastStateUpdate = 0;

void stateUpdate(unsigned long currentMillis) {
    // TODO: State Management
    switch (state) {
    case OFF:
        break;

    case BOOTING:
        if (currentMillis > stateLastChanged + BOOTING_TIME) {
            //initialiseState(IDLE, currentMillis);
        }
        break;

    case IDLE:
        break;

    case POWERDOWN:
        if (currentMillis > stateLastChanged + POWERDOWN_TIME) {
            //initialiseState(OFF, currentMillis);
        }
        break;

    case FIRING:
    case FIRING_WARN:
    case FIRING_STOP:
        cyclotronSpinPeriod = max(cyclotronMinPeriod, cyclotronSpinPeriod - ((currentMillis - lastStateUpdate) * cyclotronFiringAcceleration));
        break;

    case VENTING:
        if (currentMillis > stateLastChanged + VENT_TIME) {
            //initialiseState(BOOTING, currentMillis);
        }
        break;

    case MUSIC_MODE:
        break;

    default:
        break;
    }

    lastStateUpdate = currentMillis;
}

/*  ----------------------
    Power Cell Management
----------------------- */
void pcellInit() {
    lastPowerCellChange = millis();
    powerCellPeriod = powerCellIdlePeriod;
    powerCellLitIndex = 0;
    pcellLights.clear();    
}

void pcellUpdate() {
    unsigned long currentMillis = millis();

    switch (state) {
        case OFF:
	        break;

        case BOOTING: {
                int newPowerCellLitIndex = round(2 * (((float)max(stateLastChanged, currentMillis) - stateLastChanged) / BOOTING_TIME) * PCELL_LED_COUNT);
                powerCellLitIndex = newPowerCellLitIndex < PCELL_LED_COUNT ? newPowerCellLitIndex : (2 * PCELL_LED_COUNT) - newPowerCellLitIndex;
                pcellLightTo(powerCellLitIndex);
                lastPowerCellChange = currentMillis;
            }
	        break;

        case IDLE:
            if ((currentMillis - lastPowerCellChange) > (((float)powerCellPeriod) / PCELL_LED_COUNT)) {
                if (powerCellLitIndex >= PCELL_LED_COUNT) {
                    powerCellLitIndex = 0;
                    pcellLights.clear();
                    pcellLights.show();
                }
                else {
                    pcellLightTo(powerCellLitIndex);
                    powerCellLitIndex++;
                }
                lastPowerCellChange = currentMillis;
            }
	        break;

        case POWERDOWN:
            if ((currentMillis - lastPowerCellChange) > powerDownPCellDecayTickPeriod) {
                if (powerCellLitIndex >= 0) {
                    powerCellLitIndex--;
                    pcellLightTo(powerCellLitIndex);
                    lastPowerCellChange = currentMillis;
                }
            }
	        break;

        case FIRING:
            // Make the period shrink over time by moving the lowest index up (to a max position)
	        break;

        case FIRING_WARN:
	        break;

        case VENTING:
	        break;

        case FIRING_STOP:
	        break;

        case MUSIC_MODE: {
                if (audioPeak.available()) {
                    float peak = audioPeak.read();
                    float peakMultiplier = 1.5;
                    
                    powerCellLitIndex = floor(peakMultiplier * peak * PCELL_LED_COUNT);
                    pcellLightTo(powerCellLitIndex, false);
                    if (powerCellLitIndex < PCELL_LED_COUNT) {
                        float remainder = 0.01 * (((int) (peakMultiplier * peak * PCELL_LED_COUNT * 100.0)) % 100);
                        pcellLights.setPixelColor(powerCellLitIndex + 1, Adafruit_NeoPixel::Color(0, 0, floor(remainder * 255), floor(remainder * 100)));
                    }
                    pcellLights.show();
                }
                lastPowerCellChange = currentMillis;
            }
	        break;

        default:
	        break;
    }
}

void pcellLightTo(int lightIndex) {
    pcellLightTo(lightIndex, true);
}

void pcellLightTo(int lightIndex, bool show) {
    pcellLights.clear();
    for (int i = 0; i <= lightIndex; i++) {
        pcellLights.setPixelColor(i, pCellColour);
    }
    if (show) { pcellLights.show(); }
}

/*  ----------------------
    Cyclotron Management
----------------------- */
void cycloInit() {
    unsigned long currentMillis = millis();
    cyclotronLitIndex = 0;
    cyclotronSpinPeriod = cyclotronIdlePeriod;
    for (int i = 0; i < CYCLO_LED_COUNT; i++) {
        lastCycloChange[i] = currentMillis;
    }
}

void cycloUpdate() {
    unsigned long currentMillis = millis();

    switch (state) {
    case OFF:
        break;

    case BOOTING:{
            float cycloIntensity = ((float) (max(stateLastChanged, currentMillis) - stateLastChanged)) / BOOTING_TIME;
            for (int i = 0; i < CYCLO_LED_COUNT; i++) {                
                cycloLights.setPixelColor(i, Adafruit_NeoPixel::Color((int)(cycloIntensity * cyclotronFullR), (int)(cycloIntensity * cyclotronFullG), (int)(cycloIntensity * cyclotronFullB), (int)(cycloIntensity * cyclotronFullW)));
                lastCycloChange[i] = currentMillis;
            }

            cycloLights.show();           
        }
        break;

        case FIRING:
        case FIRING_WARN:
        case FIRING_STOP:
        case IDLE: {
            // Move the lit lamp if we hit the period change
            if ((currentMillis - lastCycloChange[cyclotronLitIndex]) > (((float)cyclotronSpinPeriod) / CYCLO_LED_COUNT)) {
                // Set the current time as last update for the old lamp, and save value at time
                lastCycloChange[cyclotronLitIndex] = currentMillis;
                savedCyclotronValues[cyclotronLitIndex] = cycloLights.getPixelColor(cyclotronLitIndex);
                cyclotronLitIndex = looparound(cyclotronLitIndex + cyclotronDirection, CYCLO_LED_COUNT);                
                cycloLights.setPixelColor(cyclotronLitIndex, cyclotronFull);

                // Set the current time as last update for the new lamp, and save value at time
                lastCycloChange[cyclotronLitIndex] = currentMillis;
                savedCyclotronValues[cyclotronLitIndex] = cycloLights.getPixelColor(cyclotronLitIndex);
            }            

            // Fade non-lit lamps
            for (int i = 0; i < CYCLO_LED_COUNT; i++) {
                if (i != cyclotronLitIndex) {
                    unsigned long timeSinceLastChange = max(currentMillis, lastCycloChange[i]) - lastCycloChange[i];
                    cycloLights.setPixelColor(i,
                        Adafruit_NeoPixel::Color(
                            max(0, Red(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange))),
                            max(0, Green(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange))),
                            max(0, Blue(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange))),
                            max(0, White(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange)))
                        )
                    );
                }
            }

            cycloLights.show();
        }
        break;

    case POWERDOWN: {
            unsigned long timeSinceShutdown = max(currentMillis, stateLastChanged) - stateLastChanged;
            for (int i = 0; i < CYCLO_LED_COUNT; i++) {
                cycloLights.setPixelColor(i,
                    Adafruit_NeoPixel::Color(
                        max(0,Red(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown))),
                        max(0, Green(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown))),
                        max(0, Blue(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown))),
                        max(0, White(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown)))
                    )
                );
                lastCycloChange[i] = currentMillis;
            }
            cycloLights.show();
        }
        break;

    case VENTING:
        break;
            
    case MUSIC_MODE: {
            if (audioRMS.available()) {
                float rms = audioRMS.read();
                int brightness = min(255,255 * rms * 2);
                for (int i = 0; i < CYCLO_LED_COUNT; i++) {
                    cycloLights.setPixelColor(i, Adafruit_NeoPixel::Color(brightness, 0, 0));
                }
                cycloLights.show();
            }
        }
        break;

    default:
        break;
    }
}

/*  ----------------------
    Vent Management
----------------------- */
void ventInit() {

}

void ventUpdate() {

}