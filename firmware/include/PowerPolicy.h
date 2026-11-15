#pragma once
#include <stdint.h>
namespace radio {
// Activity timestamps are monotonic modulo uint32_t, just like millis().
class PowerPolicy {
 public:
  static constexpr uint32_t IDLE_MS=60000;
  bool enabled=true;
  void activity(uint32_t now){last_=now;}
  uint32_t idleMs(uint32_t now)const{return int32_t(now-last_)<0?0:now-last_;}
  bool sleep(uint32_t now,bool busy,bool wakeEnabled,bool keyDown)const{
    return enabled&&!busy&&wakeEnabled&&!keyDown&&idleMs(now)>=IDLE_MS;
  }
 private:
  uint32_t last_=0;
};
}
