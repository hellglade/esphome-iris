#include "my_custom_transmitter.h"
#include "esphome/core/log.h"
#include <Arduino.h>

namespace esphome {
namespace my_custom_transmitter {

static const char *TAG = "my_custom_transmitter";

void MyCustomTransmitter::setup() {
  if (pin_ != 0) {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    ESP_LOGCONFIG(TAG, "Transmitter setup on pin %d", pin_);
  }
}

void MyCustomTransmitter::loop() {
  // Nothing needed here
}

void MyCustomTransmitter::send_raw_data(const std::vector<int> &data) {
  if (pin_ == 0) {
    ESP_LOGE(TAG, "Pin not set!");
    return;
  }

  ESP_LOGD(TAG, "Sending raw data of length %d", static_cast<int>(data.size()));

  for (auto pulse : data) {
    bool level = pulse > 0;
    digitalWrite(pin_, level ? HIGH : LOW);
    delayMicroseconds(abs(pulse));
  }

  // Ensure pin low at end
  digitalWrite(pin_, LOW);
  ESP_LOGD(TAG, "Transmission complete");
}

}  // namespace my_custom_transmitter
}  // namespace esphome
