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
  uint8_t sync_count = 0;
  while (data.is_valid()) {
    while (data.expect_item(SYMBOL * 4, SYMBOL * 4)) {
      sync_count++;
    }
    if (sync_count >= 2 && data.expect_mark(4550)) {
      break;
    }
    sync_count = 0;
    data.advance();
  }
  if (sync_count < 2) {
    return true;
  }
  data.expect_space(SYMBOL);

  uint8_t frame[7];
  for (uint8_t &byte : frame) {
    for (uint32_t i = 0; i < 8; i++) {
      byte <<= 1;
      if (data.expect_mark(SYMBOL) || data.expect_mark(SYMBOL * 2)) {
        data.expect_space(SYMBOL);
        byte |= 0;
      } else if (data.expect_space(SYMBOL) || data.expect_space(SYMBOL * 2)) {
        data.expect_mark(SYMBOL);
        byte |= 1;
      } else {
        return true;
      }
    }
  }

  for (uint8_t i = 6; i >= 1; i--) {
    frame[i] ^= frame[i - 1];
  }

  uint8_t crc = 0;
  for (uint8_t i = 0; i < 7; i++) {
    crc ^= frame[i];
    crc ^= frame[i] >> 4;
  }
  if ((crc & 0xF) == 0) {
    uint8_t command = frame[1] >> 4;
    uint16_t code = (frame[2] << 8) | frame[3];
    uint32_t address = (frame[4] << 16) | (frame[5] << 8) | frame[6];
    ESP_LOGD(TAG, "Received: command: %" PRIx8 ", code: %" PRIu16 ", address %" PRIx32,
             command, code, address);
    if (command == IRIS_SENSOR) {
      for (auto *sensor : this->sensors_) {
        sensor->update_windy(address, (code & 1) != 0);
        sensor->update_sunny(address, (code & 2) != 0);
      }
    }
  }

  return true;
}



}  // namespace iris
}  // namespace esphome

// test