[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/V7V6KE7PQ)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/donate/?business=ESPELHR7Z3MMJ&no_recurring=0&item_name=Coffee+makes+the+world+go+round&currency_code=GBP)
# Teensy Based Proton Pack Electronics
This is my implementation of the electronics for a Ghostbusters Proton Pack.  It uses a Teensy microcontroller in both parts of the kit (Pack and Wand) and communicates between them over 4 wires (power + serial).  For now I have kept it to a simple movie implementation with some additions - overheating and music playback.  I've added two features that are a bit unusual:
- An OLED display in the rear box on the wand is used for managing volume control and track selection, in conjunctions with a rotary encoder (push/turn) in the top knob.
- A bluetooth receiver can be turned on for playback of anything not available on the SD card.  The module I'm using will mix the stereo audio down to mono for a single pack, but comes as a pair for True Wireless Stereo.  The other board will be in a friend's pack so we can use the packs as a complete speaker set.
  
It is designed to fit inside the 3D printed pack available [here - the Q-Pack](https://github.com/mr-kiou/q-pack).  It's been developed for a Mk3 pack - I will review it at some point in the future for Mk4 compatability.

*TBD Demo Video*

## Software
The code is set up as a VS solution/project set using [Visual Micro](https://www.visualmicro.com/), but you can tweak this to your preferred environment.  There are three core files:
- Wand.ino for the Wand sketch on the Teensy LC
- Pack.ino for the Pack sketch on the Teensy 4.0
- ProtonPackCommon.h which is used by both for some shared enums, timers, etc.
  
The Wand sketch does all state management, and will update the Pack sketch as necessary over serial.  If you want to tweak speeds, animations, direction of switches, etc. then you should find the right data to change either above the setup() functions in each file, or within the common header.
  
You'll need the libraries listed in the section below.

### Libraries
[minIni for Arduino](https://github.com/lipoyang/minIni4Arduino) - Config file saving/loading with the SD card  
[LCDGFX](https://github.com/lexus2k/lcdgfx/) - Drives the OLED display  
[CmdMessenger](https://github.com/thijse/Arduino-CmdMessenger) - Manages serial connection between both boards with callbacks  
[Encoder](https://github.com/PaulStoffregen/Encoder) - Converts rotary encoder inputs to movement  
[HT16K33](https://github.com/jonpearse/ht16k33-arduino) - Basic library for controlling the LED matrix backpack/bargraph.  
[NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) - The Adafruit NeoPixel library.  Considered using FastLED, but as I had GRBW LEDs in stock (and wanted them for some parts) I stuck with the Adafruit library.  
[TimerEvent](https://github.com/cygig/TimerEvent) - A library for managing timing of updates (helping to banish unnecessary delay() calls)  
[Switch](https://github.com/avandalen/avdweb_Switch) - Primarily used for the debouncing and callback features for switches.  Detects events for pressing, holding, and releasing.  

### Pack Operation
Once power is on to both boards, they will start up in the state determined by the current switch positions, so you can switch straight into music mode if you want.  If the pack is booted without a wand connected, it will enter bluetooth mode after 5 seconds.  

**Act Switch on Left Box**  
- This toggles the pack power for both movie and music mode.

**Lower Switch Next to Bargraph**  
- In movie mode this toggles whether or not the overheat warning will kick in after a period of time and then move to venting after further warning.  If off, firing will continue indefinitely.  
- In music mode, this is used to toggle the Bluetooth module on, which will then stop any playback from the SD card.

**Upper Switch Next to Bargraph**  
- This switches between movie mode (off) and music mode (on)

**Int Button on Left Box**  
- In movie mode, this will cause the pack to fire while held.  
- In music mode a single press will act as a play/pause button, while a long press will move to the next track in the list.

**Tip Button**  
- In movie mode, this will cause the pack to fire while held.  If single pressed, it will cause a venting routing to kick off.  
- In music mode a single press will act as a stop button, while a long press will move to the previous track in the list.

**Pack Crank Knob**  
- Connected to an encoder, this will currently just mimic the behaviour of the wand encoder (see the OLED Menu System below).

**Pack Ion Switch**  
- Currently unused.

### OLED Menu System
A long press on the Wand encoder will activate the menu (to avoid accidental bumps).  While active, a long press will also cancel the menu.  The menu has an automatic timeout so will not stay open indefinitely.  Single click to select / confirm menu items

Currently there are three features for control:  
1. **Volume**: This changes the volume of the amplifier in the pack via i2c.  This will update as you turn the knob.  
2. **Track Change**: This lets you select the extact track you want queued up for playback from the SD card (if enabled).  Single click to confirm the selection, or long press to cancel.  Selecting a new track while one is playing will cause the new track to start immediately.  Note that using the next/prev track shortcuts on the INT/TIP buttons will display the new track name in the display.  
3. **Load/Save Config**: This will load or save the volume and track configuration to a file on the SD card.  This is loaded as the default when the pack is first powered  

### Future Plans
- Some pin headers in the pack are already spare to support adding a smoke machine to the pack at a later date for venting (marked as IO2 on the PCB).  This should be easy to trigger in software.
- Alternative lighting schemes, sounds, animations, etc. can be added relatively easily.  Switching would probably be done with additional menu items through the OLED controls.
- I'd like to add more granular volume control, again through the menu, to individually tweak the volume of SFX, Music, and Bluetooth channels.
- The Ion Switch input currently doesn't do anything.  I may use this as either a hard or soft toggle for any future smoke effects.
- Check compatibility with Mk4 Q-Pack and generally review the PCBs for other options.  Now I've got my own wiring in, I might see some better positioning for connectors, consider breakout boards, etc.
- Consider either including an isolated converter in the BOM and as part of existing PCBs, or a breakout depending on power requirements for the smoke system.

## Hardware
### BOM
Where I can remember it, I've listed the Equipment used and links to purchase below.  You may find other sources are better for availability (Mouser, Digikey, etc.).  As with all of these types of projects, it depends exactly how you're planning to mount your electronics, so I'd advise thinking about your layout before purchasing.  I'm assuming I don't need to go into tools and wire here, that should be a given.  A decent soldering iron is a must, and I have to say how much I'm loving my [Pinecil](https://www.pine64.org/pinecil/), and also the best helping hands I've ever used, the [Omnifixo](https://omnifixo.com/en-gb).  

**Controllers**  
 1 x [Teensy LC](https://www.pjrc.com/store/teensylc.html).  Would be fine using a Teensy 4.0 here as well if availability is a challenge  
 1 x [Teensy 4.0](https://www.pjrc.com/store/teensy40.html)  
 1 x [Teensy Audio Board Rev D](https://www.pjrc.com/store/teensy3_audio.html)  
 1 x [Teensy Stacking Header Kit](https://www.adafruit.com/product/3883).  For the audio board to stack on the Teensy 4.0.  This was the easiest source of 14 pin stackable headers I could find.  If not using PCBs then you may just stick with non-stacking headers here.  
 4 x [14x1R Pins](https://www.pjrc.com/store/header_14x1.html) for the Teensy boards  
 4 x [14x1R Socket Headers](https://www.pjrc.com/store/socket_14x1.html) for the PCBs  
 1 x [5x2R Socket Header](https://www.amazon.co.uk/Double-Female-Straight-Header-Socket-Black/dp/B00R1LKZOM/ref=sr_1_5?crid=2BQW1XCGHFEJU&keywords=2r+5+header&qid=1682349085&sprefix=2r+5+he%2Caps%2C120&sr=8-5) for the PCB connection to the audio inputs/outputs on the board  
 1 x Micro SD Card for the Audio Adapter.  See the [Audio Adapter product page](https://www.pjrc.com/store/teensy3_audio.html#:~:text=video%20for%20details.-,Recommended%20SD%20Card,-Most%20SD%20cards) for up to date recommendations.  Not much is stored on here so you can get away with a fairly small card, but if you want to load more songs on YMMV.  

**Premade Boards**  
1 x [Adafruit 16x8 LED Matrix Driver Backpack - HT16K33 Breakout](https://www.adafruit.com/product/1427) for driving the bargraph.  
1 x [DFRobot Mosfet Controller DFR0457](https://wiki.dfrobot.com/Gravity__MOSFET_Power_Controller_SKU__DFR0457) to switch the bluetooth board  
1 x [Adafruit 20W Class D Amplifier](https://www.adafruit.com/product/1752)  
1 x [Bluetooth Audio Board](https://www.aliexpress.com/item/1005004753541997.html). Comes as a wireless stereo pair.  

**Buttons and Switches**  
3 x [2 Position MTS102 mini toggle switches](https://www.amazon.co.uk/gp/product/B01BWL7Z44/) for wand  
1 x [Black Off/On SPST Momentary switch](https://www.amazon.co.uk/gp/product/B00TXNXU4S)  
1 x [Red Off/On SPST Momentary switch](https://www.amazon.co.uk/gp/product/B008ZYE9LY)  
2 x [EC11 Rotary Encoder with Switch](https://www.ebay.co.uk/sch/i.html?_from=R40&_trksid=p2380057.m570.l1313&_nkw=ec11+rotary+encoder&_sacat=0) for the pack (crank) and wand (top / front knob).  
  
Ion switches can be picked up on Etsy.  
  
I also use some of [these](https://www.digikey.co.uk/en/products/detail/e-switch/100DP1T1B1M1QEH/378867) to switch on/off and choose charging vs electronics power for my internally mounted [Talentcell battery](https://talentcell.com/lithium-ion-battery/12v/yb1206000-usb.html).

**LEDs**  
You may want to do your own LEDs.  Numbers and type can be varied in the code, though I haven't allowed for cyclotron LEDs that are more than a single addressable LED in a chain.  You may want to use the 7 LED jewels, though I found they were unnecessary for brightness and just ramped up power consumption for the sake of it.  
1 x [144LEDs/m SK6812 RGBW Strip](https://www.aliexpress.com/item/32476317187.html?spm=a2g0o.order_list.order_list_main.5.74871802yO2AMH).  144 LEDs/m is just enough density to give you 15 LEDs in the Power Cell.  You can get a minimum length if you like.  
13+ x [SK6812 RGBW Chips](https://www.aliexpress.com/item/1005002509850925.html?spm=a2g0o.order_list.order_list_main.29.74871802yO2AMH) which I use in the wand, pack vent, and for the cyclotron.  Worth getting some spares in case you fry one.  
1 x [RGBW LED jewel](https://www.aliexpress.com/item/32825068416.html?spm=a2g0o.order_list.order_list_main.56.74871802yO2AMH) for the barrel light  
1 x [28 Segment Bar Graph](http://www.barmeter.com/download/bl28-3005sda04y.pdf).  Sourcing for this always ends up out of date, so hunt around Aliexpress or try asking on one of the FB groups.  Note that there are two versions of this bargraph, a common anode and common cathode version.  My code is designed for the BL28-3005SDK04Y model (note the A or K in the model name differentiating them).  It can be adapted for the other by simply changing the array mapping in the common header to point to the correct row/column references for your model.  

**Capacitors and Resistors**  
4 x 4.7K Ohm Resistors for pullups on the Teensy i2c lines  
6 x 220 Ohm Resistors for neopixel data inputs  
2 x 100 uF Capacitors for neopixel power inputs (these may not be necessary depending on the pixels you're using - I'm keeping them in as they don't harm)  

**Connectors (for PCB / wiring)**  
Source these where you like depending on how many you need / want for other projects.  I tend to buy bulk lots from AliExpress.  You can count up the exact number you need, but I always find I need more so I'll leave them open ended :)  
- [JST-XH connectors and headers](https://www.digikey.co.uk/en/product-highlight/j/jst/xh-series-connectors) for the boards and cables (2,3,4 pin)  
- [Screw terminal blocks](https://www.digikey.co.uk/en/products/detail/phoenix-contact/1729018/260604) (2 pin, 5mm pitch) 
- [Dupont connectors](https://www.amazon.co.uk/dupont-connectors/s?k=dupont+connectors)
- [JST-SM wire to wire connectors](https://www.amazon.co.uk/Aussel-Connector-Housing-Assortment-Kit-560PCS-2-5mm-JST-SM-Connector-560PCS/dp/B0716JMHNJ/ref=sr_1_5?crid=1V0JLS666ZR6J&keywords=jst+sm+kit&qid=1682347520&sprefix=jst+sm+kit%2Caps%2C67&sr=8-5) for  (handy locking wire to wire options)  
- [JST-PH connectors](https://www.amazon.co.uk/RUNCCI-YUN-540Pcs-Connector-Housing-Adapter/dp/B092LWQJHN/ref=sr_1_7?crid=V8F2IT2RMUFS&keywords=jst%2Bph%2Bkit&qid=1682347555&sprefix=jst%2Bph%2Bkit%2Caps%2C65&sr=8-7&th=1) (2,3 pin) for some of the smaller connectors on the DFRobot boards and the bluetooth board  
- An assortment of [Wago221s](https://www.amazon.co.uk/s?k=wago+221) because they're brilliant for patching cables together with quick release  
- [2 Pairs of 4 pin aviation connectors](https://www.amazon.co.uk/gp/product/B09WXZNKXN/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) which I use to carry the serial/power to and from the wand while allowing a quick release.

**Other**  
2 x [74AHCT125 Level Shifters](https://www.adafruit.com/product/1787) for driving the Neopixels as the Teensy uses 3.3V logic.  You may get away without these, but better safe than sorry.  
2 x [14 Pin IC Sockets](https://www.digikey.co.uk/en/products/detail/cnc-tech/245-14-1-03/3441580) for mounting the above into the PCBs (or you can just solder them in).  
1 x [0.91-inch OLED Display SSD1306](https://www.amazon.co.uk/gp/product/B07V8B3LSR)
1 x [M6 Thumb Nut](https://www.aliexpress.com/item/1005003320281166.html) for the top knob on the wand.  I drilled out a 2.5 mm hole in the side, tapped it for M3 and used a grub screw to attach it to an encoder.  The top is capped with a cut-off and glued countersunk bolt head.

### PCBs
I have created a couple of PCBs to make installation neater in a Q-Pack.  I've shared the production files in the PCBs folder for use with [JLCPCB](https://jlcpcb.com/).  I used Altium Circuitmaker to create these, so it's hard to share a useful copy of them.  I plan at some point to migrate these to KiCad but that requires time to learn KiCad.  It'll probably happen when I start working on a better belt gizmo :)  
  
The folder also contains STLs to mount the boards and keep the solder joins clear of the mounting surface.  For the Wand, it is just a spacer - the board is designed to fit along the handle side of the Mk3 Q-Pack, using the external M3 bolt to secure it with a nut internally.  This should position the serial/power connections for the handle exit, but leave enough space for an encoder to run through the top knob if desired.  For the pack, it's a backer designed to have M3x5x4 heat sets (standard size used in Vorons) added to the holes so the board can be attached and removed easily with M3 bolts.  The backer can then be secured in place to the motherboard with VHB tape.  It should be slim enough to fit underneath the booster box of the Mk3 Q-Pack.
  
*TBD Supporting Images*
  
*Revision 1.0:* Initial release of boards  
*Revision 1.1:* Pack board updated to move Audio Board output pin.  I made a mis-reading of the schematic, and only the pins nearest the Teensy pins are connected to L/R out.  Temporarily solder bridged on my v1.0 boards.  

### Power
I've left supplying the 5V power to the boards to the individual user.  You can take a feed directly from a Talentcell 5V output or use a common buck converter, but either of these may introduce some noise into the audio.  Your mileage with this may vary.  I got myself some [isolated dc-dc converters](https://www.digikey.co.uk/en/products/detail/mornsun-america-llc/VRB1205S-6WR3/16348304) to step down my 12V talentcell battery and avoid noise issues going to the amp.  6W is overspecced (in my testing, the setup draws at most 0.6A at its busiest time when in the overheat warning sequence).

### Changelog
1.0    Initial Release  

## Useful Models
I'm slowly going over my library of replacement parts and jigs that I've made over the course of this build and exporting them into the [STLs](./STLs/) folder.  Things like board mounts are generally reusable either with my PCBs or the off-the-shelf boards listed above.  Some bits I've made are very specific to my build so probably not of interest to others.  Have a play with Fusion 360 - you'll both love and hate yourself as a result :)  

If you see any others I'm using in photos and want to get hold of them, let me know.  

## Thanks
A huge thanks to everyone over on the [Arduino for Ghostbusters Props](https://www.facebook.com/groups/1187612118706042/) Facebook group.  Special shout outs to:  
- Chris Rossi for steering me towards the Teensy in the first place
- Kevin Delcourt for his excellent github documentation.  Inspired me to take a different approach to fixing the top knob to the encoder!
- Quentin Machiels and Taco Belli over at [3D Printed Ghostbusters Props](https://www.facebook.com/groups/3DprintedGBprops/), without whom I would not have a pack to build this into
- [Mike Simone](https://github.com/MikeS11/ProtonPack) and [CountDeMonet](https://github.com/CountDeMonet/ArduinoProtonPack) for their implementations.  Both have steered hardware and software choices.