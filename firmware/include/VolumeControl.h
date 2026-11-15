#pragma once
#include <stdint.h>
namespace radio {
inline int percentToVolume(int p){return (p*255+50)/100;}
inline int volumeToPercent(int v){return (v*100+127)/255;}
class VolumeControl {
 public:
  int percent=80;
  bool enabled=false;
  void enter(uint32_t now){enabled=true;pending_=false;changed_=now;}
  void leave(){enabled=false;pending_=false;}
  void turn(int steps,uint32_t now){
    if(!steps)return;
    int next=percent+steps*5;percent=next<0?0:next>100?100:next;
    pending_=true;changed_=now;
  }
  bool settled(uint32_t now){
    if(!pending_||int32_t(now-changed_)<650)return false;
    pending_=false;return true;
  }
  bool expired(uint32_t now,uint32_t duration)const{
    return enabled&&int32_t(now-changed_)>=int32_t(duration);
  }
 private:
  bool pending_=false;uint32_t changed_=0;
};
}
