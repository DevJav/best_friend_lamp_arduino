// import the libraries

#include <NeoPixelBrightnessBus.h>
#include <WiFiManager.h>

#include "config.h"

#define N_LEDS  12
#define LED_PIN 3
#define BOT     4 // capacitive sensor pin

//---------------------------------------------
// LAMP ID
int lampID = 2;

NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(N_LEDS, LED_PIN);

// Adafruit inicialization
AdafruitIO_Feed* lamp = io.feed("Lampara"); // Change to your feed

enum States {
    WAIT,
    WAIT_RELEASE,
    SELECT_COLOR,
    PUBLISH_MSG,
    SET_TIMER,
    WAIT_ANSWER,
    ANSWER_RECEIVED,
    ON,
    RESET,
    MSG_RECEIVED,
    WAIT_CONFIRMATION,
    SEND_ANSWER
};

int recVal{0};
int sendVal{0};

const int max_intensity = 255; //  Max intensity

int selected_color = 0; //  Index for color vector

int i_breath;

char msg[50]; //  Custom messages for Adafruit IO

//  Color definitions
RgbColor red(max_intensity, 0, 0);
RgbColor green(0, max_intensity, 0);
RgbColor blue(0, 0, max_intensity);
RgbColor purple(200, 0, max_intensity);
RgbColor cian(0, max_intensity, max_intensity);
RgbColor yellow(max_intensity, 200, 0);
RgbColor white(max_intensity, max_intensity, max_intensity);
RgbColor pink(255, 20, 30);
RgbColor orange(max_intensity, 50, 0);
RgbColor black(0, 0, 0);

RgbColor colors[] = {red, orange, yellow, green, cian, blue, purple, pink, white, black};

int state = 0;

// Time vars
unsigned long RefMillis;
unsigned long ActMillis;
const int     send_selected_color_time = 4000;
const int     answer_time_out          = 900000;
const int     on_time                  = 900000;

// Disconection timeout
unsigned long       currentMillis;
unsigned long       previousMillis     = 0;
const unsigned long conection_time_out = 300000; // 5 minutos

// Long press detection
const int     long_press_time = 2000;
int           lastState       = LOW; // the previous state from the input pin
int           currentState;          // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup()
{
    // Start the serial monitor for debugging and status
    Serial.begin(115200);

    // Activate neopixels
    strip.Begin();
    strip.Show(); // Initialize all pixels to 'off'

    wificonfig();

    pinMode(BOT, INPUT);

    //  Set ID values
    if (lampID == 1) {
        recVal  = 20;
        sendVal = 10;
    } else if (lampID == 2) {
        recVal  = 10;
        sendVal = 20;
    }

    // start connecting to Adafruit IO
    Serial.printf("\nConnecting to Adafruit IO with User: %s Key: %s.\n", IO_USERNAME, IO_KEY);
    AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, "", "");
    io.connect();

    lamp->onMessage(handle_message);

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
    turn_off();
    delay(100);
    flash(8);
    turn_off();
    delay(50);

    // get the status of our value in Adafruit IO
    lamp->get();
    sprintf(msg, "L%d: connected", lampID);
    lamp->save(msg);
}

void loop()
{
    currentMillis = millis();
    io.run();
    // State machine
    switch (state) {
        case WAIT:
            currentState = digitalRead(BOT);
            if (lastState == LOW && currentState == HIGH) // Button is pressed
            {
                pressedTime = millis();
            } else if (currentState == HIGH) {
                releasedTime       = millis();
                long pressDuration = releasedTime - pressedTime;
                if (pressDuration > long_press_time) {
                    state = WAIT_RELEASE;
                }
            }
            lastState = currentState;
            break;

        case WAIT_RELEASE:
            selected_color = 0;
            light_half_intensity(selected_color);
            state     = SELECT_COLOR;
            RefMillis = millis();
            while (digitalRead(BOT) == HIGH) {}
            break;

        case SELECT_COLOR:
            if (digitalRead(BOT) == HIGH) {
                selected_color++;
                if (selected_color > 9)
                    selected_color = 0;
                while (digitalRead(BOT) == HIGH) {
                    delay(50);
                }
                light_half_intensity(selected_color);
                // Reset timer each time it is touched
                RefMillis = millis();
            }
            // If a color is selected more than a time, it is interpreted as the one
            // selected
            ActMillis = millis();
            if (ActMillis - RefMillis > send_selected_color_time) {
                if (selected_color == 9) //  Cancel msg
                    state = RESET;
                else
                    state = PUBLISH_MSG;
            }
            break;

        case PUBLISH_MSG:
            sprintf(msg, "L%d: color send", lampID);
            lamp->save(msg);
            lamp->save(selected_color + sendVal);
            Serial.print(selected_color + sendVal);
            state = SET_TIMER;
            flash(selected_color);
            light_half_intensity(selected_color);
            delay(100);
            flash(selected_color);
            light_half_intensity(selected_color);
            break;

        case SET_TIMER:
            RefMillis = millis();
            state     = WAIT_ANSWER;
            i_breath  = 0;
            break;

        case WAIT_ANSWER:
            for (i_breath = 0; i_breath <= 314; i_breath++) {
                breath(selected_color, i_breath);
                ActMillis = millis();
                if (ActMillis - RefMillis > answer_time_out) {
                    turn_off();
                    lamp->save("L%d: Answer time out", lampID);
                    lamp->save(0);
                    state = RESET;
                    break;
                }
            }
            break;

        case ANSWER_RECEIVED:
            Serial.println("Answer received");
            light_full_intensity(selected_color);
            RefMillis = millis();
            sprintf(msg, "L%d: connected", lampID);
            lamp->save(msg);
            lamp->save(0);
            state = ON;
            break;

        case ON:
            ActMillis = millis();
            //  Send pulse
            if (digitalRead(BOT) == HIGH) {
                lamp->save(420 + sendVal);
                pulse(selected_color);
            }
            if (ActMillis - RefMillis > on_time) {
                turn_off();
                lamp->save(0);
                state = RESET;
            }
            break;

        case RESET:
            turn_off();
            state = WAIT;
            break;

        case MSG_RECEIVED:
            sprintf(msg, "L%d: msg received", lampID);
            lamp->save(msg);
            RefMillis = millis();
            state     = WAIT_CONFIRMATION;
            break;

        case WAIT_CONFIRMATION:
            for (i_breath = 236; i_breath <= 549; i_breath++) {
                breath(selected_color, i_breath);
                if (digitalRead(BOT) == HIGH) {
                    state = SEND_ANSWER;
                    break;
                }
                ActMillis = millis();
                if (ActMillis - RefMillis > answer_time_out) {
                    turn_off();
                    sprintf(msg, "L%d: answer time out", lampID);
                    lamp->save(msg);
                    state = RESET;
                    break;
                }
            }
            break;

        case SEND_ANSWER:
            light_full_intensity(selected_color);
            RefMillis = millis();
            sprintf(msg, "L%d: answer sent", lampID);
            lamp->save(msg);
            lamp->save(1);
            state = ON;
            break;

        default: state = WAIT; break;
    }
    if ((currentMillis - previousMillis >= conection_time_out)) {
        if (WiFi.status() != WL_CONNECTED)
            ESP.restart();
        previousMillis = currentMillis;
    }
}

