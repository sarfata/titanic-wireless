#pragma once
#include <stdint.h>
// Bulb-matrix numerals, rendered by both the CoreS3 and the native preview.
// Coordinates fit the existing 224-pixel circular aperture. No new font/assets.
namespace radio {
constexpr uint16_t rgb565(int r,int g,int b) { return ((r>>3)<<11)|((g>>2)<<5)|(b>>3); }
static constexpr uint8_t bulbDigits[10][7]={
  {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
  {14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
  {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
  {6,8,16,30,17,17,14},{31,1,2,4,8,8,8},
  {14,17,17,14,17,17,14},{14,17,17,15,1,2,12}
};
template<class Canvas> void smallGlyph(Canvas& c,const uint8_t* rows,int x,int y,uint16_t color){
  for(int r=0;r<7;++r)for(int b=0;b<5;++b)if(rows[r]&(16>>b))c.fillRect(x+b,y+r,1,1,color);
}
template<class Canvas> void frequencyFace(Canvas& c,int frequency){
  c.fillScreen(uint16_t(0));
  c.fillCircle(112,112,110,rgb565(157,125,67));
  c.fillCircle(112,112,108,rgb565(46,34,20));
  for(int r=106;r>0;r-=10)c.fillCircle(112,112,r,rgb565(13+(106-r)/10,10+(106-r)/20,8));
  // Small TUNING legend, subordinate to the three large glowing digits.
  static constexpr uint8_t title[6][7]={
    {31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,25,21,19,17,17,17},
    {14,4,4,4,4,4,14},{17,25,21,19,17,17,17},{14,17,16,23,17,17,14}};
  for(int i=0;i<6;++i)smallGlyph(c,title[i],89+i*8,46,rgb565(179,133,75));
  frequency=frequency<0?0:frequency>999?999:frequency;
  const int digits[3]={frequency/100,(frequency/10)%10,frequency%10};
  for(int d=0;d<3;++d){
    const int x=39+d*54;
    // Dark glass and fine brass rails behind each bulb bank.
    c.fillRect(x,72,39,79,rgb565(30,22,16));
    c.drawLine(x,73,x,149,rgb565(73,51,32));
    c.drawLine(x+38,73,x+38,149,rgb565(73,51,32));
    c.drawLine(x+3,70,x+35,70,rgb565(98,69,38));
    c.drawLine(x+3,153,x+35,153,rgb565(98,69,38));
    for(int r=0;r<7;++r)for(int b=0;b<5;++b){
      const int px=x+5+b*7,py=90+r*7;
      const bool lit=bulbDigits[digits[d]][r]&(16>>b);
      c.fillCircle(px,py,3,lit?rgb565(126,48,12):rgb565(45,29,18));
      c.fillCircle(px,py,2,lit?rgb565(255,144,39):rgb565(59,37,20));
      if(lit)c.fillCircle(px-1,py-1,1,rgb565(255,218,134));
    }
  }
  static constexpr uint8_t units[3][7]={{16,16,18,20,24,20,18},{17,17,17,31,17,17,17},{0,0,31,2,4,8,31}};
  for(int i=0;i<3;++i)smallGlyph(c,units[i],102+i*8,174,rgb565(200,154,86));
}
}
