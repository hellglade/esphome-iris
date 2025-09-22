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

// Static helper to build accumulated pulse vector
static std::vector<int> build_frame(uint16_t address, esphome::iris::IrisCommand cmd, esphome::iris::IrisMode mode) {
    static const int MARK = 105;
    static const int SPACE = 104;

    uint8_t frame[12] = {
        0xAA, 0xAA, 0xAA, 0xAA, // header / sync
        0x2D, 0xD4,             // payload start
        0x00, 0x00,             // address MSB, LSB
        0x00,                   // reserved / payload
        0x00,                   // command
        0x00,                   // mode
        0x00                    // checksum
    };

    // Fill address, command, mode
    frame[6]  = (address >> 8) & 0xFF;
    frame[7]  = address & 0xFF;
    frame[9]  = static_cast<uint8_t>(cmd);
    frame[10] = static_cast<uint8_t>(mode);

    // Calculate checksum over bytes 4..10
    unsigned int sum = 0;
    for (int i = 4; i <= 10; i++) sum += frame[i];
    frame[11] = static_cast<uint8_t>(-sum);

    // Log frame
    std::ostringstream oss;
    for (auto b : frame) oss << std::hex << std::uppercase << (int)b << " ";
    ESP_LOGD(TAG, "Frame: %s", oss.str().c_str());

    // Build accumulated pulse vector
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
    if (accumulated != 0) DataVector.push_back(accumulated);

    // Optional: log the DataVector
    std::ostringstream voss;
    for (auto val : DataVector) voss << val << ", ";
    ESP_LOGD(TAG, "DataVector: %s", voss.str().c_str());

    return DataVector;
}

// Send command function
void esphome::iris::IrisComponent::send_command(IrisCommand cmd, IrisMode mode) {
    ESP_LOGD(TAG, "send_command: cmd=%d, mode=%d", cmd, mode);

    static const int REPEAT_COUNT = 6;

    // Build pulse vector
    auto DataVector = build_frame(this->address_, cmd, mode);

    // Transmit
    auto call = this->tx_->transmit();
    remote_base::RemoteTransmitData* dst = call.get_data();

    for (int repeat = 0; repeat < REPEAT_COUNT; repeat++) {
        for (auto pulse : DataVector) {
            bool level = (pulse > 0);
            dst->item(level ? pulse : 0, level ? 0 : -pulse);
        }
    }

    call.perform();
    ESP_LOGD(TAG, "send_command complete");
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