// code that tells the ESP8266 what to do when it recieves new data from the
// Adafruit IO feed
void handle_message(AdafruitIO_Data* data)
{
    // convert the recieved data to an INT
    int reading = data->toInt();
    if (reading == 66) {
        sprintf(msg, "L%d: rebooting", lampID);
        lamp->save(msg);
        lamp->save(0);
        ESP.restart();
    } else if (reading == 100) {
        sprintf(msg, "L%d: ping", lampID);
        lamp->save(msg);
        lamp->save(0);
    } else if (reading == 420 + recVal) {
        sprintf(msg, "L%d: pulse received", lampID);
        lamp->save(msg);
        lamp->save(0);
        pulse(selected_color);
    } else if (reading != 0 && reading / 10 != lampID) {
        // Is it a color msg?
        if (state == WAIT && reading != 1) {
            state          = MSG_RECEIVED;
            selected_color = reading - recVal;
        }
        // Is it an answer?
        if (state == WAIT_ANSWER && reading == 1)
            state = ANSWER_RECEIVED;
    }
}

void turn_off()
{
    strip.SetBrightness(max_intensity);
    for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, black);
    }
    strip.Show();
}

// 50% intensity
void light_half_intensity(int ind)
{
    strip.SetBrightness(max_intensity / 2);
    for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
    }
    strip.Show();
}

// 100% intensity
void light_full_intensity(int ind)
{
    strip.SetBrightness(max_intensity);
    for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
    }
    strip.Show();
}

void pulse(int ind)
{
    int i;
    int i_step = 5;
    for (i = max_intensity; i > 80; i -= i_step) {
        strip.SetBrightness(i);
        for (int i = 0; i < N_LEDS; i++) {
            strip.SetPixelColor(i, colors[ind]);
            strip.Show();
            delay(1);
        }
    }
    delay(20);
    for (i = 80; i < max_intensity; i += i_step) {
        strip.SetBrightness(i);
        for (int i = 0; i < N_LEDS; i++) {
            strip.SetPixelColor(i, colors[ind]);
            strip.Show();
            delay(1);
        }
    }
}

// The code that creates the gradual color change animation in the Neopixels
// (thank you to Adafruit for this!!)
void spin(int ind)
{
    strip.SetBrightness(max_intensity);
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
void breath(int ind, int i)
{
    float MaximumBrightness = max_intensity / 2;
    float SpeedFactor       = 0.02;
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

// code to flash the Neopixels when a stable connection to Adafruit IO is made
void flash(int ind)
{
    strip.SetBrightness(max_intensity);
    for (int i = 0; i < N_LEDS; i++) {
        strip.SetPixelColor(i, colors[ind]);
    }
    strip.Show();

    delay(200);
}

// Waiting connection led setup
void wait_connection()
{
    strip.SetBrightness(max_intensity);
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

void configModeCallback(WiFiManager* myWiFiManager)
{
    Serial.println("Entered config mode");
    wait_connection();
}

void wificonfig()
{
    WiFi.mode(WIFI_STA);
    WiFiManager wifiManager;

    std::vector<const char*> menu = {"wifi", "info"};
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
        // if you get here you have connected to the WiFi
        spin(3);
        delay(50);
        turn_off();
    }
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}
