#include "iris.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include "esphome/core/application.h"  // for esphome::delay()

#include <vector>
#include <sstream>
#include <string>
#include <inttypes.h>  // for PRIx16

namespace esphome {
namespace iris {

static const char *TAG = "iris";

IrisComponent::IrisComponent()
{
  this->gdo0_ = NULL;
  this->emitter_ = NULL;
}

void IrisComponent::set_config_gdo0(InternalGPIOPin* pin)
{
  gdo0_ = pin; 
  
  if(gdo2_ == NULL) gdo2_ = pin;
}

void IrisComponent::set_config_emitter(InternalGPIOPin* pin)
{
  emitter_ = pin;
}

// Symbol durations
static const int MARK = 105;   // HIGH bit duration (us)
static const int SPACE = 104;  // LOW bit duration (us)

static const int REPEAT_COUNT = 5;  // Consistent repeat count

void IrisComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "Iris:");
    ESP_LOGCONFIG(TAG, "  Address: %" PRIx16, this->address_);
    LOG_PIN("  Iris GDO0: ", this->gdo0_);
    LOG_PIN("  Iris EMITTER: ", this->emitter_);
}

void IrisComponent::setup() {
  this->gdo0_->setup();
  this->emitter_->setup();
  this->gdo0_->pin_mode(gpio::FLAG_OUTPUT);
  this->emmiter_->pin_mode(gpio::FLAG_OUTPUT);
  
    ESP_LOGCONFIG(TAG, "Iris setup done");
}

// Static helper to build accumulated pulse vector
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

    // Log DataVector
    std::ostringstream voss;
    for (auto val : DataVector) voss << val << ", ";
    ESP_LOGD(TAG, "DataVector: %s", voss.str().c_str());

    return DataVector;
}

// Send command
void IrisComponent::send_command(IrisCommand cmd, IrisMode mode) {
    ESP_LOGD(TAG, "send_command: cmd=%d, mode=%d", cmd, mode);

    static const int REPEAT_COUNT = 6;

    // Build pulse vector
    auto DataVector = build_frame(this->address_, cmd, mode);

    // transmit directly from code instead of using transmitt_raw feature
    int repeat = 5;
    for (int r = 0; r < repeat; r++) {
      // Transmit pulse sequence on GDO0 pin
      for (int pulse : DataVector) {
          bool level = (pulse > 0);
          gdo0_->digital_write(level);
          delayMicroseconds(abs(pulse));
      }

       // Optional small delay between repeats
       // delay(10);
    }
    ESP_LOGD(TAG, "send_command complete");
}

}  // namespace iris
}  // namespace esphome
