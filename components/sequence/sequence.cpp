#include "sequence.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sequence {

static const char *const TAG = "sequence";
static const char *const TIMEOUT_NAME = "seq_timeout";

void Sequence::setup() {
  // Boot into a known, all-off state.
  this->turn_all_off_();
  this->current_ = 0;
}

void Sequence::dump_config() {
  ESP_LOGCONFIG(TAG, "Sequence:");
  ESP_LOGCONFIG(TAG, "  Stages: %u", (unsigned) this->stages_.size());
  ESP_LOGCONFIG(TAG, "  Loop: %s", YESNO(this->loop_));
  if (this->timeout_ > 0) {
    ESP_LOGCONFIG(TAG, "  Timeout: %u ms", this->timeout_);
  }
}

void Sequence::advance() {
  // A press after the idle timeout dropped us out: go straight to off.
  if (this->timed_out_) {
    this->timed_out_ = false;
    this->go_off_();
    return;
  }

  int next = this->current_ + 1;
  if (next > (int) this->stages_.size()) {
    if (this->loop_) {
      next = 0;  // wrap through the off state
    } else {
      return;  // stay on the last stage
    }
  }

  if (next == 0) {
    this->go_off_();
  } else {
    this->enter_(next);
  }
}

void Sequence::set_stage(int stage) {
  if (stage <= 0) {
    this->go_off_();
  } else if (stage <= (int) this->stages_.size()) {
    this->enter_(stage);
  } else {
    ESP_LOGW(TAG, "Stage %d out of range (have %u)", stage, (unsigned) this->stages_.size());
  }
}

void Sequence::enter_(int stage) {
  ESP_LOGD(TAG, "Entering stage %d", stage);
  this->current_ = stage;
  // Exclusive state: clear every actuator, then light up this stage's set.
  this->turn_all_off_();
  this->stages_[stage - 1]->enter();
  this->arm_timeout_();
}

void Sequence::go_off_() {
  ESP_LOGD(TAG, "Sequence off");
  this->current_ = 0;
  this->timed_out_ = false;
  this->cancel_timeout(TIMEOUT_NAME);
  this->turn_all_off_();
  if (this->off_trigger_ != nullptr)
    this->off_trigger_->trigger();
}

void Sequence::turn_all_off_() {
  for (auto *stage : this->stages_)
    stage->turn_off();
}

void Sequence::arm_timeout_() {
  if (this->timeout_ == 0)
    return;
  // set_timeout with a fixed name replaces the pending one — restart for free.
  // Qualify the base method: our set_timeout(uint32_t) setter hides it here.
  this->Component::set_timeout(TIMEOUT_NAME, this->timeout_, [this]() {
    ESP_LOGD(TAG, "Idle timeout reached; next advance turns off");
    this->timed_out_ = true;
  });
}

}  // namespace sequence
}  // namespace esphome
