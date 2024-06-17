Forked from DevJav/best_friend_lamp_arduino to attempt to run multiple lamps  ***Beta***


# Arduino Best Friend Lamps

![lamp-gif](doc/bflamp.gif)


This project objective is to make a cheap, open source version of what's commomnly called a best friend/friendship/long distance lamp,
like [this one](https://www.friendlamps.com/) or [this one](https://bit.ly/3GN3ZJa).

The idea is having two (or more) lamps connected so as when one of them is turned on, the others will turn on.

This project is a modification and ampliation of [CoronaLamps](https://www.instructables.com/CoronaLamps-Simple-Friendship-Lamps-Anyone-Can-Mak/),
so thanks to the authors for making it public.

## Material used (per lamp)
- Microcontroller esp8266
- NeoPixel Ring 12 LEDs
- TTP223 touch sensor
- [IKEA TOKABO lamp](https://www.ikea.com/es/es/p/tokabo-lampara-mesa-vidrio-blanco-opalo-40357998/)
- Male micro USB to male USB
- Welder, glue, wires...

These components could be easily replaced for por example another microcontroller like ESP32, different LED type (but directonal)
or even a different lamp.

All together looks like this:

<img src="doc/inside.jpeg" alt="drawing" width="500"/>

The touch sensor has a wire connected so it will cast less shadows inside the lamp, however this might be what makes the sensor trigger by itself sometimes :(

## Adafruit IO setup
[Adafruit IO](https://io.adafruit.com/) is a free use server that we will use to communicate the lamps.
You must create an account and create a new feed with whatever name you want. This created feed is where the messages from the 
lamps will be published.
## Code setup
First of all, we need to have [Arduino IDE](https://www.arduino.cc/en/software) installed and the ESP8266 configuration added.
You can follow [this tutorial](https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/) to achieve that. 

We also need to install some libraries:
- NeoPixelBus: to control de LED ring.
- WifiManager: used to create a WiFi connection to set the WiFi id and password.
- Adafruit IO Arduino: this library provides the tools for connecting to Adafruit IO. Once installed, we must make some changes in "AdafruitIO_ESP8266.cpp" in order to 
  be able to use with WifiManager. Those changes are provided by [wyojustin](https://github.com/wyojustin) in 
  [this comment](https://github.com/tzapu/WiFiManager/issues/243#issuecomment-364804188) (so thanks to him too).

#### Changes in best_friend_lamp_arduino.ino:
- Change the defines:
    ```
    #define N_LEDS  //  number of leds used
    #define LED_PIN //  pin connected to leds, must be an RX pin
    #define BOT     //  pin connected to capacitive sensor
    ```
- Change lamp id, one lamp must be se to 1 and the other to 2:
    ```
    int lampID = 1;
    ```
- Change adafruit feed name to the one you created before:
    ```
    AdafruitIO_Feed * lamp = io.feed("your_feed");
    ```

#### Changes in config.h:
Here we will add our login credentials to Adafruit IO. In the web page, go to 
"My Key" and copy the Arduino section. Then it should be paste on the config file:
    ```
    #define IO_USERNAME  "user"
    #define IO_KEY       "key"
    ```

After making this changes, the program can be uploaded to both lamps, just remember
to change the lampID for each one.

## User manual
The first time the lamp is connected, or if the Wifi has changed, the lamp will light
4 different colors, indicating that it is not connected to internet. The micro will
create a Wifi connection called "Lamp" and with password "password" (can be 
changed if you're afraid to be hacked). Connect to it (from your 
phone for example) and navigate to "192.168.4.1". From there, you can 
configure the connection.

Once the internet setup is done, it will connect to Adafruit IO. During this process
light will spin on green, and when its done, it will flash.

Now it's ready to use. To send a message, touch the lamp for 2 seconds and it will light
up on red. Touch again and again and the color will change. When you decide the one you
like, wait some time and after a flash, the message is sent. Now, both lamps will make a 
breath animation until the one who received the message is touched (considered an answer)
and will turn on at 100% for 15 minutes, or until 15 minutes have passed without response
and it will turn off.

## Known issues
- In my experience with TTP223 touch sensor, it may start tiggering randomly by itself. I haven't found a way to fix this apart from incresing the time needed to recognise the first touch.
