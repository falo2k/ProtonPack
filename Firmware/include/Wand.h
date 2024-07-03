#include <Arduino.h>

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

#include <ProtonPackCommon.h>
#include <HardwareConfig.h>

void initialiseState(State newState, unsigned long currentMillis);

void stateUpdate();

void stateUpdate(unsigned long currentMillis);

void setSDTrack(unsigned long currentMillis, int newTrackNumber);
void setVolume(unsigned long currentMillis, int newVolume);
void attachCmdMessengerCallbacks();
void onUnknownCommand();
void onPackIonSwitch();
void onPackEncoderButton();
void onPackEncoderTurn();
void onPackSetTrack();
void onPackSetVolume();
void onDisplayVolume();
void onUpdateMusicPlayingState();
void onPackConnect();
void onSendConfigToWand();
void checkHall();
void setBGLamp(unsigned long currentMillis, int bar);
void setBGLamp(unsigned long currentMillis, int bar, bool state);
void setBGLampRange(unsigned long currentMillis, int from, int to);
void setBGLampRange(unsigned long currentMillis, int from, int to, bool state);
void clearBargraph(unsigned long currentMillis);
void checkDisplayTimeout(unsigned long currentMillis);

void displayBoot(unsigned long currentMillis);
void updatePackSettings(unsigned long currentMillis);

void bodyInit(unsigned long currentMillis);

void bodyUpdate();

void barrelInit(unsigned long currentMillis);

void barrelUpdate();

void tipInit(unsigned long currentMillis);

void tipUpdate();

void graphInit(unsigned long currentMillis);

void graphUpdate(unsigned long currentMillis);

void drawDisplay(unsigned long currentMillis);

void setupInputs();

void updateInputs(unsigned long currentMillis);

void actSwitchToggle(void * ref);

void lowerSwitchToggle(void * ref);

void upperSwitchToggle(void * ref);

void intButtonPress(void * ref);

void intButtonLongPress(void * ref);

void intButtonRelease(void * ref);

void tipButtonPress(void * ref);

void tipButtonLongPress(void * ref);

void tipButtonRelease(void * ref);

void rotaryButtonPress(void * ref);

void rotaryButtonLongPress(void * ref);

void rotaryButtonRelease(void * ref);

void rotaryMove(unsigned long currentMillis, int movement);

void setStartupState(unsigned long currentMillis);
