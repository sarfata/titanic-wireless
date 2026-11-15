#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
namespace photo {
constexpr int WIDTH=400,HEIGHT=300,BYTES=WIDTH*HEIGHT/8;
// Floyd–Steinberg halftone, full 4:3 frame. PBM/GFX: one bit means black.
inline bool dither(const uint8_t* rgb,int width,int height,uint8_t* out){
  if(!rgb||!out||width<=0||height<=0)return false;
  memset(out,0,BYTES);
  std::vector<int> current(WIDTH+2),next(WIDTH+2);
  for(int y=0;y<HEIGHT;++y){
    for(int x=0;x<WIDTH;++x){
      const uint8_t* p=rgb+3*((y*height/HEIGHT)*width+x*width/WIDTH);
      int gray=(77*p[0]+150*p[1]+29*p[2])/256+current[x+1]/16;
      gray=gray<0?0:gray>255?255:gray;
      bool black=gray<128;if(black)out[(y*WIDTH+x)/8]|=0x80>>(x&7);
      int error=gray-(black?0:255);
      current[x+2]+=error*7;next[x]+=error*3;next[x+1]+=error*5;next[x+2]+=error;
    }
    current.swap(next);for(auto& e:next)e=0;
  }
  return true;
}
}
