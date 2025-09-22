#include "iris.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

#include <vector>
#include <sstream>
#include <string>

namespace esphome {
namespace iris {

static const char *TAG = "iris";

// Symbol durations
static const int MARK = 105;   // HIGH bit duration
static const int SPACE = 104;  // LOW bit duration

void IrisComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "Iris:");
    ESP_LOGCONFIG(TAG, "  Address: %" PRIx16, this->address_);
}

void IrisComponent::setup() {
    ESP_LOGCONFIG(TAG, "Iris setup done");
    if (this->rx_ != nullptr)
        this->rx_->register_listener(this);
}

// Build frame and accumulate consecutive pulses
static std::vector<int> build_frame(uint16_t address, IrisCommand cmd, IrisMode mode) {
    uint8_t frame[12] = {
        0xAA, 0xAA, 0xAA, 0xAA, // Header / sync
        0x2D, 0xD4,             // Payload start
        0x00, 0x00,             // Address MSB, LSB
        0x00,                   // Reserved / payload
        0x00,                   // Command
        0x00,                   // Mode
        0x00                    // Checksum
    };

    frame[6]  = (address >> 8) & 0xFF;
    frame[7]  = address & 0xFF;
    frame[9]  = static_cast<uint8_t>(cmd);
    frame[10] = static_cast<uint8_t>(mode);

    // Checksum over bytes 4..10
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

    // Log DataVector
    std::ostringstream voss;
    for (auto val : DataVector) voss << val << ", ";
    ESP_LOGD(TAG, "DataVector: %s", voss.str().c_str());

    return DataVector;
}

// Send command
void IrisComponent::send_command(IrisCommand cmd, IrisMode mode) {
    auto DataVector = build_frame(this->address_, cmd, mode); // your frame builder

    if (this->my_transmitter_ != nullptr) {
        // Send full frame via your custom transmitter
        this->my_transmitter_->send_raw_data(DataVector);
        ESP_LOGD(TAG, "send_command complete");
    } else {
        ESP_LOGE(TAG, "Transmitter not set!");
    }
}


// Receive callback
bool IrisComponent::on_receive(remote_base::RemoteReceiveData data) {
    const auto &timings = data.get_raw_data();

    ESP_LOGD(TAG, "Received raw timings (%d):", static_cast<int>(timings.size()));
    std::ostringstream out;
    for (size_t i = 0; i < timings.size(); i++) {
        out << timings[i];
        if (i < timings.size() - 1) out << ", ";
    }
    ESP_LOGD(TAG, "%s", out.str().c_str());

    return true;
}

}  // namespace iris
}  // namespace esphome
