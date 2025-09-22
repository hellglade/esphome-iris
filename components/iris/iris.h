#pragma once

#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/components/remote_receiver/remote_receiver.h"
#include "my_custom_transmitter.h"   // <-- your custom transmitter

#include <vector>

namespace esphome {
namespace iris {

enum IrisCommand : uint16_t {
    IRIS_POWER     = 0x11,
    IRIS_BLUE      = 0x21,
    IRIS_MAGENTA   = 0x22,
    IRIS_RED       = 0x23,
    IRIS_LIME      = 0x24,
    IRIS_GREEN     = 0x25,
    IRIS_AQUA      = 0x26,
    IRIS_WHITE     = 0x27,
    IRIS_MODE1     = 0x31,
    IRIS_MODE2     = 0x32,
    IRIS_MODE3     = 0x33,
    IRIS_MODE4     = 0x34,
    IRIS_BRIGHTNESS= 0x41,
};

enum IrisMode : uint16_t {
    IRIS_POOL   = 0x01,
    IRIS_SPA    = 0x02,
    IRIS_POOLSPA= 0x03,
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

  void send_command(IrisCommand cmd, IrisMode mode);  // sends full frame

  void set_tx(MyCustomTransmitter* tx) { this->tx_ = tx; }
  void set_rx(remote_receiver::RemoteReceiverComponent *rx) { this->rx_ = rx; }
  void set_address(uint16_t address) { this->address_ = address; }
  void set_command(IrisCommand command) { this->command_ = command; }
  void set_mode(IrisMode mode) { this->mode_ = mode; }
  void add_sensor(IrisSensor *sensor) { this->sensors_.push_back(sensor); }
  void set_my_transmitter(my_custom_transmitter::MyCustomTransmitter *tx) { this->my_transmitter_ = tx; }

 protected:
  my_custom_transmitter::MyCustomTransmitter *my_transmitter_{nullptr};
  MyCustomTransmitter* tx_{nullptr};
  remote_receiver::RemoteReceiverComponent* rx_{nullptr};
  ESPPreferenceObject preferences_;
  uint16_t address_{0};
  IrisCommand command_{IRIS_POWER};
  IrisMode mode_{IRIS_POOL};
  std::vector<IrisSensor *> sensors_;
};

}  // namespace iris
}  // namespace esphome
