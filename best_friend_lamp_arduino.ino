
#include <EEPROM.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#include "utils/definitions.h"
#include "utils/utils.h"
#include "utils/leds_control.h"

State current_state = State::IDLE;
NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>strip(N_LEDS, LEDS_PIN);

WiFiClient espClient;
PubSubClient client(espClient);
String topic_to_publish = "";
String topic_to_subscribe = "";
String last_message = "";

uint32_t start_time = 0;
uint8_t color_index = 0;
uint8_t walk_iterator = 0;

RgbColor current_color = BLACK;

void setup() 
{
    Serial.begin(9600);
    Serial.println("Starting...");

    pinMode(BUTTON_PIN, INPUT);

    strip.Begin();

    turnOnLedsAlternating();

    configureWifi();
    configureMqtt();

    current_color = WHITE;
    performDoubleFlashAnimation();

    turnOffLeds();
    Serial.println("Ready!");
}

bool isButtonPressed()
{
    return digitalRead(BUTTON_PIN) == HIGH;
}

void loop()
{
    switch(current_state)
    {
        case State::IDLE:
        {
            if (isButtonPressed())
            {
                current_state = State::COLOR_SELECTION;
                color_index = 0;
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
            if (isButtonPressed())
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
                Serial.println("No color selected");
                current_state = State::IDLE;
            }
            else
            {
                Serial.println("Sending color");
                performDoubleFlashAnimation();
                sendColor(color_index);
                current_state = State::WAITING_RESPONSE;
                start_time = millis();
            }
            break;
        }
        case State::WAITING_RESPONSE:
        {
            performBreatheAnimation(false);
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
            performBreatheAnimation(true);
            if (current_color == BLACK)
            {
                current_color = getColorFromMessage();
            }
            if (isButtonPressed())
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
    updateAllLeds(strip, color, MAX_BRIGHTNESS);
    current_color = color;
}

void turnOnLedsHalfIntensity(RgbColor color)
{
    updateAllLeds(strip, color, MAX_BRIGHTNESS / 2);
    current_color = color;
}

void turnOffLeds()
{
    updateAllLeds(strip, BLACK, 0);
    current_color = BLACK;
}

void turnOnLedsRainbow()
{
    for (int i = 0; i < N_LEDS; i++)
    {
        updateNthLed(strip, COLORS[i % N_COLORS], MAX_BRIGHTNESS, i);
    }
}

void turnOnLedsAlternating()
{
    for (int i = 0; i < N_LEDS; i++)
    {
        updateNthLed(strip, COLORS[i % 2], MAX_BRIGHTNESS, i);
    }
}

void performBreatheAnimation(bool start_from_zero)
{
    float intensity;
    if (start_from_zero)
    {
        intensity = MAX_BRIGHTNESS / 2.0 * (1 + sin(millis() / BREATHE_SPEED_FACTOR));
    }
    else
    {
        intensity = MAX_BRIGHTNESS / 2.0 * (1 + cos(millis() / BREATHE_SPEED_FACTOR));
    }
    updateAllLeds(strip, current_color, intensity);
}

void performDoubleFlashAnimation() 
{
    updateAllLeds(strip, current_color, MAX_BRIGHTNESS);
    delay(FLASH_TIME_MS);
    updateAllLeds(strip, current_color, MAX_BRIGHTNESS / 2);
    delay(FLASH_TIME_MS / 2);
    updateAllLeds(strip, current_color, MAX_BRIGHTNESS);
    delay(FLASH_TIME_MS);
    updateAllLeds(strip, BLACK, MAX_BRIGHTNESS);
}

void performWalkAnimation()
{
    if (((millis() - start_time) % ANIMATION_STEP_TIME_MS) == 0)
    {
        updateAllLeds(strip, BLACK, 0);
        delay(100);
        updateNthLed(strip, current_color, MAX_BRIGHTNESS / 2, walk_iterator);
        updateNthLed(strip, current_color, MAX_BRIGHTNESS / 2, walk_iterator + (N_LEDS / 2));
        walk_iterator = (walk_iterator + 1) % (N_LEDS / 2);
    }
}

void performWaveAnimation() 
{
    updateAllLeds(strip, BLACK, 0);
    for (int i = 0; i < N_LEDS / 2; i++) {
        updateNthLed(strip, current_color, MAX_BRIGHTNESS, i);
        updateNthLed(strip, current_color, MAX_BRIGHTNESS, N_LEDS - i - 1);
        delay(ANIMATION_STEP_TIME_MS);
        updateNthLed(strip, BLACK, MAX_BRIGHTNESS, i);
        updateNthLed(strip, BLACK, MAX_BRIGHTNESS, N_LEDS - i - 1);
    }
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
    unsigned long start_time = millis();
    while (isButtonPressed())
    {
        if (millis() - start_time > RESET_CONFIGURATION_TIME_MS)
        {
            Serial.println("Resetting configuration");
            WiFiManager wifiManager;
            wifiManager.resetSettings();
            ESP.restart();
        }
      yield();
    };
}

void configureWifi()
{
    WiFi.mode(WIFI_STA);
    WiFiManager wifiManager;

    if (wifiManager.getWiFiIsSaved())
    {
        Serial.println("Saved wifi found");
        wifiManager.autoConnect();
        return;
    }

    std::vector<const char *> menu = {"wifi", "info"};
    wifiManager.setMenu(menu);
    wifiManager.setClass("invert");

    WiFiManagerParameter myNameParam("MyName", "Your Name", "", 8);
    WiFiManagerParameter friendParam("FriendName", "Your Friends Name", "", 8);
    WiFiManagerParameter groupParam("GroupName", "Group Name", "", 8);
    WiFiManagerParameter brokerParam("broker", "MQTT Broker", "", 32);
    WiFiManagerParameter portParam("port", "Port", "", 6);

    wifiManager.addParameter(&myNameParam);
    wifiManager.addParameter(&friendParam);
    wifiManager.addParameter(&groupParam);
    wifiManager.addParameter(&brokerParam);
    wifiManager.addParameter(&portParam);

    turnOnLedsRainbow();

    wifiManager.autoConnect("Lamp", "password");

    struct MqttConfiguration mqtt_config;
    setParameterValue(myNameParam, mqtt_config.thisLampName, "Lamp1", sizeof(mqtt_config.thisLampName));
    setParameterValue(friendParam, mqtt_config.friendLampName, "Lamp2", sizeof(mqtt_config.friendLampName));
    setParameterValue(groupParam, mqtt_config.groupName, "Group", sizeof(mqtt_config.groupName));
    setParameterValue(brokerParam, mqtt_config.broker, "broker.hivemq.com", sizeof(mqtt_config.broker));
    setParameterValue(portParam, mqtt_config.port, "1883", sizeof(mqtt_config.port));

    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, mqtt_config);
    EEPROM.commit();

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MQTT configuration:");
    Serial.println("This lamp name: ");
    Serial.println(mqtt_config.thisLampName);
    Serial.println("Friend lamp name: ");
    Serial.println(mqtt_config.friendLampName);
    Serial.println("Group name: ");
    Serial.println(mqtt_config.groupName);
    Serial.println("Broker: ");
    Serial.println(mqtt_config.broker);
    Serial.println("Port: ");
    Serial.println(mqtt_config.port);

    turnOffLeds();
}

void configureMqtt()
{
    struct MqttConfiguration mqtt_config;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, mqtt_config);
    EEPROM.end();

    Serial.println("MQTT configuration read:");
    Serial.println("This lamp name: ");
    Serial.println(mqtt_config.thisLampName);
    Serial.println("Friend lamp name: ");
    Serial.println(mqtt_config.friendLampName);
    Serial.println("Group name: ");
    Serial.println(mqtt_config.groupName);
    Serial.println("Broker: ");
    Serial.println(mqtt_config.broker);
    Serial.println("Port: ");
    Serial.println(mqtt_config.port);

    topic_to_subscribe = "devjav/projects/best_friend_lamp/" + String(mqtt_config.groupName) + "/" + String(mqtt_config.friendLampName);
    topic_to_publish = "devjav/projects/best_friend_lamp/" + String(mqtt_config.groupName) + "/" + String(mqtt_config.thisLampName);
    Serial.println("Topic to subscribe: ");
    Serial.println(topic_to_subscribe);
    Serial.println("Topic to publish: ");
    Serial.println(topic_to_publish);
    
    String user = "devjav_" + String(mqtt_config.thisLampName);

    client.setServer(mqtt_config.broker, atoi(mqtt_config.port));
    client.setCallback(mqttCallback);
    
    while (!client.connected()) 
    {
        if (client.connect(user.c_str()))
        {
            Serial.println("Connected to MQTT broker");
            client.subscribe(topic_to_subscribe.c_str());
        } 
        else 
        {
            Serial.print("failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }
  
}
