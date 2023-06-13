/*
 This sketch is designed to run on a Teensy 4.0 / Audio Board in the proton pack.
*/

#include <ProtonPackCommon.h>
#include <HardwareConfig.h>

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
#include <minINI.h>

/*  ----------------------
    Variables to flip switch directions depending on
    your wiring implementation
----------------------- */
int rotaryDirection = 1;
const bool SW_ION_INVERT = false;
int cyclotronDirection = 1;

/*  ----------------------
    Pin & Range Definitions
    : Note that for ranges the indexes are inclusive
----------------------- */
// Switches
const int ION_SWITCH_PIN = 22;
const int ROT_BTN_PIN = 9;
const int ROT_ENC_A_PIN = 4;
const int ROT_ENC_B_PIN = 2;

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

bool wandConnected = false;

// POWER CELL STATE
int powerCellLitBar = 0;
const int powerCellIdlePeriod = 1000;
const int powerCellFiringPeriod = 500;
int powerCellPeriod = 1000;
const int powerDownPCellDecayTickPeriod = (int)(0.025 * POWERDOWN_TIME);
unsigned long lastPowerCellChange;
const uint32_t pCellColour = Adafruit_NeoPixel::Color(150, 150, 250);
float pCellMinBar = 0.0;
float pCellMaxMinBar = 12;
// Rate for power cell minimum to climb when firing
const int pCellClimbPeriod = 500;
const float pCellClimbRate = 1.0 / pCellClimbPeriod;
const float peakMultiplier = 2;

// CYCLOTRON STATE
int cyclotronLitIndex = 0;
unsigned long lastCycloChange[CYCLO_LED_COUNT];
const int cyclotronMinPeriod = 100;
const int cyclotronIdlePeriod = 4000;
// ms to reduce spin period per ms of firing
const float cyclotronFiringAcceleration = 0.01;
int cyclotronSpinPeriod = 4000;
// Rate for cyclotron lamps to fade under normal operation
const int lampFadeMs = 250;
const float cycloSpinDecayRate = 255.0 / lampFadeMs;

// Scaling used for audio visualisation
const float fftMultiplier = 2;
const float fftExponent = 0.5;
const float fftThreshold = 0.02;
const float rmsMultiplier = 4;
const float rmsExponent = 1;

uint32_t savedCyclotronValues[CYCLO_LED_COUNT];

//const unsigned long powerDownCycloDecayTickPeriod = (int)(0.1 * POWERDOWN_TIME);
// Decay rate for powercell shutdown in pixel value per ms.  
// At most full lights switch off in 80% of shutdown time
const float powerDownCycloDecayRate = (float)255 / (0.8 * POWERDOWN_TIME);

uint32_t cyclotronFull = Adafruit_NeoPixel::Color(255, 0, 0, 0);
uint32_t cyclotronOverheat = Adafruit_NeoPixel::Color(255, 0, 0, 10);
uint32_t ventColour = Adafruit_NeoPixel::Color(255, 255, 255, 255);

// Time out and enter demo mode (music mode with bluetooth) if
// it's been 5s since booting and no sign of the wand
unsigned long bootTime;
const unsigned long wandTimeoutTime = 5000;

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
    // This should now happen in the first call to load the config file
    //if (!setvolume(theVol)) {
    //    DEBUG_SERIAL.println("Failed to set volume, MAX9744 not found!");
    //    // Should loop here until ready - or loop for a small amount of time?
    //    // // Error message over serial to display?
    //    //while (1);
    //}
    
    // Initialise the audio board
    // Audio connections require memory to work.  For more
    // detailed information, see the MemoryAndCpuUsage example
    AudioMemory(24);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.75);
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    audioFFT.averageTogether(10);
    //audioFFT.windowFunction(NULL);

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

    pcellLights.show();
    cycloLights.show();
    ventLights.show();

    // Initiate the input switches
    setupInputs();

    // Set up output pins
    pinMode(IO_1_PIN, OUTPUT); // Bluetooth control
    digitalWrite(IO_1_PIN, LOW);

    bootTime = millis();
    DEBUG_SERIAL.println("setup() Complete");
}

