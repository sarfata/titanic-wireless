#pragma once
#include "ChargingFace.h"
namespace radio {
// Decorative LCD lamps, not physical LEDs or a paper-progress percentage.
template<class C> void startupFace(C& c,unsigned phase){
  phase%=48;
  const auto rim=rgb565(157,125,67),dark=rgb565(11,19,19),amber=rgb565(255,167,52);
  const auto pale=rgb565(162,210,175),shade=rgb565(53,90,74);
  c.fillScreen(0);c.fillCircle(112,112,110,rim);c.fillCircle(112,112,108,dark);
  chargingWord(c,"WAKING SPIRITS",49,amber);
  // Warm lamps chase clockwise and gently flicker; the whole LCD stays steady.
  for(unsigned i=0;i<16;++i){
    const float a=(135+i*270.0f/15)*3.14159265f/180;
    const int x=112+int(std::cos(a)*95),y=112+int(std::sin(a)*95);
    const bool on=((phase/3+16-i)%16)<4 || (phase/6+i*7)%11==0;
    c.fillCircle(x,y,4,shade);c.fillCircle(x,y,2,on?amber:rgb565(63,45,23));
  }
  const int bob=int(std::sin(phase*6.2831853f/48)*4),cy=110+bob;
  c.fillCircle(112,cy,29,shade);c.fillRect(83,cy,59,29,shade);
  c.fillCircle(112,cy-1,26,pale);c.fillRect(86,cy-1,53,30,pale);
  for(int i=0;i<4;++i)c.fillCircle(92+i*13,cy+27,7,pale);
  // A slow blink rather than a flash; tiny shadow eyes keep it friendly.
  if(phase==21||phase==22){c.fillRect(98,cy-2,7,2,dark);c.fillRect(119,cy-2,7,2,dark);}
  else {c.fillCircle(101,cy-2,4,dark);c.fillCircle(122,cy-2,4,dark);}
  c.fillCircle(112,cy+13,4,dark);
  chargingWord(c,"PLEASE WAIT",178,amber);
}
}
