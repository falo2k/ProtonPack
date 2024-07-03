#include <Arduino.h>
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

void readConfigFile(unsigned long currentMillis);
void writeConfigFile(unsigned long currentMillis);
void setupInputs();
void updateInputs(unsigned long currentMillis);
void ionSwitchToggle(void* ref);
void rotaryButtonPress(void* ref);
void rotaryButtonLongPress(void* ref);
void rotaryButtonRelease(void* ref);
void rotaryMove(unsigned long currentMillis, int movement);
void playRandomTrack();
boolean setvolume(int8_t v);
void toggleBluetoothModule(bool state);
void attachCmdMessengerCallbacks();
void onUnknownCommand();
void onSetVolume();
void onChangeState();
void onSetBluetoothMode();
void onSetSDTrack();
void onPlayPauseSDTrack();
void onStopSDTrack();
void onLoadConfig();
void onWriteConfig();
void onSendConfigToPack();
void onSerialLog();
void onWandConnect();
void initialiseState(State newState, unsigned long currentMillis);
void stateUpdate(unsigned long currentMillis);
void pcellInit();
void pcellUpdate();
void pcellLightTo(int lightBar);
void pcellLightTo(int lightBar, bool show);
void cycloInit();
void cycloUpdate();
void ventInit();
void ventUpdate();