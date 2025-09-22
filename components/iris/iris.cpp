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

void IrisComponent::send_command(IrisCommand cmd, IrisMode mode) {
  ESP_LOGD(TAG, "send_command: cmd=%d, mode=%d", cmd, mode);

  static const int MARK = 105;   // Mark duration (µs)
  static const int SPACE = 104;  // Space duration (µs)
  static const int REPEAT_COUNT = 6; // Total frames to send

  uint8_t frame[12] = {
      0xAA, 0xAA, 0xAA, 0xAA, // Header / sync bytes
      0x2D, 0xD4,             // Payload start
      0x00, 0x00,             // Address MSB, LSB
      0x00,                   // Reserved or payload
      0x00,                   // Command
      0x00,                   // Mode
      0x00                    // Checksum
  };



  // Set address bytes
  frame[6] = (this->address_ >> 8) & 0xFF;
  frame[7] = this->address_ & 0xFF;

  // Set command and mode
  frame[9] = static_cast<uint8_t>(cmd);
  frame[10] = static_cast<uint8_t>(mode);

  // Calculate checksum over bytes 4 to 10
  unsigned int sum = 0;
  for (int i = 4; i <= 10; i++) {
    sum += frame[i];
  }
  frame[11] = static_cast<uint8_t>(-sum);  // Two’s complement checksum

  void log_frame(const uint8_t *frame, size_t len) {
    char buf[3 * len + 1];  // "FF " per byte + null terminator
    char *ptr = buf;

    for (size_t i = 0; i < len; i++) {
        ptr += sprintf(ptr, "%02X ", frame[i]);
    }

    ESP_LOGD(TAG, "Frame: %s", buf);
}


  
  
  auto call = this->tx_->transmit();
  remote_base::RemoteTransmitData *dst = call.get_data();

  for (int repeat = 0; repeat < REPEAT_COUNT; repeat++) {
    for (int i = 0; i < 12; i++) {
      uint8_t byte = frame[i];
      for (int bit = 7; bit >= 0; bit--) {
        bool bit_set = (byte & (1 << bit)) != 0;
        if (bit_set) {
          dst->item(MARK, SPACE);
        } else {
          dst->item(SPACE, MARK);
        }
      }
    }
    // No space or delay between repeats as requested
  }

  call.perform();
}

bool IrisComponent::on_receive(remote_base::RemoteReceiveData data) {
  const auto &timings = data.get_raw_data(); // Get raw timings as a vector of int

  ESP_LOGD(TAG, "Received raw timings (%d):", static_cast<int>(timings.size()));
  std::string out;
  for (size_t i = 0; i < timings.size(); i++) {
    out += std::to_string(timings[i]);
    if (i < timings.size() - 1) out += ", ";
  }
  ESP_LOGD(TAG, "%s", out.c_str());

  return true;
}



}  // namespace iris
}  // namespace esphome

// test