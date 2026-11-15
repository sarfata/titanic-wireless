#pragma once
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "PatrickHand30.h"
#include "PaperReveal.h"
namespace paper {
constexpr const char* CERTIFICATE_NAME_1="Harold Bride";
constexpr const char* CERTIFICATE_NAME_2="Radio Cadet";
template<class Display> void centered(Display& d,const char* text,int baseline,const GFXfont* font){
  d.setFont(font);d.setTextSize(1);
  int16_t x,y;uint16_t w,h;d.getTextBounds(text,0,baseline,&x,&y,&w,&h);
  d.setCursor((400-int(w))/2-x,baseline);d.print(text);
}
template<class Display> void certificateArtwork(Display& d){
  d.fillScreen(1);d.setTextColor(0);d.setTextSize(1);
  d.drawRect(7,7,386,286,0);d.drawRect(11,11,378,278,0);
  for(int x:{18,381})for(int y:{18,281})d.fillCircle(x,y,2,0);
  centered(d,"R.M.S. TITANIC  -  WIRELESS OFFICE",33,&::FreeSerif9pt7b);
  centered(d,"Certificate",77,&::FreeSerifBold18pt7b);
  centered(d,"of Radiotelegraphy",104,&::FreeSerif12pt7b);
  d.drawFastHLine(77,116,246,0);
  centered(d,CERTIFICATE_NAME_1,147,&PatrickHand30);
  centered(d,"&",169,&::FreeSerif9pt7b);
  centered(d,CERTIFICATE_NAME_2,201,&PatrickHand30);
  d.drawFastHLine(77,214,246,0);
  centered(d,"HISTORIC DISTRESS / CALLING",235,&::FreeSerif9pt7b);
  centered(d,"500 kHz  /  600 metres",260,&::FreeSerif12pt7b);
  centered(d,"Press to start a new exchange",281,&::FreeSerif9pt7b);
}
template<class Display> void receptionArtwork(Display& d,const Frame& f){
  d.fillScreen(1);d.setTextColor(0);
  d.setFont(nullptr);d.setTextSize(2);d.setCursor(12,10);d.print("WIRELESS OFFICE");
  d.drawFastHLine(12,34,376,0);
  d.setTextSize(1);d.setFont(&PatrickHand30);
  d.setCursor(12,74);d.print("Titanic:");d.setCursor(12,112);d.print(f.tx);
  d.setCursor(12,194);d.print("Carpathia:");d.setCursor(12,232);d.print(f.rx);
  centered(d,"RECEPTION  -  500 kHz / 600 m",279,&::FreeSerif9pt7b);
}
template<class Display> void artwork(Display& d,const Frame& f){
  if(f.view==View::Certificate)certificateArtwork(d);else receptionArtwork(d,f);
}
}
