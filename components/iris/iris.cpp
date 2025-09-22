#include "iris.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <sstream>

namespace esphome {
namespace iris {

static const char *TAG = "iris";

// Symbol durations (µs)
static const int MARK = 105;   // HIGH pulse
static const int SPACE = 104;  // LOW pulse

void IrisComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Iris:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%04X", this->address_);
  if (tx_pin_)
    ESP_LOGCONFIG(TAG, "  TX custom pin assigned");
}

void IrisComponent::setup() {
  ESP_LOGCONFIG(TAG, "Iris setup done");
  if (rx_)
    rx_->register_listener(this);
}

// Build pulse vector with consecutive pulses accumulated
std::vector<int> IrisComponent::build_frame(uint16_t address, IrisCommand cmd, IrisMode mode) {
  uint8_t frame[12] = {
      0xAA, 0xAA, 0xAA, 0xAA, // Header / sync
      0x2D, 0xD4,             // Payload start
      0x00, 0x00,             // Address
      0x00,                   // Reserved
      0x00,                   // Command
      0x00,                   // Mode
      0x00                    // Checksum
  };

  frame[6] = (address >> 8) & 0xFF;
  frame[7] = address & 0xFF;
  frame[9] = static_cast<uint8_t>(cmd);
  frame[10] = static_cast<uint8_t>(mode);

  // Checksum
  uint8_t sum = 0;
  for (int i = 4; i <= 10; i++) sum += frame[i];
  frame[11] = static_cast<uint8_t>(-sum);

  // Build pulse vector
  std::vector<int> DataVector;
  int accumulated = 0;
  int last_sign = 0;

  for (int i = 0; i < 12; i++) {
    uint8_t byte = frame[i];
    for (int bit = 7; bit >= 0; bit--) {
      int pulse = (byte & (1 << bit)) ? MARK : -SPACE;
      int sign = (pulse > 0) ? 1 : -1;
      if (last_sign == 0) {
        accumulated = pulse;
        last_sign = sign;
      } else if (sign == last_sign) {
        accumulated += pulse;
      } else {
        DataVector.push_back(accumulated);
        accumulated = pulse;
        last_sign = sign;
      }
    }
  }
  if (accumulated != 0)
    DataVector.push_back(accumulated);

  // Optional log
  std::ostringstream oss;
  for (auto val : DataVector) oss << val << ", ";
  ESP_LOGD(TAG, "DataVector: %s", oss.str().c_str());

  return DataVector;
}

// Send command by toggling TX pin directly
void IrisComponent::send_command(IrisCommand cmd, IrisMode mode) {
  if (!tx_pin_) {
    ESP_LOGE(TAG, "TX pin not assigned, cannot send command");
    return;
  }

  ESP_LOGD(TAG, "send_command: cmd=%d, mode=%d", cmd, mode);

  const int REPEAT_COUNT = 6;
  auto DataVector = build_frame(this->address_, cmd, mode);

  for (int repeat = 0; repeat < REPEAT_COUNT; repeat++) {
    for (auto pulse : DataVector) {
      if (pulse > 0) {
        tx_pin_->digital_write(true);
        delayMicroseconds(pulse);
        tx_pin_->digital_write(false);
      } else {
        tx_pin_->digital_write(false);
        delayMicroseconds(-pulse);
        tx_pin_->digital_write(true);
      }
    }
  }

  ESP_LOGD(TAG, "send_command complete");
}

// Receive callback (optional)
bool IrisComponent::on_receive(remote_base::RemoteReceiveData data) {
  const auto &timings = data.get_raw_data();
  std::ostringstream out;
  for (size_t i = 0; i < timings.size(); i++) {
    out << timings[i];
    if (i < timings.size() - 1) out << ", ";
  }
  ESP_LOGD(TAG, "Received raw timings: %s", out.str().c_str());
  return true;
}

}  // namespace iris
}  // namespace esphome
