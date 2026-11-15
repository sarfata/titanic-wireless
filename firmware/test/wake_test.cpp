#include "EncoderWake.h"
#include <assert.h>
#include <stdio.h>
static bool wakes(radio::EncoderWake plan,uint8_t ab){
  return plan.high?(ab&plan.phases)!=0:((~ab)&plan.phases)!=0;
}
int main(){
  const uint8_t cycle[]={0,1,3,2};
  for(int start=0;start<4;++start){
    auto plan=radio::encoderWake(cycle[start]);
    assert(plan.phases&&!(plan.phases&~3));
    assert(!wakes(plan,cycle[start])); // No reboot loop at any resting position.
    for(int direction=-1;direction<=1;direction+=2){
      bool woke=false;
      for(int edge=1;edge<=2;++edge)woke|=wakes(plan,cycle[(start+direction*edge+4)%4]);
      assert(woke); // Either direction wakes within a two-edge detent.
    }
  }
  puts("Deep-sleep encoder wake tests passed");
}
