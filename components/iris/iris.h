#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include <vector>

namespace esphome {
namespace iris {

enum IrisCommand : uint16_t {
    IRIS_POWER = 0X11,
    IRIS_BLUE = 0X21,
    IRIS_MAGENTA = 0X22,
    IRIS_RED = 0X23,
    IRIS_LIME = 0X24,
    IRIS_GREEN = 0X25,
    IRIS_AQUA = 0X26,
    IRIS_WHITE = 0X27,
    IRIS_MODE1 = 0X31,
    IRIS_MODE2 = 0X32,
    IRIS_MODE3 = 0X33,
    IRIS_MODE4 = 0X34,
    IRIS_BRIGHTNESS = 0X41,
};

enum IrisMode : uint16_t {
    IRIS_POOL = 0X01,
    IRIS_SPA = 0X02,
    IRIS_POOLSPA = 0X03,
};

class IrisSensor {
 public:
};

class IrisComponent : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void dump_config() override;
  void send_command(IrisCommand cmd, IrisMode mode);
  void set_address(uint16_t address) { this->address_ = address; }
  void set_command(IrisCommand command) { this->command_ = command; }
  void set_mode(IrisMode mode) { this->mode_ = mode; }
  void add_sensor(IrisSensor *sensor) { this->sensors_.push_back(sensor); }
  void set_config_gdo0(InternalGPIOPin* pin);
  void set_config_emitter(InternalGPIOPin* pin);

 protected:
  InternalGPIOPin* gdo0_;
  InternalGPIOPin* emitter_;
  ESPPreferenceObject preferences_;
  uint16_t address_{0};
  IrisCommand command_{IRIS_POWER};
  IrisMode mode_{IRIS_POOL};
  std::vector<IrisSensor *> sensors_;
};

}  // namespace iris
}  // namespace esphome
