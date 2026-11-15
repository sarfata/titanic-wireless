#pragma once
#include <stdint.h>
namespace radio {
// Only deliberate tuning activity opens this LCD view. Game detuning does not.
class TuningView {
 public:
  void rotated(uint32_t now) { lastTurn_=now; showing_=true; }
  void reset() { showing_=false; }
  bool active(uint32_t now,uint32_t timeout) {
    if(showing_ && uint32_t(now-lastTurn_)>=timeout)showing_=false;
    return showing_;
  }
 private:
  uint32_t lastTurn_=0;
  bool showing_=false;
};
}
