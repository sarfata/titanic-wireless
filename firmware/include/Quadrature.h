#pragma once
#include <stdint.h>
namespace radio {
// Gray-code decoder for dry-contact encoders. Count only at the stable 00/11
// phases; retracing a bouncing contact cancels it. No timing filter drops fast turns.
class Quadrature {
 public:
  void reset(uint8_t ab, int edgesPerStep=2) { last_=ab&3; partial_=0; edges_=edgesPerStep; }
  __attribute__((always_inline)) inline int update(uint8_t ab) {
    ab&=3;
    const uint8_t changed=last_^ab;
    if(!changed)return 0;
    if(changed==3){last_=ab;partial_=0;return 0;} // missed/invalid transition
    partial_+=((last_&1)^((ab>>1)&1))?1:-1;
    last_=ab;
    if(ab!=0 && ab!=3)return 0;
    if(partial_>=edges_){partial_-=edges_;return 1;}
    if(partial_<=-edges_){partial_+=edges_;return -1;}
    return 0;
  }
 private:
  uint8_t last_=3;
  int partial_=0,edges_=2;
};
}
