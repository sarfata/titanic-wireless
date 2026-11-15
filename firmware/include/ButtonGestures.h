#pragma once
#include <stdint.h>
namespace radio {
enum class Gesture {None,Click,Hold,Triple};
// Feed debounced contact state. Delay a short click until a triple is ruled out.
class ButtonGestures {
 public:
  static constexpr uint32_t HOLD_MS=900,CLICK_GAP_MS=350;
  void reset(bool down,uint32_t now){down_=blocked_=down;long_=false;clicks_=0;pressed_=released_=now;}
  Gesture update(bool down,uint32_t now){
    if(blocked_){if(!down){blocked_=false;down_=false;}return Gesture::None;}
    if(down!=down_){
      down_=down;
      if(down){pressed_=now;long_=false;}
      else if(!long_){released_=now;if(++clicks_==3){clicks_=0;return Gesture::Triple;}}
    }
    if(down_&&!long_&&uint32_t(now-pressed_)>=HOLD_MS){long_=true;clicks_=0;return Gesture::Hold;}
    if(!down_&&clicks_&&uint32_t(now-released_)>=CLICK_GAP_MS){clicks_=0;return Gesture::Click;}
    return Gesture::None;
  }
 private:
  bool down_=false,blocked_=false,long_=false;
  uint8_t clicks_=0;
  uint32_t pressed_=0,released_=0;
};
}