// Tracker time to space out checks for wand connection
unsigned long lastWandCheck = millis();

void loop() {
    unsigned long currentMillis = millis();

    if (!wandConnected) {
        if (((currentMillis - lastWandCheck) > 250)) {
            DEBUG_SERIAL.println("Pack waiting for Wand ...");
            cmdMessenger.sendCmd(eventPackConnect);
            lastWandCheck = currentMillis;
        }
        cmdMessenger.feedinSerialData();

        // If we timeout turn on music mode and bluetooth
        // Pretend the wand is connected
        // Otherwise keep waiting
        if ((currentMillis - bootTime) > wandTimeoutTime) {
            DEBUG_SERIAL.println("Waiting for wand timed out.  Enabling demo mode.");
            wandConnected = true;

            readConfigFile(currentMillis);
            setvolume(theVol);

            initialiseState(MUSIC_MODE, currentMillis);
            bluetoothMode = true;
            toggleBluetoothModule(bluetoothMode);
        }
        else {
            return;
        }        
    }

    // Process State Updates
    stateUpdate(currentMillis);

    // Get any updates from wand
    cmdMessenger.feedinSerialData();

    // Process Switch Updates
    updateInputs(currentMillis);    

    // Process LED Updates
    pcellTimer.update();
    cycloTimer.update();
    ventTimer.update();
}

/*  ----------------------
    Config Management
----------------------- */
void readConfigFile(unsigned long currentMillis) {
    if (SD.exists(configFile)) {
        DEBUG_SERIAL.println("Config File Found.  Reading Config.");
        minIni ini(configFile);
        theVol = ini.geti("", configVolume, defaultVol);
        trackNumber = ini.geti("", configTrack, defaultTrack);
        OVERHEAT_TIME = ini.geti("", configOverheat, OVERHEAT_DEFAULT_TIME);
        FIRING_WARN_TIME = ini.geti("", configWarn, FIRING_WARN_DEFAULT_TIME);
    }
    else {
        DEBUG_SERIAL.println("No Config File Found.  Using Default values.");
        trackNumber = defaultTrack;
        theVol = defaultVol;
        OVERHEAT_TIME = OVERHEAT_DEFAULT_TIME;
        FIRING_WARN_TIME = FIRING_WARN_DEFAULT_TIME;
    }
}

void writeConfigFile(unsigned long currentMillis) {
    DEBUG_SERIAL.println("Saving config file to SD.");
    
    minIni ini(configFile);

    ini.put("", configVolume, theVol);
    ini.put("", configTrack, trackNumber);
    ini.put("", configOverheat, OVERHEAT_TIME);
    ini.put("", configWarn, FIRING_WARN_TIME);
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
        DEBUG_SERIAL.print("Rotary Position: "); DEBUG_SERIAL.println(newRotaryPosition);
        int movement = rotaryDirection * round(0.25 * (newRotaryPosition - lastRotaryPosition));
        lastRotaryPosition = newRotaryPosition;

        DEBUG_SERIAL.print("Rotary Movement: "); DEBUG_SERIAL.println(movement);

        rotaryMove(currentMillis, movement);
    }
}

void ionSwitchToggle(void* ref) {
    DEBUG_SERIAL.print("Ion Switch: "); DEBUG_SERIAL.println(ionSwitch.on() ^ SW_ION_INVERT);

    cmdMessenger.sendCmd(eventPackIonSwitch, ionSwitch.on() ^ SW_ION_INVERT);
}

bool encoderHeld = false;
bool encoderPressed = false;

void rotaryButtonPress(void* ref) {
    DEBUG_SERIAL.println("Rotary Pressed");
    encoderPressed = true;
    //cmdMessenger.sendCmd(eventPackEncoderButton, PRESSED);
}

void rotaryButtonLongPress(void* ref) {
    DEBUG_SERIAL.println("Rotary Held");
    encoderHeld = true;
    //cmdMessenger.sendCmd(eventPackEncoderButton, HELD);    
}

