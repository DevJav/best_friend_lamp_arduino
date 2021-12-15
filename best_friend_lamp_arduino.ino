//import the libraries

#include "config.h"
#include <NeoPixelBrightnessBus.h>
#include <WiFiManager.h>

#define N_LEDS 12
#define LED_PIN 3
#define BOT 4 // capacitive sensor pin

//////////////////LAMP ID////////////////////////////////////////////////////////////
int lampID = 1;
/////////////////////////////////////////////////////////////////////////////////////

NeoPixelBrightnessBus < NeoGrbFeature, NeoEsp8266Dma800KbpsMethod > strip(N_LEDS, LED_PIN);

// Adafruit inicialization
AdafruitIO_Feed * lamp = io.feed("Lampara"); // Change to your feed

int recVal  = 0;
int sendVal = 0;

int IntMax = 255; //  Max intensity

int SelectColor = 0;  //  Index for color vector

int i_breath;

char msg[50]; //  Custom messages for Adafruit IO

//  Color definitions
RgbColor red(IntMax, 0, 0);
RgbColor green(0, IntMax, 0);
RgbColor blue(0, 0, IntMax);
RgbColor purple(200, 0, IntMax);
RgbColor cian(0, IntMax, IntMax);
RgbColor yellow(IntMax, 200, 0);
RgbColor white(IntMax, IntMax, IntMax);
RgbColor pink(255, 20, 30);
RgbColor orange(IntMax, 50, 0);
RgbColor black(0, 0, 0);

RgbColor colors[] = {
  red,
  orange,
  yellow,
  green,
  cian,
  blue,
  purple,
  pink,
  white,
  black
};

int state = 0;

// Time vars
unsigned long RefMillis;
unsigned long ActMillis;
int send_selected_color_time = 4000;
int answer_time_out          = 900000; // 15 min
int on_time                  = 900000;

// Disconection timeout
unsigned long currentMillis;
unsigned long previousMillis = 0;
unsigned long conection_time_out = 300000; // 5 minutos

// Long press detection
long long_press_time = 2000;
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup() {
  //Start the serial monitor for debugging and status
  Serial.begin(115200);

  // Activate neopixels
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'

  wificonfig();

  pinMode(BOT, INPUT);

  //  Set ID values
  if (lampID == 1) {
    recVal = 20;
    sendVal = 10;
  }
  else if (lampID == 2) {
    recVal = 10;
    sendVal = 20;
  }

  //start connecting to Adafruit IO
  Serial.print("\nConnecting to Adafruit IO");
  io.connect();

  lamp -> onMessage(handleMessage);

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    spin(6);
    delay(500);
  }
  turn_off();
  Serial.println();
  Serial.println(io.statusText());
  // Animation
  spin(3);
  turn_off();
  delay(50);
  flash(8);
  delay(50);
  turn_off();
  delay(50);
  flash(8);
  delay(50);
  turn_off();
  //get the status of our value in Adafruit IO
  lamp -> get();
  sprintf(msg, "L%d: connected", lampID);
  lamp -> save(msg);
}

