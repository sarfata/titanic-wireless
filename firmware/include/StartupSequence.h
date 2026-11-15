#pragma once
#include "PaperReveal.h"
namespace radio {
enum class StartupResult {Waiting,Ready,TimedOut};
// Silent boot/story prelude. A stale paper acknowledgment cannot start audio.
class StartupSequence {
 public:
  static constexpr uint32_t MIN_MS=2800,MAX_MS=20000;
  void begin(uint32_t now,const paper::Frame& target,bool transmit){
    started_=now;target_=target;transmit_=transmit;active_=true;
  }
  bool active()const{return active_;}
  bool transmit()const{return transmit_;}
  // Story preparation still gates audio, but only a boot/wake shows the ghost.
  bool bootAnimation()const{return active_&&!transmit_;}
  uint32_t elapsed(uint32_t now)const{return int32_t(now-started_)<0?0:now-started_;}
  void cancel(){active_=transmit_=false;}
  StartupResult update(uint32_t now,bool paperEnabled,const paper::Frame& shown){
    if(!active_)return StartupResult::Waiting;
    const auto age=elapsed(now);
    if(age>=MIN_MS&&(!paperEnabled||paper::caughtUp(shown,target_))){active_=false;return StartupResult::Ready;}
    if(age>=MAX_MS){active_=transmit_=false;return StartupResult::TimedOut;}
    return StartupResult::Waiting;
  }
 private:
  paper::Frame target_{};uint32_t started_=0;bool active_=false,transmit_=false;
};
}
