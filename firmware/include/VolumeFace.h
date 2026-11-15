#pragma once
#include "FrequencyFace.h"
#include <cmath>
namespace radio {
// Same brass rim, a halo of twenty lamps and a small loudspeaker emblem.
template<class Canvas> void volumeFace(Canvas& c,int percent){
  c.fillScreen(uint16_t(0));c.fillCircle(112,112,110,rgb565(157,125,67));
  c.fillCircle(112,112,108,rgb565(21,16,11));
  static constexpr uint8_t title[6][7]={
    {17,17,17,17,17,10,4},{14,17,17,17,17,17,14},{16,16,16,16,16,16,31},
    {17,17,17,17,17,17,14},{17,27,21,21,17,17,17},{31,16,16,30,16,16,31}};
  for(int i=0;i<6;++i)smallGlyph(c,title[i],89+i*8,55,rgb565(205,165,94));
  for(int i=0;i<20;++i){
    // Fill clockwise: lower left -> top -> lower right, like the physical knob.
    float angle=(-145+i*290.0f/19)*3.14159265f/180;
    int x=112+int(std::sin(angle)*88),y=112-int(std::cos(angle)*88);
    bool lit=i<(percent+4)/5;
    c.fillCircle(x,y,4,lit?rgb565(181,83,18):rgb565(66,44,24));
    if(lit)c.fillCircle(x-1,y-1,2,rgb565(255,213,113));
  }
  int digits[3]={percent/100,(percent/10)%10,percent%10};
  for(int d=0;d<3;++d){if(d==0&&percent<100)continue;
    int x=(percent<100?38:31)+d*45;
    for(int r=0;r<7;++r)for(int b=0;b<5;++b)if(bulbDigits[digits[d]][r]&(16>>b)){
      c.fillCircle(x+b*6,88+r*6,2,rgb565(255,170,58));
      c.fillCircle(x+b*6-1,87+r*6,1,rgb565(255,231,153));
    }
  }
  static constexpr uint8_t pct[7]={25,25,2,4,8,19,19};smallGlyph(c,pct,165,121,rgb565(255,199,111));
  auto gold=rgb565(210,159,81);
  c.fillRect(91,149,8,12,gold);
  for(int i=0;i<10;++i)c.drawLine(99+i,149-i/2,99+i,160+i/2,gold);
  for(int i=0;i<3;++i){int x=119+i*7;c.drawLine(x,148-i*3,x+3,155,gold);c.drawLine(x+3,155,x,162+i*3,gold);}
}
}