void rotaryButtonRelease(void* ref) {
    // Encoder is bouncing weirdly on startup.  Ingore if it wasn't pressed in the first place.
    if (!encoderPressed) { return; }
    else {
        encoderPressed = false;
    }

    DEBUG_SERIAL.println("Rotary Released");

    if (encoderHeld) {
        encoderHeld = false;
    }

    //cmdMessenger.sendCmd(eventPackEncoderButton, RELEASED);
}

void rotaryMove(unsigned long currentMillis, int movement) {
    // Change the volume only if held and moved
    if (encoderHeld) {
        setvolume(theVol + movement);
        cmdMessenger.sendCmd(eventSetVolume, theVol);
        cmdMessenger.sendCmd(eventDisplayVolume, theVol);
    }
    //cmdMessenger.sendCmd(eventPackEncoderTurn, movement);
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
    if (v > maxVol) v = maxVol;
    if (v < 0) v = 0;

    if (v != theVol) {
        DEBUG_SERIAL.printf("Setting volume to %i\n", v);
    }

    theVol = v;

    Wire.beginTransmission(MAX9744_I2CADDR);
    Wire.write(v);
    if (Wire.endTransmission() == 0) {        
        return true;
    }
    else {
        return false;
    }
}

void toggleBluetoothModule(bool state) {
    DEBUG_SERIAL.print("Switching Bluetooth Module: "); DEBUG_SERIAL.println(state);

    if (state) {
        sdFXChannel.stop();
        audioMixer.gain(mixerChannel2, 0.0);
        audioMixer.gain(mixerAudioInChannel, 0.5);
        digitalWrite(IO_1_PIN, HIGH);
    }
    else {
        audioMixer.gain(mixerChannel2, 0.5);
        audioMixer.gain(mixerAudioInChannel, 0.0);
        digitalWrite(IO_1_PIN, LOW);
    }
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
    cmdMessenger.attach(eventLoadConfig, onLoadConfig);
    cmdMessenger.attach(eventWriteConfig, onWriteConfig);
    cmdMessenger.attach(eventWandConnect, onWandConnect);
    cmdMessenger.attach(eventSendConfigToPack, onSendConfigToPack);
}

void onUnknownCommand()
{
    DEBUG_SERIAL.println("Command without attached callback");
}

void onSetVolume() {
    theVol = cmdMessenger.readInt16Arg();
    setvolume(theVol);
}

void onChangeState() {
    State newState = (State)cmdMessenger.readInt16Arg();
    initialiseState(newState, millis());
}

void onSetBluetoothMode() {
    bool arg = cmdMessenger.readBoolArg();
    
    bluetoothMode = arg;

    if (state == MUSIC_MODE) {
        toggleBluetoothModule(bluetoothMode);
    }
}