void loop() {
    currentMillis = millis();
    io.run();
    // State machine
    switch (state) {
      // Wait
    case 0:
      currentState = digitalRead(BOT);
      if(lastState == LOW && currentState == HIGH)  // Button is pressed
      {
        pressedTime = millis();
      }
      else if(currentState == HIGH) {
        releasedTime = millis();
        long pressDuration = releasedTime - pressedTime;
        if( pressDuration > long_press_time )
        {
            state = 1;
        }
      }
      lastState = currentState;
      break;
      // Wait for button release
    case 1:
      SelectColor = 0;
      Ilum50(SelectColor);
      state = 2;
      RefMillis = millis();
      while(digitalRead(BOT) == HIGH){}
      break;
      // Color selector
    case 2:
      if (digitalRead(BOT) == HIGH) {
        SelectColor++;
        if (SelectColor > 9)
          SelectColor = 0;
        while (digitalRead(BOT) == HIGH) {
          delay(50);
        }
        Ilum50(SelectColor);
        // Reset timer each time it is touched
        RefMillis = millis();
      }
      // If a color is selected more than a time, it is interpreted as the one selected
      ActMillis = millis();
      if (ActMillis - RefMillis > send_selected_color_time) {
        if (SelectColor == 9) //  Cancel msg
          state = 8;
        else
          state = 3;
      }
      break;
      // Publish msg
    case 3:
      sprintf(msg, "L%d: color send", lampID);
      lamp -> save(msg);
      lamp -> save(SelectColor + sendVal);
      Serial.print(SelectColor + sendVal);
      state = 4;
      flash(SelectColor);
      Ilum50(SelectColor);
      delay(100);
      flash(SelectColor);
      Ilum50(SelectColor);
      break;
      // Set timer
    case 4:
      RefMillis = millis();
      state = 5;
      i_breath = 0;
      break;
      // Wait for answer
    case 5:
      for (i_breath = 0; i_breath <= 314; i_breath++) {
        breath(SelectColor, i_breath);
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          turn_off();
          lamp -> save("L%d: Answer time out", lampID);
          lamp -> save(0);
          state = 8;
          break;
        }
      }
      break;
      // Answer received
    case 6:
      Serial.println("Answer received");
      Ilum100(SelectColor);
      RefMillis = millis();
      lamp -> save("L%d: Answer received", lampID);
      lamp -> save(0);
      state = 7;
      break;
      // Turned on
    case 7:
      ActMillis = millis();
      //  Send pulse
      if (digitalRead(BOT) == HIGH) {
        lamp -> save(420 + sendVal);
        pulse(SelectColor);
      }
      if (ActMillis - RefMillis > on_time) {
        turn_off();
        lamp -> save(0);
        state = 8;
      }
      break;
      // Reset before state 0
    case 8:
      turn_off();
      state = 0;
      break;
      // Msg received
    case 9:
      lamp -> save("L%d: msg received", lampID);
      RefMillis = millis();
      state = 10;
      break;
      // Send answer wait
    case 10:
      for (i_breath = 236; i_breath <= 549; i_breath++) {
        breath(SelectColor, i_breath);
        if (digitalRead(BOT) == HIGH) {
          state = 11;
          break;
        }
        ActMillis = millis();
        if (ActMillis - RefMillis > answer_time_out) {
          turn_off();
          lamp -> save("L%d: answer time out", lampID);
          state = 8;
          break;
        }
      }
      break;
      // Send answer
    case 11:
      Ilum100(SelectColor);
      RefMillis = millis();
      lamp -> save("L%d: answer sent", lampID);
      lamp -> save(1);
      state = 7;
      break;
    default:
        state = 0;
      break;
    }
    if ((currentMillis - previousMillis >= conection_time_out)) {
        if (WiFi.status() != WL_CONNECTED)
          ESP.restart();
        previousMillis = currentMillis;
      }
    }

    //code that tells the ESP8266 what to do when it recieves new data from the Adafruit IO feed
    void handleMessage(AdafruitIO_Data * data) {

      //convert the recieved data to an INT
      int reading = data -> toInt();
      if (reading == 66) {
        sprintf(msg, "L%d: rebooting", lampID);
        lamp -> save(msg);
        lamp -> save(0);
        ESP.restart();
      } else if (reading == 100) {
        sprintf(msg, "L%d: ping", lampID);
        lamp -> save(msg);
        lamp -> save(0);
      } else if (reading == 420 + recVal) {
        sprintf(msg, "L%d: pulse received", lampID);
        lamp -> save(msg);
        lamp -> save(0);
        pulse(SelectColor);
      } else if (reading != 0 && reading / 10 != lampID) {
        // Is it a color msg?
        if (state == 0 && reading != 1) {
          state = 9;
          SelectColor = reading - recVal;
        } 
        // Is it an answer?
        if (state == 5 && reading == 1)
          state = 6;
      }
    }

    void turn_off() {
      strip.SetBrightness(IntMax);
      for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, black);
      }
      strip.Show();
    }

    // 50% intensity
    void Ilum50(int ind) {
      strip.SetBrightness(IntMax / 2);
      for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
      }
      strip.Show();
    }

    // 100% intensity
    void Ilum100(int ind) {
      strip.SetBrightness(IntMax);
      for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
      }
      strip.Show();
    }

    void pulse(int ind) {
      int i;
      int i_step = 5;
      for (i = IntMax; i > 80; i -= i_step) {
        strip.SetBrightness(i);
        for (int i = 0; i < N_LEDS; i++) {
          strip.SetPixelColor(i, colors[ind]);
          strip.Show();
          delay(1);
        }
      }
      delay(20);
      for (i = 80; i < IntMax; i += i_step) {
        strip.SetBrightness(i);
        for (int i = 0; i < N_LEDS; i++) {
          strip.SetPixelColor(i, colors[ind]);
          strip.Show();
          delay(1);
        }
      }
    }

    //The code that creates the gradual color change animation in the Neopixels (thank you to Adafruit for this!!)
    void spin(int ind) {
      strip.SetBrightness(IntMax);
      for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
        strip.Show();
        delay(30);
      }
      for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, black);
        strip.Show();
        delay(30);
      }
    }

    // Inspired by Jason Yandell
    void breath(int ind, int i) {
      float MaximumBrightness = IntMax / 2;
      float SpeedFactor = 0.02;
      float intensity;
      if (state == 5)
        intensity = MaximumBrightness / 2.0 * (1 + cos(SpeedFactor * i));
      else
        intensity = MaximumBrightness / 2.0 * (1 + sin(SpeedFactor * i));
      strip.SetBrightness(intensity);
      for (int ledNumber = 0; ledNumber < N_LEDS; ledNumber++) {
        strip.SetPixelColor(ledNumber, colors[ind]);
        strip.Show();
        delay(1);
      }
    }

    //code to flash the Neopixels when a stable connection to Adafruit IO is made
    void flash(int ind) {
      strip.SetBrightness(IntMax);
      for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
      }
      strip.Show();

      delay(200);

    }

    // Waiting connection led setup 
    void wait_connection() {
      strip.SetBrightness(IntMax);
      for (int i = 0; i < 3; i++) {
        strip.SetPixelColor(i, yellow);
      }
      strip.Show();
      delay(50);
      for (int i = 3; i < 6; i++) {
        strip.SetPixelColor(i, red);
      }
      strip.Show();
      delay(50);
      for (int i = 6; i < 9; i++) {
        strip.SetPixelColor(i, blue);
      }
      strip.Show();
      delay(50);
      for (int i = 9; i < 12; i++) {
        strip.SetPixelColor(i, green);
      }
      strip.Show();
      delay(50);
    }

    void configModeCallback(WiFiManager * myWiFiManager) {
      Serial.println("Entered config mode");
      wait_connection();
    }

    void wificonfig() {
      WiFi.mode(WIFI_STA);
      WiFiManager wifiManager;
      
      std::vector<const char *> menu = {"wifi","info"};
      wifiManager.setMenu(menu);
      // set dark theme
      wifiManager.setClass("invert");
      
      bool res;
      wifiManager.setAPCallback(configModeCallback);
      res = wifiManager.autoConnect("Lamp", "password"); // password protected ap
    
      if (!res) {
        spin(0);
        delay(50);
        turn_off();
      } else {
        //if you get here you have connected to the WiFi    
        spin(3);
        delay(50);
        turn_off();
      }
      Serial.println("Ready");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
