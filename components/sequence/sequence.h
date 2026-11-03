#pragma once

#include <vector>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/light/light_state.h"

namespace esphome {
namespace sequence {

// One step of a sequence. In simple mode it holds the switches/lights that
// should be active while this stage is current; in advanced mode it carries an
// on_enter trigger with user actions. Both can be combined.
class SequenceStage {
 public:
  void add_switch(switch_::Switch *sw) { this->switches_.push_back(sw); }
  void add_light(light::LightState *light) { this->lights_.push_back(light); }
  void set_on_enter(Trigger<> *trigger) { this->on_enter_ = trigger; }

  // Turn this stage's actuators on, then fire the user on_enter actions.
  void enter() {
    for (auto *sw : this->switches_)
      sw->turn_on();
    for (auto *light : this->lights_)
      light->turn_on().perform();
    if (this->on_enter_ != nullptr)
      this->on_enter_->trigger();
  }

  void turn_off() {
    for (auto *sw : this->switches_)
      sw->turn_off();
    for (auto *light : this->lights_)
      light->turn_off().perform();
  }

 protected:
  std::vector<switch_::Switch *> switches_;
  std::vector<light::LightState *> lights_;
  Trigger<> *on_enter_{nullptr};
};

class Sequence : public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_timeout(uint32_t timeout) { this->timeout_ = timeout; }
  void set_loop(bool loop) { this->loop_ = loop; }

  void add_stage(SequenceStage *stage) { this->stages_.push_back(stage); }

  void set_off_trigger(Trigger<> *trigger) { this->off_trigger_ = trigger; }

  // Move to the next stage. After a timeout the next call drops to off instead.
  void advance();
  // Force everything off and return to stage 0.
  void off() { this->go_off_(); }
  // Alias of off(), kept for readability at call sites.
  void reset() { this->go_off_(); }
  // Jump straight to a stage (0 = off, 1..N = stage index).
  void set_stage(int stage);

  int current_stage() const { return this->current_; }

 protected:
  void enter_(int stage);
  void go_off_();
  void turn_all_off_();
  void arm_timeout_();

  std::vector<SequenceStage *> stages_;
  Trigger<> *off_trigger_{nullptr};
  uint32_t timeout_{0};
  int current_{0};
  bool loop_{true};
  bool timed_out_{false};
};

template<typename... Ts> class AdvanceAction : public Action<Ts...> {
 public:
  explicit AdvanceAction(Sequence *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->advance(); }

 protected:
  Sequence *parent_;
};

template<typename... Ts> class OffAction : public Action<Ts...> {
 public:
  explicit OffAction(Sequence *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->off(); }

 protected:
  Sequence *parent_;
};

template<typename... Ts> class ResetAction : public Action<Ts...> {
 public:
  explicit ResetAction(Sequence *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->reset(); }

 protected:
  Sequence *parent_;
};

template<typename... Ts> class SetStageAction : public Action<Ts...> {
 public:
  explicit SetStageAction(Sequence *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, stage)
  void play(Ts... x) override { this->parent_->set_stage(this->stage_.value(x...)); }

 protected:
  Sequence *parent_;
};

}  // namespace sequence
}  // namespace esphome
