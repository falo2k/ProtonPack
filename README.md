# Teensy Based Proton Pack Electronics
This is my implementation of the electronics for a Ghostbusters Proton Pack.  It uses a Teensy microcontroller in both parts of the kit (Pack and Wand) and communicates between them over 4 wires (power + serial).  For now I have kept it to a simple movie implementation with some additions - overheating and music playback.  I've added two features that are a bit unusual:
- An OLED display in the rear box on the wand is used for managing volume control and track selection, in conjunctions with a rotary encoder (push/turn) in the top knob.
- A bluetooth receiver can be turned on for playback of anything not available on the SD card.  The module I'm using will mix the stereo audio down to mono for a single pack, but comes as a pair for True Wireless Stereo.  The other board will be in a friend's pack so we can use the packs as a complete speaker set.

## Software
The code is set up as a VS solution/project set using [Visual Micro](https://www.visualmicro.com/), but you can tweak this to your preferred environment.  There are three core files:
- Wand.ino for the Wand sketch on the Teensy LC
- Pack.ino for the Pack sketch on the Teensy 4.0
- ProtonPackCommon.h which is used by both for some shared enums, timers, etc.

The Wand sketch does all state management, and will update the Pack sketch as necessary over serial.

You'll also need the libraries listed in the section below.

### Libraries
https://github.com/lipoyang/minIni4Arduino
https://github.com/lexus2k/lcdgfx/
https://github.com/thijse/Arduino-CmdMessenger
https://github.com/PaulStoffregen/Encoder
https://github.com/jonpearse/ht16k33-arduino
https://github.com/adafruit/Adafruit_NeoPixel
https://github.com/cygig/TimerEvent

### Pack Operation


### OLED Menu System
A long press on the Wand encoder will activate the menu (to avoid accidental bumps).  While active, a long press will also cancel the menu.  The menu has an automatic timeout so will not stay open indefinitely.

Currently there are two sections for control:
1. Volume: This changes the volume of the amplifier in the pack via i2c.
2. Track Change: This lets you select the extact track you want queued up for playback from the SD card (if enabled).  Selecting a new track while one is playing will cause the new track to start immediately.  Note that using the next/prev track shortcuts on the INT/TIP buttons will display the new track name in the display.

### Future Plans
- Some pin headers in the pack are spare to support adding a smoke machine to the pack at a later date for venting.  This should be easy to trigger in software.
- Alternative lighting schemes, sounds, animations, etc. can be added relatively easily.  Switching would probably be done with additional menu items through the OLED controls.
- I'd like to add more granular volume control, again through the menu, to individually tweak the volume of SFX, Music, and Bluetooth channels.

## Hardware
### BOM
[TBC] Equipment used and links to purchase

### PCBs
[TBC] - I have created a couple of PCBs to make installation neater.  I will share these here when I have manage to export them in

### Power
I've left supplying the 5V power to the boards to the individual user.  You can take a feed directly from a Talentcell 5V or use a common buck converter, but either of these may introduce some noise into the audio.  Your mileage with this may vary.

## Thanks
A huge thanks to everyone over on the [Arduino for Ghostbusters Props](https://www.facebook.com/groups/1187612118706042/) Facebook group.  Special shout outs to:
- Chris Rossi for steering me towards the Teensy in the first place
- Kevin Delcourt for his excellent github documentation.  Inspired me to take a different approach to fixing the top knob to the encoder!
- Quentin Machiels and Taco Belli over at [3D Printed Ghostbusters Props](https://www.facebook.com/groups/3DprintedGBprops/), without whom I would not have a pack to build this into
- [Mike Simone](https://github.com/MikeS11/ProtonPack) and [CountDeMonet](https://github.com/CountDeMonet/ArduinoProtonPack) for their implementations.  Both have steered hardware and software choices.