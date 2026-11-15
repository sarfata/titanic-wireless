#pragma once
#include "FrequencyFace.h"
#include <cmath>
namespace radio {
// Alembic within the same 224 px round window. Bubbles indicate USB power,
// never a fabricated battery percentage or confirmed DIN charger state.
template<class C> void chargingWord(C& c,const char* word,int y,uint16_t ink){
  static constexpr uint8_t letters[26][7]={
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
  int count=0;while(word[count])++count;
  for(int i=0;i<count;++i)if(word[i]>='A'&&word[i]<='Z')smallGlyph(c,letters[word[i]-'A'],112-count*6/2+i*6,y,ink);
}
template<class C> void chargingFace(C& c,unsigned phase){
  phase%=32;
  const auto rim=rgb565(157,125,67),dark=rgb565(16,20,20),copper=rgb565(193,112,52),shine=rgb565(255,205,125);
  const auto glass=rgb565(99,157,151),amber=rgb565(181,80,13),light=rgb565(255,173,49);
  c.fillScreen(0);c.fillCircle(112,112,110,rim);c.fillCircle(112,112,108,dark);
  chargingWord(c,"SHORE POWER",43,shine);
  // Brass swan neck into the smaller receiving flask.
  const int px[]={80,80,92,105,128,149,153,153},py[]={99,74,67,71,84,93,101,127};
  for(int i=1;i<8;++i)for(int w=-2;w<=2;++w)c.drawLine(px[i-1]+w,py[i-1],px[i]+w,py[i],copper);
  for(int i=1;i<8;++i)c.drawLine(px[i-1]-1,py[i-1]-1,px[i]-1,py[i]-1,shine);
  c.fillRect(71,82,18,22,copper);c.fillRect(75,84,10,19,glass);
  c.fillCircle(80,122,31,copper);c.fillCircle(80,122,28,glass);c.fillCircle(80,122,26,rgb565(23,47,47));
  // Fixed decorative fill with a tiny ripple, not a state-of-charge level.
  for(int y=121;y<=147;++y){int dx=int(std::sqrt(float(26*26-(y-122)*(y-122))));c.fillRect(80-dx,y,2*dx+1,1,y<125?light:amber);}
  c.drawLine(61,116,62,110,shine);c.drawLine(64,105,69,101,shine);
  for(unsigned i=0;i<4;++i){int x=67+int(i)*8,y=143-int((phase*2+i*13)%33);if(y>124){c.fillCircle(x,y,2,light);c.fillCircle(x,y,1,dark);}}
  c.fillRect(144,126,18,16,glass);c.fillRect(147,127,12,16,dark);
  c.fillCircle(153,149,19,glass);c.fillCircle(153,149,16,dark);
  for(int y=150;y<=164;++y){int dx=int(std::sqrt(float(16*16-(y-149)*(y-149))));c.fillRect(153-dx,y,2*dx+1,1,light);}
  int drop=128+int(phase%12)*2;if(drop<149){c.fillCircle(153,drop,2,shine);c.drawLine(153,drop-3,153,drop,shine);}
  c.drawLine(166,143,166,150,shine);
  // Small animated burner and stand under the main flask.
  c.drawLine(58,151,102,151,rim);c.drawLine(61,152,57,172,rim);c.drawLine(99,152,103,172,rim);
  for(int y=158;y<174;++y){int half=(y-157)/3+1;int sway=int((phase/4)%3)-1;c.fillRect(80-half+sway,y,half*2,1,y>164?light:copper);}
  c.fillRect(68,174,24,3,copper);
  chargingWord(c,"PRESS TO WAKE",188,shine);
}
}
