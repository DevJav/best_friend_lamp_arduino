
#include "utils/definitions.h"
#include "utils/utils.h"

#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

State current_state = State::IDLE;
NeoPixelBrightnessBus < NeoGrbFeature, NeoEsp8266Dma800KbpsMethod > strip(N_LEDS, LEDS_PIN);

WiFiClient espClient;
PubSubClient client(espClient);
String topic_to_publish = "javi/test/lamp/1";
String topic_to_subscribe = "javi/test/lamp/2";
void setup() 
{

    Serial.begin(9600);
    Serial.println("Starting...");

    pinMode(BUTTON_PIN, INPUT);

    // TEMPORAL
    wificonfig();

    client.setServer("broker.hivemq.com", 1883);
    client.setCallback(mqttCallback);
    
    while (!client.connected()) {
    if (client.connect("ESP8266Client326342541", "mqttUser", "mqttPassword")) {
      Serial.println("Conexión MQTT establecida");
      // Suscripción a un topic
      client.subscribe(topic_to_subscribe.c_str());
    } else {
      Serial.print("Error de conexión MQTT. Código de estado: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  
    strip.Begin();
    strip.Show();
    turnOffLeds();
    Serial.println("Ready!");
}

uint32_t start_time = 0;
uint8_t color_index = 0;

RgbColor current_color = BLACK;

String last_message = "";

void loop()
{
    switch(current_state)
    {
        case State::IDLE:
        {
            if (digitalRead(BUTTON_PIN) == HIGH)
            {
                current_state = State::COLOR_SELECTION;
                turnOnLedsHalfIntensity(RED);
                waitForRelease();
                start_time = millis();
                Serial.println("Color selection");
            }
            else if (isMessageReceived())
            {
                current_state = State::MSG_RECEIVED;
                start_time = millis();
                Serial.println("Message received");
            }
            break;
        }
        case State::COLOR_SELECTION:
        {
            if (digitalRead(BUTTON_PIN) == HIGH)
            {
                Serial.println("Color changed");
                color_index = (color_index + 1) % N_COLORS;
                turnOnLedsHalfIntensity(COLORS[color_index]);
                waitForRelease();
                start_time = millis();
            }

            if (millis() - start_time < COLOR_SELECTION_TIME_MS)
            {
                break;
            }

            if (COLORS[color_index] == BLACK)
            {
                current_state = State::IDLE;
            }
            else
            {
                sendColor(color_index);
                current_state = State::WAITING_RESPONSE;
                start_time = millis();
            }
            break;
        }
        case State::WAITING_RESPONSE:
        {
            // breath effect
            if (isResponseReceived())
            {
                Serial.println("Response received");
                current_state = State::TURNED_ON;
                start_time = millis();
                break;
            }
            else if (millis() - start_time > WAITING_TIME_MS)
            {
                turnOffLeds();
                current_state = State::IDLE;
            }
            break;
        }
        case State::TURNED_ON:
        {
            turnOnLedsFullIntensity(current_color);
            if (millis() - start_time > TURNED_ON_TIME_MS)
            {
                turnOffLeds();
                current_state = State::IDLE;
            }
            break;
        }
        case State::MSG_RECEIVED:
        {
            // breath effect
            if (current_color == BLACK)
            {
                turnOnLedsHalfIntensity(getColorFromMessage());
            }
            if (digitalRead(BUTTON_PIN) == HIGH)
            {
                current_state = State::TURNED_ON;
                waitForRelease();
                sendResponse();
                start_time = millis();
            }
            else if (millis() - start_time > WAITING_TIME_MS)
            {
                turnOffLeds();
                current_state = State::IDLE;
            }
        }
    }
    client.loop();
}

void turnOnLedsFullIntensity(RgbColor color)
{
    strip.SetBrightness(MAX_BRIGHTNESS);
    for (int i = 0; i < N_LEDS; i++)
    {
        strip.SetPixelColor(i, color);
        current_color = color;
    }
    strip.Show();
}

void turnOffLeds()
{
    strip.SetBrightness(0);
    for (int i = 0; i < N_LEDS; i++)
    {
        strip.SetPixelColor(i, BLACK);
        current_color = BLACK;
    }
    strip.Show();
}

void turnOnLedsHalfIntensity(RgbColor color)
{
    strip.SetBrightness(MAX_BRIGHTNESS / 2);
    for (int i = 0; i < N_LEDS; i++)
    {
        strip.SetPixelColor(i, color);
        current_color = color;
    }
    strip.Show();
}

bool isMessageReceived()
{
    return ((last_message != "") && isNumeric(last_message) && (last_message.toInt() < (N_COLORS - 1)));
}

RgbColor getColorFromMessage()
{
    if (last_message == "")
    {
        return BLACK;
    }
    uint8_t color_index = last_message.toInt();
    last_message = "";
    return COLORS[color_index];
}

bool isResponseReceived()
{
    if (last_message == "ok")
    {
        last_message = "";
        return true;
    }
    return false;
}

void sendColor(uint8_t selected_color)
{
    char msg[3];
    sprintf(msg, "%d", selected_color);
    client.publish(topic_to_publish.c_str(), msg);
}

void sendResponse()
{
    client.publish(topic_to_publish.c_str(), "ok");
}

void mqttCallback(char * topic, byte * payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    last_message = "";
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
        last_message.concat((char)payload[i]);
    }
    Serial.println();
}

void waitForRelease()
{
    while (digitalRead(BUTTON_PIN) == HIGH){};
}

// TEMPORAL
void configModeCallback(WiFiManager * myWiFiManager) {
    Serial.println("Entered config mode");
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

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}