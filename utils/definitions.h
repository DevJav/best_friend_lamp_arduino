#include <NeoPixelBrightnessBus.h>

#define N_LEDS 8
#define LEDS_PIN 3
#define BUTTON_PIN 5

#define MAX_BRIGHTNESS 100.0
#define BREATHE_SPEED_FACTOR 1000.0

const RgbColor RED(MAX_BRIGHTNESS, 0, 0);
const RgbColor ORANGE(MAX_BRIGHTNESS, MAX_BRIGHTNESS/5, 0);
const RgbColor YELLOW(MAX_BRIGHTNESS, MAX_BRIGHTNESS * (4.0/5.0), 0);
const RgbColor GREEN(0, MAX_BRIGHTNESS, 0);
const RgbColor CIAN(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
const RgbColor BLUE(0, 0, MAX_BRIGHTNESS);
const RgbColor PURPLE(MAX_BRIGHTNESS * (4.0/5.0), 0, MAX_BRIGHTNESS);
const RgbColor PINK(MAX_BRIGHTNESS, MAX_BRIGHTNESS/11 , MAX_BRIGHTNESS/8);
const RgbColor WHITE(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
const RgbColor BLACK(0, 0, 0);

const RgbColor COLORS[] = {RED, ORANGE, YELLOW, GREEN, CIAN, BLUE, PURPLE, PINK, WHITE, BLACK};
const uint8_t N_COLORS = sizeof(COLORS) / sizeof(RgbColor);

enum class State {
  IDLE,
  COLOR_SELECTION,
  WAITING_RESPONSE,
  MSG_RECEIVED,
  SEND_RESPONSE,
  TURNED_ON
};

const int COLOR_SELECTION_TIME_MS = 4000;
const int WAITING_TIME_MS = 900000;
const int TURNED_ON_TIME_MS = 900000;
const int FLASH_TIME_MS = 200;
const int RESET_CONFIGURATION_TIME_MS = 8000;
const int ANIMATION_STEP_TIME_MS = 750;

struct MqttConfiguration {
  char thisLampName[8];
  char friendLampName[8];
  char groupName[8];
  char broker[32];
  char port[6];
};

const int EEPROM_SIZE = sizeof(MqttConfiguration);