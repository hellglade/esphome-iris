#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/iris/iris.h"

namespace esphome {
namespace iris {

// Action to send a command
template<typename... Ts>
class IrisSendCommandAction : public Action<Ts...> {
 public:
  explicit IrisSendCommandAction(IrisComponent *iris) : iris_(iris) {}

  // Templatable command & mode
  TEMPLATABLE_VALUE(IrisCommand, command)
  TEMPLATABLE_VALUE(IrisMode, mode)

  void play(Ts... x) override {
    // Call send_command on the component with resolved command/mode
    this->iris_->send_command(this->command_.value(x...), this->mode_.value(x...));
  }

 protected:
  IrisComponent *iris_;
};

}  // namespace iris
}  // namespace esphome