void onSetSDTrack() {
    int newTrackNumber = cmdMessenger.readInt16Arg();

    if (newTrackNumber != trackNumber) {
        DEBUG_SERIAL.printf("Setting Track to %s\n", trackList[newTrackNumber][1]);
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

void onLoadConfig() {
    readConfigFile(millis());
    cmdMessenger.sendCmd(eventSetVolume, theVol);
    cmdMessenger.sendCmd(eventSetSDTrack, trackNumber);
    cmdMessenger.sendCmdStart(eventSendConfigToWand);
    cmdMessenger.sendCmdArg(OVERHEAT_TIME);
    cmdMessenger.sendCmdArg(FIRING_WARN_TIME);
    cmdMessenger.sendCmdEnd();
}

void onWriteConfig() {
    writeConfigFile(millis());
}

void onSendConfigToPack() {
    DEBUG_SERIAL.println("Received configuration from wand");
    OVERHEAT_TIME = cmdMessenger.readInt16Arg();
    FIRING_WARN_TIME = cmdMessenger.readInt16Arg();
}

// Wand Connect - Send acknowledgment
void onWandConnect() {
    DEBUG_SERIAL.println("Wand Connected");
    wandConnected = true;
    cmdMessenger.sendCmd(eventPackConnect);
}

/*  ----------------------
    State Management
----------------------- */
void initialiseState(State newState, unsigned long currentMillis) {
    DEBUG_SERIAL.print("Initialising State: "); DEBUG_SERIAL.println((char)newState);

    // Ensure we stop the music if we're coming from music mode,
    // and make sure bluetooth module is shut down
    if (state == MUSIC_MODE && newState != MUSIC_MODE) {
        sdFXChannel.stop();
        toggleBluetoothModule(false);
    }

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
        powerCellPeriod = powerCellIdlePeriod;
        cyclotronSpinPeriod = cyclotronIdlePeriod;        
        pCellMinBar = 0.0;
        pcellLights.clear();
        ventLights.clear();

        // Store current cyclotron value state for bulb fade - at this point
        // I'm wondering if I should just cache these on any change, but here we are
        for (int i = 0; i < CYCLO_LED_COUNT; i++) {
            savedCyclotronValues[i] = cycloLights.getPixelColor(i);
        }

        break;

    case POWERDOWN:
        for (int i = 0; i < CYCLO_LED_COUNT; i++) {
            savedCyclotronValues[i] = cycloLights.getPixelColor(i);
        }

        sdBackgroundChannel.stop();
        sdFXChannel.play(sfxPowerDown);
        break;

    case FIRING:
        pCellMinBar = 0.0;// 0.1 * PCELL_LED_COUNT;
        powerCellPeriod = powerCellFiringPeriod;

        sdFXChannel.play(sfxFireHead);
        sdBackgroundChannel.play(sfxFireLoop);
        break;

    case FIRING_STOP:
        sdBackgroundChannel.stop();
        sdBackgroundChannel.play(sfxHum);
        sdFXChannel.play(sfxFireTail);
        break;

    case FIRING_WARN:
        break;
        
    case VENTING: {
            for (int i = 0; i < VENT_LED_COUNT; i++) {
                ventLights.setPixelColor(i, ventColour);
            }
            for (int i = 0; i < CYCLO_LED_COUNT; i++) {
                cycloLights.setPixelColor(i, cyclotronOverheat);
            }

            sdBackgroundChannel.stop();
            sdFXChannel.play(sfxVent);
        }
        break;
    
    case MUSIC_MODE:
        pcellLights.clear();
        cycloLights.clear();
        ventLights.clear();
        sdBackgroundChannel.stop();
        sdFXChannel.stop();

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
    switch (state) {
    case OFF:
        break;

    case BOOTING:
        // First check is because I cba to go back and refactor for race conditions (really I should use millis() each time rather than caching it)
        // But if I do get a sequence wrong then the negative unsigned becomes very large!  This is just defence against my laziness.
        if ((currentMillis > stateLastChanged) && ((currentMillis - stateLastChanged) > BOOTING_HUM_START) && (!sdBackgroundChannel.isPlaying())) {
            sdBackgroundChannel.play(sfxHum);
        }
        break;

    case IDLE:
        if (!sdBackgroundChannel.isPlaying()) {
            sdBackgroundChannel.play(sfxHum);
        }
        break;

    case POWERDOWN:        
        break;

    case FIRING_WARN:
        if (!sdFXChannel.isPlaying()) {
            sdFXChannel.play(sfxOverheat);
        }
    case FIRING:    
        if (!sdBackgroundChannel.isPlaying()) {
            sdBackgroundChannel.play(sfxFireLoop);
        }

        cyclotronSpinPeriod = max(cyclotronMinPeriod, cyclotronSpinPeriod - (cyclotronFiringAcceleration * (currentMillis - lastStateUpdate)));
        pCellMinBar = min(pCellMaxMinBar, pCellMinBar + (pCellClimbRate * (currentMillis - lastStateUpdate)));
        break;

    case FIRING_STOP:
        cyclotronSpinPeriod = min(cyclotronIdlePeriod, cyclotronSpinPeriod + (100 * cyclotronFiringAcceleration * (currentMillis - lastStateUpdate)));
        pCellMinBar = max(0.0, pCellMinBar - (3 * pCellClimbRate * (currentMillis - lastStateUpdate)));
        break;

    case VENTING:
        break;

    case MUSIC_MODE:
        if (!bluetoothMode) {
            cmdMessenger.sendCmd(eventUpdateMusicPlayingState, sdFXChannel.isPlaying());
        }
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
    powerCellLitBar = 0;
    pCellMinBar = 0.0;
    pcellLights.clear();    
}

void pcellUpdate() {
    unsigned long currentMillis = millis();

    switch (state) {
        case OFF:
	        break;

        case BOOTING: {
                int newPowerCellLitIndex = round(2 * (((float)max(stateLastChanged, currentMillis) - stateLastChanged) / BOOTING_TIME) * PCELL_LED_COUNT);
                powerCellLitBar = max(0, newPowerCellLitIndex < PCELL_LED_COUNT ? newPowerCellLitIndex : (2 * PCELL_LED_COUNT) - newPowerCellLitIndex);
                pcellLightTo(powerCellLitBar);
                lastPowerCellChange = currentMillis;
            }
	        break;

        case FIRING:
        case FIRING_WARN:
        case FIRING_STOP:
        case IDLE:
            if ((currentMillis - lastPowerCellChange) > (((float)powerCellPeriod) / PCELL_LED_COUNT)) {
                if (powerCellLitBar >= PCELL_LED_COUNT) {
                    powerCellLitBar = floor(pCellMinBar);
                } 
                else {                    
                    powerCellLitBar++;
                }
                pcellLightTo(powerCellLitBar);
                lastPowerCellChange = currentMillis;
            }
	        break;

        case POWERDOWN:
            if ((currentMillis - lastPowerCellChange) > powerDownPCellDecayTickPeriod) {
                if (powerCellLitBar > 0) {
                    powerCellLitBar--;
                    pcellLightTo(powerCellLitBar);
                    lastPowerCellChange = currentMillis;
                }
                else {
                    pcellLights.clear();
                    lastPowerCellChange = currentMillis;
                }
            }
	        break;
        
        case VENTING: {
                unsigned long timeSinceVent = max(currentMillis, stateLastChanged) - stateLastChanged;
                if (timeSinceVent < VENT_LIGHT_FADE_TIME) {
                    for (int i = 0; i < powerCellLitBar; i++) {
                        pcellLights.setPixelColor(i, Adafruit_NeoPixel::gamma32(colourMultiply(pCellColour, 1 - (float)timeSinceVent / VENT_LIGHT_FADE_TIME)));
                    }
                }
                else {
                    pcellLights.clear();
                }
                
                lastPowerCellChange = currentMillis;
                pcellLights.show();
            }
            break;
                    
        case MUSIC_MODE: {
                if (audioPeak.available()) {
                    float peak = min(audioPeak.read(), 1);
                    
                    powerCellLitBar = floor(peakMultiplier * peak * PCELL_LED_COUNT);
                    pcellLights.clear();

                    for (int i = 0; i < powerCellLitBar; i++) {
                        pcellLights.setPixelColor(i, 
                            Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(
                                0,
                                255 * pow(((float)(PCELL_LED_COUNT - i)/ PCELL_LED_COUNT),1),
                                75 * pow(((float)(i) / PCELL_LED_COUNT),0.5)
                            )));
                    }

                    if ((powerCellLitBar < PCELL_LED_COUNT)/* && (powerCellLitIndex > 0)*/) {
                        float remainder = 0.01 * (((int)(peakMultiplier * peak * PCELL_LED_COUNT * 100.0)) % 100);
                        
                        pcellLights.setPixelColor(powerCellLitBar - 1,
                            Adafruit_NeoPixel::gamma32(colourMultiply(Adafruit_NeoPixel::Color(
                                0,
                                255 * pow((PCELL_LED_COUNT - powerCellLitBar) / PCELL_LED_COUNT, 1),
                                75 * pow((powerCellLitBar / PCELL_LED_COUNT), 0.5)
                            ), remainder)));
                    }
                    pcellLights.show();
                    lastPowerCellChange = currentMillis;
                }
            }
	        break;

        default:
	        break;
    }
}

void pcellLightTo(int lightBar) {
    pcellLightTo(lightBar, true);
}

void pcellLightTo(int lightBar, bool show) {
    pcellLights.clear();
    for (int i = 0; i < lightBar; i++) {
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
            float cycloIntensity = min(1, ((float) (max(stateLastChanged, currentMillis) - stateLastChanged)) / BOOTING_TIME);
            for (int i = 0; i < CYCLO_LED_COUNT; i++) {                
                cycloLights.setPixelColor(i, Adafruit_NeoPixel::gamma32(colourMultiply(cyclotronFull, cycloIntensity)));
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
                        Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(
                            max(0, Red(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange))),
                            max(0, Green(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange))),
                            max(0, Blue(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange))),
                            max(0, White(savedCyclotronValues[i]) - ((int)(cycloSpinDecayRate * timeSinceLastChange)))
                        ))
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
                    Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::Color(
                        max(0,Red(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown))),
                        max(0, Green(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown))),
                        max(0, Blue(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown))),
                        max(0, White(savedCyclotronValues[i]) - ((int)(powerDownCycloDecayRate * timeSinceShutdown)))
                    ))
                );
                lastCycloChange[i] = currentMillis;
            }
            cycloLights.show();
        }
        break;

    case VENTING: {
            unsigned long timeSinceVent = max(currentMillis, stateLastChanged) - stateLastChanged;
            if (timeSinceVent < VENT_LIGHT_FADE_TIME) {
                for (int i = 0; i < CYCLO_LED_COUNT; i++) {
                    cycloLights.setPixelColor(i, Adafruit_NeoPixel::gamma32(colourMultiply(cyclotronOverheat, 1 - (float)timeSinceVent / VENT_LIGHT_FADE_TIME)));
                }
            }
            else {
                cycloLights.clear();
            }

            for (int i = 0; i < CYCLO_LED_COUNT; i++) {
                lastCycloChange[i] = currentMillis;
            }
            cycloLights.show();
        }
        break;
            
    case MUSIC_MODE: {
            if (audioFFT.available() && audioRMS.available()) {
                double band[4] = {
                audioFFT.read(1, 3),
                audioFFT.read(4, 12),
                audioFFT.read(13, 40),
                audioFFT.read(41, 127)
                };

                // Do some cleanup and transformation on the fft readings to both
                // limit low readings and then scale up a bit
                for (int i = 0; i < 4; i++) {
                    if (band[i] < fftThreshold) {
                        band[i] = 0;
                    }
                    else {
                        band[i] = min(1, fftMultiplier * pow(band[i], fftExponent));
                    }
                }
                
                // Tweak the RMS value to skew higher
                double rms = min(1, pow(audioRMS.read() * rmsMultiplier, rmsExponent));

                //DEBUG_SERIAL.printf("Bands: %0.2f,%0.2f,%0.2f,%0.2f RMS: %0.2f\n", band[0], band[1], band[2], band[3], rms);

                // Green, Yellow, Red Based on RMS
                /*for (int i = 0; i < 4; i++) {
                    cycloLights.setPixelColor(i,
                        Adafruit_NeoPixel::gamma32(colourMultiply(Adafruit_NeoPixel::Color(
                            255 * rms,
                            255 * (1 - rms),
                            0,
                            0
                        ), band[i])));
                }*/

                // Hue based on RMS, Saturation = 255, Brightness as Band
                for (int i = 0; i < 4; i++) {
                    cycloLights.setPixelColor(i,
                        Adafruit_NeoPixel::ColorHSV(
                            rms * 65535,
                            255,
                            band[i] * 255));
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
    //unsigned long currentMillis = millis();

    switch (state) {
    case VENTING:
        // TODO: Future venting with smoke
        break;

    default:
        break;
    }
}
