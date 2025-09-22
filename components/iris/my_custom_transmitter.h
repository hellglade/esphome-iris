#pragma once

#include "esphome/core/component.h"
#include <vector>

namespace esphome {
namespace my_custom_transmitter {

class MyCustomTransmitter : public Component {
 public:
  void setup() override;
  void loop() override;

  void set_pin(uint8_t pin) { this->pin_ = pin; pin_mode(pin_, OUTPUT); }
  void send_raw_data(const std::vector<int> &data);

 protected:
  uint8_t pin_{0};
};

}  // namespace my_custom_transmitter
}  // namespace esphome
