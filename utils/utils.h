#include <WiFiManager.h>

bool isNumeric(const String& str) {
    for (size_t i = 0; i < str.length(); i++) {
        if (!isdigit(str.charAt(i))) {
            return false;
        }
    }
    return true;
}

void setParameterValue(const WiFiManagerParameter& param, char* target, const char* defaultValue, size_t maxSize) {
  const char* value = param.getValue();
  if (strlen(value) > 0 && strlen(value) < maxSize) {
    strcpy(target, value);
  } else {
    strcpy(target, defaultValue);
  }
}
