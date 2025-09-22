#pragma once

#include "esphome/core/preferences.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/remote_receiver/remote_receiver.h"

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

class IrisComponent : public Component, public remote_base::RemoteReceiverListener {
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void send_command(IrisCommand cmd, IrisMode mode, uint32_t repeat = 4);
  void set_tx(remote_transmitter::RemoteTransmitterComponent *tx) { this->tx_ = tx; }
  void set_rx(remote_receiver::RemoteReceiverComponent *rx) { this->rx_ = rx; }
  void set_address(uint16_t address) { this->address_ = address; }
  void set_command(IrisCommand command) { this->command_ = command; }
  void set_mode(IrisMode mode) { this->mode_ = mode; }
  void add_sensor(IrisSensor *sensor) { this->sensors_.push_back(sensor); }
  void set_repeat(uint32_t repeat) { this->repeat_ = repeat; }
 protected:
  remote_transmitter::RemoteTransmitterComponent *tx_{nullptr};
  remote_receiver::RemoteReceiverComponent *rx_{nullptr};
  ESPPreferenceObject preferences_;
  uint16_t address_{0};
  IrisCommand command_{IRIS_POWER};
  IrisMode mode_{IRIS_POOL};
  uint32_t repeat_{4};
  std::vector<IrisSensor *> sensors_;
};

}  // namespace iris
}  // namespace esphome

// test