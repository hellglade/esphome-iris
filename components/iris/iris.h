#pragma once

#include "esphome/core/component.h"
#include "esphome/components/remote_receiver/remote_receiver.h"
#include "esphome/components/gpio/gpio.h"
#include <vector>

namespace esphome {
namespace iris {

enum IrisCommand : uint16_t {
    IRIS_POWER = 0x11,
    IRIS_BLUE = 0x21,
    IRIS_MAGENTA = 0x22,
    IRIS_RED = 0x23,
    IRIS_LIME = 0x24,
    IRIS_GREEN = 0x25,
    IRIS_AQUA = 0x26,
    IRIS_WHITE = 0x27,
    IRIS_MODE1 = 0x31,
    IRIS_MODE2 = 0x32,
    IRIS_MODE3 = 0x33,
    IRIS_MODE4 = 0x34,
    IRIS_BRIGHTNESS = 0x41,
};

enum IrisMode : uint16_t {
    IRIS_POOL = 0x01,
    IRIS_SPA = 0x02,
    IRIS_POOLSPA = 0x03,
};

class IrisComponent : public Component, public remote_base::RemoteReceiverListener {
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }

  void setup() override;
  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

  // Core command function
  void send_command(IrisCommand cmd, IrisMode mode);

  // Setters
  void set_rx(remote_base::RemoteReceiverComponent *rx) { this->rx_ = rx; }
  void set_address(uint16_t address) { this->address_ = address; }
  void set_tx_custom(esphome::gpio::GPIOPin *pin) { this->tx_pin_ = pin; }

 protected:
  remote_base::RemoteReceiverComponent *rx_{nullptr};
  gpio::GPIOPin *tx_pin_{nullptr};
  uint16_t address_{0};

  // Build pulse timings
  std::vector<int> build_frame(uint16_t address, IrisCommand cmd, IrisMode mode);
};

}  // namespace iris
}  // namespace esphome
