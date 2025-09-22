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
  std::vector<uint8_t> received_bytes;

  // Expected bits: 12 bytes * 8 bits = 96 bits
  const int expected_bits = 12 * 8;

  // Helper lambda for approximate timing check
  auto is_approx = [](int value, int expected, int tolerance = 15) {
    return std::abs(value - expected) <= tolerance;
  };

  const auto &raw = data.get_data(); // <-- FIXED HERE

  // raw alternates mark and space durations: raw[0] = mark, raw[1] = space, etc.
  if (raw.size() < expected_bits * 2) {
    ESP_LOGW(TAG, "Not enough raw data: expected at least %d mark/space pairs, got %d",
             expected_bits, static_cast<int>(raw.size() / 2));
    return false;
  }

  std::vector<bool> bits;

  for (size_t i = 0; i + 1 < raw.size() && bits.size() < expected_bits; i += 2) {
    int mark = raw[i];
    int space = raw[i + 1];

    // Decode bit by timing:
    // bit 0 = mark ~105 µs + space ~104 µs
    // bit 1 = space ~105 µs + mark ~104 µs
    if (is_approx(mark, 105) && is_approx(space, 104)) {
      bits.push_back(false); // bit 0
    } else if (is_approx(space, 105) && is_approx(mark, 104)) {
      bits.push_back(true);  // bit 1
    } else {
      ESP_LOGW(TAG, "Unexpected mark/space timings at index %zu: mark=%d, space=%d", i, mark, space);
      return false;
    }
  }

  // Pack bits into bytes (MSB first)
  for (int i = 0; i < expected_bits; i += 8) {
    uint8_t byte = 0;
    for (int b = 0; b < 8; b++) {
      byte <<= 1;
      if (bits[i + b]) {
        byte |= 1;
      }
    }
    received_bytes.push_back(byte);
  }

  if (received_bytes.size() < 12) {
    ESP_LOGW(TAG, "Decoded bytes less than 12");
    return false;
  }

  // Log header and payload
  ESP_LOGD(TAG, "Header/Sync: %02X %02X %02X %02X %02X %02X",
           received_bytes[0], received_bytes[1], received_bytes[2],
           received_bytes[3], received_bytes[4], received_bytes[5]);
  ESP_LOGD(TAG, "Payload: %02X %02X %02X %02X %02X %02X",
           received_bytes[6], received_bytes[7], received_bytes[8],
           received_bytes[9], received_bytes[10], received_bytes[11]);

  // Validate checksum (two's complement of sum bytes 4..10)
  unsigned int sum = 0;
  for (int i = 4; i <= 10; i++) {
    sum += received_bytes[i];
  }
  uint8_t checksum = static_cast<uint8_t>(-sum);

  if (checksum != received_bytes[11]) {
    ESP_LOGW(TAG, "Checksum failed: calculated %02X but received %02X", checksum, received_bytes[11]);
    return false;
  }

  ESP_LOGI(TAG, "Checksum valid, frame received successfully.");

  // Extract command and mode
  IrisCommand cmd = static_cast<IrisCommand>(received_bytes[9]);
  IrisMode mode = static_cast<IrisMode>(received_bytes[10]);

  // TODO: Handle received command and mode here
  // e.g. handle_command(cmd, mode);

  return true;
}


}  // namespace iris
}  // namespace esphome

// test