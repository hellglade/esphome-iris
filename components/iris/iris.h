#pragma once

#include "esphome/core/preferences.h"
#include "esphome/core/gpio.h"
#include "esphome/components/cc1101/cc1101.h"

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
  virtual void update_sunny(uint16_t address, bool value) {}
  virtual void update_windy(uint16_t address, bool value) {}
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
  void set_gdo0_pin(esphome::GPIOPin *pin) { gdo0_pin_ = pin; }
  void set_emitter_pin(esphome::GPIOPin *pin) { emitter_pin_ = pin; }
 protected:
  esphome::GPIOPin *gdo0_pin_{nullptr};
  esphome::GPIOPin *emitter_pin_{nullptr};
  ESPPreferenceObject preferences_;
  uint16_t address_{0};
  IrisCommand command_{IRIS_POWER};
  IrisMode mode_{IRIS_POOL};
  std::vector<IrisSensor *> sensors_;
};

}  // namespace iris
}  // namespace esphome
