#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include "iris.h"

namespace esphome {
namespace iris {

static const char *const TAG = "iris";

// Symbol duration in microseconds (matches your -104/+105 timings)
static const int SYMBOL_HIGH = 105;
static const int SYMBOL_LOW = -104;

void IrisComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Iris:");
  ESP_LOGCONFIG(TAG, "  Address: %" PRIx16, this->address_);
}

void IrisComponent::setup() {
  ESP_LOGCONFIG(TAG, "Iris setup done");
  this->rx_->register_listener(this);
}

std::vector<int> IrisComponent::build_frame(uint16_t address_, IrisCommand command_, IrisMode mode_) {
  uint8_t frame[12] = {
      0xAA, 0xAA, 0xAA, 0xAA, // sync
      0x2D, 0xD4,             // sync
      0x00, 0x00,             // ID0, ID1 (filled below)
      0x00,                   // always 0x00
      0x00,                   // instruction (filled below)
      0x00,                   // mode (filled below)
      0x00                    // checksum (calculated below)
  };

  frame[6] = (address_ >> 8) & 0xFF;  // high byte → 0xF9
  frame[7] = address_ & 0xFF;         // low byte  → 0xCB
  frame[9] = static_cast<uint8_t>(command_);
  frame[10] = static_cast<uint8_t>(mode_);

  // checksum: two's complement of sum(frame[4..10])
  unsigned int sum = 0;
  for (int i = 4; i <= 10; i++) {
    sum += frame[i];
  }
  frame[11] = static_cast<uint8_t>(-sum);

  // Convert to timing vector
  std::vector<int> timings;
  for (int i = 0; i < 12; i++) {
    uint8_t byte = frame[i];
    for (int bit = 7; bit >= 0; bit--) {
      if (byte & (1 << bit)) {
        timings.push_back(SYMBOL_HIGH);
      } else {
        timings.push_back(SYMBOL_LOW);
      }
    }
  }

  // Debug log full frame
  std::ostringstream oss;
  for (int i = 0; i < 12; i++) {
    oss << "0x" << std::hex << std::uppercase << (int)frame[i];
    if (i < 11) oss << ", ";
  }
  ESP_LOGI(TAG, "Frame bytes: %s", oss.str().c_str());

  // Debug log waveform
  std::string dataString;
  for (int v : timings) {
    dataString += std::to_string(v) + ", ";
  }
  if (!dataString.empty()) dataString.erase(dataString.length() - 2);
  ESP_LOGD(TAG, "Waveform: %s", dataString.c_str());

  return timings;
}

void IrisComponent::send_command(IrisCommand cmd, IrisMode mode, uint32_t repeat) {
  auto timings = this->build_frame(this->address_, cmd, mode);

  // Use ESPHome raw transmitter
  auto call = this->tx_->transmit();
  remote_base::RemoteTransmitData *dst = call.get_data();

  for (uint32_t r = 0; r < (repeat + 1); r++) {
    for (int t : timings) {
      if (t > 0) {
        dst->mark(t);
      } else {
        dst->space(-t);
      }
    }
    // Gap between frames
    dst->space(10000);
  }

  call.perform();
}

bool IrisComponent::on_receive(remote_base::RemoteReceiveData data) {
  ESP_LOGD(TAG, "RX decode not implemented for 12-byte frame");
  return true;
}

}  // namespace iris
}  // namespace esphome

// test