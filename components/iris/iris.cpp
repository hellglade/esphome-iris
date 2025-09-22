#include "iris.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

#include <vector>
#include <sstream>

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
    if (this->rx_)
        this->rx_->register_listener(this);
}

// Build frame helper
static std::vector<int> build_frame(uint16_t address, IrisCommand cmd, IrisMode mode) {
    uint8_t frame[12] = {
        0xAA, 0xAA, 0xAA, 0xAA,
        0x2D, 0xD4,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    frame[6] = (address >> 8) & 0xFF;
    frame[7] = address & 0xFF;
    frame[9] = static_cast<uint8_t>(cmd);
    frame[10] = static_cast<uint8_t>(mode);

    // checksum over bytes 4..10
    unsigned int sum = 0;
    for (int i = 4; i <= 10; i++) sum += frame[i];
    frame[11] = static_cast<uint8_t>(-sum);

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

    return DataVector;
}

// Send command
void IrisComponent::send_command(IrisCommand cmd, IrisMode mode) {
    ESP_LOGD(TAG, "send_command: cmd=%d, mode=%d", cmd, mode);

    const int REPEAT_COUNT = 6;
    auto DataVector = build_frame(this->address_, cmd, mode);

    for (int repeat = 0; repeat < REPEAT_COUNT; repeat++) {
        if (this->tx_) {
            // Use RemoteTransmitter if available
            auto call = this->tx_->transmit_raw();
            call.set_code(DataVector);
            call.perform();
        } else if (this->tx_pin_) {
            // Custom GPIO toggle
            for (auto pulse : DataVector) {
                if (pulse > 0)
                    this->tx_pin_->digital_write(true);
                else
                    this->tx_pin_->digital_write(false);

                delayMicroseconds(abs(pulse));
            }
        }
    }

    ESP_LOGD(TAG, "send_command complete");
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
