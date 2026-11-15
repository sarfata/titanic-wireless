#pragma once
#include "PaperReveal.h"
namespace paper {
// Hold a completed live exchange or photo starting after its final refresh.
// Reset/start cancels the hold; old acknowledgments cannot finish a stopped round.
class Views {
 public:
  View view=View::Reception;
  void newRound(){view=View::Reception;waiting_=timing_=false;}
  void certificate(){view=View::Certificate;waiting_=timing_=false;}
  void photo(const Frame& target){view=View::Photo;complete(target);}
  void complete(const Frame& target){target_=target;waiting_=true;timing_=false;}
  void presented(const Frame& f,uint32_t now){
    if(waiting_ && !timing_ && caughtUp(f,target_)){
      started_=now;timing_=true;
    }
  }
  bool tick(uint32_t now,uint32_t duration){
    // The worker acknowledgment can be newer than this loop's captured `now`.
    // Signed elapsed time avoids mistaking that small negative age for rollover.
    if(waiting_ && timing_ && int32_t(now-started_)>=int32_t(duration)){
      if(view==View::Photo)newRound();else certificate();
      return true;
    }
    return false;
  }
 private:
  bool waiting_=false,timing_=false;uint32_t started_=0;Frame target_{};
};
}
