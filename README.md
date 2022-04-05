# sdrppcontroller
## Arduino SDR++ Controller
### Arduino sketch and Linux plugin

[![VIDEO](https://img.youtube.com/vi/_txrEIK9pqs/0.jpg)](https://youtu.be/_txrEIK9pqs)
_Youtube video_

## Introduction
I was inspired by a question asked on RTL-SDR Facebook group. Although this question was 
about controlling SDR# with Arduino I decided to give a try with SDR++. I took a look at 
rig_ctrl plugin, but it was missing some gui functionalities. Also this plugin was 
designed to controll the software via network and its purpose in general was quite different 
from what I wanted to achieve. That's why I decided to try something by my own.
Parts of the code was taken from Botland store and DRFrobot wiki. Serial initialization was 
googled and found in some Stackoverlow answer (hopefully). I just glued those pieces toghether. 
Please note: I'm not a software developer and I'm not a very big fan of C++. Yet, I manged to 
develop something functional,so here it is: "Arduino SDR++ Controller"

## Environment
* [SDR++](https://github.com/AlexandreRouma/SDRPlusPlus) v1.0.6 
* [Ubuntu](https://ubuntu.com/) Linux 20.04
* [Arduino UNO](https://www.arduino.cc/en/main/arduinoBoardUno)
* [Arduino Leonardo](https://www.arduino.cc/en/main/arduinoBoardLeonardo) - preffered
* DFRobot DFR0009 [LCD Keypad Shield](https://wiki.dfrobot.com/LCD_KeyPad_Shield_For_Arduino_SKU__DFR0009)
* 2 * DFRobot [EC11 rotary encoders](https://wiki.dfrobot.com/EC11_Rotary_Encoder_Module_SKU__SEN0235)

## Building
In order to build the plugin please follow the instructions from SDR++ README file. Use Arduino IDE to compile the sketch and upload to the board. 
* _Arduino/controller_uno/controller_uno.ino_ - Arduino UNO code
* _Arduino/controller_leonardo/controller_leonardo.ino_ - Arduino Leonardo code
* _Plugin/misc_modules/arduino_controller/src/main.cpp_ - plugin code

## Wiring for Arduino UNO

Rotary encoders are connected to digitial pins:

### "Left" knob:
- Hole A -> Pin 13 - left turn
- Hole B -> Pin 12 - right turn
- Hole C -> Pin 11 - button

### "Right" knob:
- Hole A -> Pin 3 - left turn
- Hole B -> Pin 2 - right turn
- Hole C -> Unused - because the shield was out of digital pins 

## Wiring for Arduino Leonardo

Rotary encoders are connected to digitial pins:

### "Left" knob:
- Hole A -> Pin 2 - left turn
- Hole B -> Pin 3 - right turn
- Hole C -> Pin 11 - button

### "Right" knob:
- Hole A -> Pin 0 - left turn
- Hole B -> Pin 1 - right turn
- Hole C -> Unused (for now)


## Connecting to PC 
Connect Arduino with standard USB cable. It should be registered as a serial device, for example: `/dev/ttyACM0`. 

## LCD Keys mapping
- Left , Right - Waterfall Zoom In / Zoom Out
- Up , Down - Tune with a step equals to device sample rate.
- Select - Cycle demodulators. 
- Left knob - Tune with a step equals to "Snap interval" multiplied by 10.
- Right knob - Tune with a step equals to "Snap interval".
- Knob button - Center waterfall.

## Usage
Enable the plugin with "Module Manager". Setup serial port (default: _/dev/ttyACM0_) and click "Start" button.

## License 
SDR++ is GPL3.0 so if someone decides to include this plugin within SDR++ then the same 
license should be applied. Keep in mind that I used some code snippets from Botland store and DFRobot wiki.
I didn't notice any licenses there. So, to conclude: I really do not care. Take this code and Do whatever you want. 

## Where to buy? 
I bought everything at [Botland](https://www.botland.store/). 
- [Arduino UNO](https://botland.com.pl/arduino-seria-podstawowa-oryginalne-plytki/1060-arduino-uno-rev3-a000066-7630049200050.html)
- [Rotary encoder](https://botland.com.pl/enkodery/9533-czujnik-obrotu-impulsator-enkoder-obrotowy-dfrobot-ec11-5904422366674.html)
- [LCD Keypad shield](https://botland.com.pl/arduino-shield-klawiatury-i-wyswietlacze/2729-dfrobot-lcd-keypad-shield-v11-wyswietlacz-dla-arduino-5903351243780.html)
- [Case](https://botland.com.pl/obudowy-do-arduino/19020-obudowa-do-arduino-uno-z-lcd-keypad-shield-v11-czarno-przezroczysta-5904422362942.html)

## Final words
There is a lot of space for improments. For example I couldn't use interrupts for knobs because Arduino UNO has only two of them and I needed four. I read 
digital pins in a loop which is not the best practice I suppose. This issue doesn't affect Arduino Leonardo. Feel free to contribute. 
Greetings go to [RTL-SDR Polska](https://www.facebook.com/groups/2628926590655863) Facebook group and specially to Kacper for inspiring me!

