// Render the production artwork/fonts without an Arduino runtime.
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "PaperArtwork.h"
#include <glcdfont.c>
class Raster {
  std::vector<unsigned char> pixels=std::vector<unsigned char>(400*300,255);
  const GFXfont* face=nullptr;int cx=0,cy=0,scale=1,color=0;
 public:
  void pixel(int x,int y,int c){if(x>=0&&x<400&&y>=0&&y<300)pixels[y*400+x]=c?255:0;}
  void fillScreen(int c){std::fill(pixels.begin(),pixels.end(),c?255:0);}
  void setTextColor(int c){color=c;} void setTextSize(int n){scale=n;}
  void setFont(const GFXfont* f){face=f;}void setCursor(int x,int y){cx=x;cy=y;}
  void drawFastHLine(int x,int y,int w,int c){for(int i=0;i<w;++i)pixel(x+i,y,c);}
  void drawRect(int x,int y,int w,int h,int c){drawFastHLine(x,y,w,c);drawFastHLine(x,y+h-1,w,c);for(int j=0;j<h;++j){pixel(x,y+j,c);pixel(x+w-1,y+j,c);}}
  void fillCircle(int x,int y,int r,int c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(i*i+j*j<=r*r)pixel(x+i,y+j,c);}
  void getTextBounds(const char* text,int x,int y,int16_t* bx,int16_t* by,uint16_t* bw,uint16_t* bh){
    int minx=10000,miny=10000,maxx=-10000,maxy=-10000;
    for(const unsigned char* p=(const unsigned char*)text;*p;++p){
      if(face && *p>=face->first && *p<=face->last){const auto& g=face->glyph[*p-face->first];
        if(g.width&&g.height){minx=std::min(minx,x+g.xOffset*scale);maxx=std::max(maxx,x+(g.xOffset+g.width)*scale-1);miny=std::min(miny,y+g.yOffset*scale);maxy=std::max(maxy,y+(g.yOffset+g.height)*scale-1);}x+=g.xAdvance*scale;
      }else if(!face){minx=std::min(minx,x);maxx=std::max(maxx,x+6*scale-1);miny=std::min(miny,y);maxy=std::max(maxy,y+8*scale-1);x+=6*scale;}
    }
    *bx=minx;*by=miny;*bw=maxx>=minx?maxx-minx+1:0;*bh=maxy>=miny?maxy-miny+1:0;
  }
  void print(const char* text){
    int16_t x,y;uint16_t w,h;getTextBounds(text,cx,cy,&x,&y,&w,&h);
    if(w && (x<12 || x+w>388 || y<7 || y+h>289))throw std::runtime_error(std::string("Text exceeds safe bounds: ")+text);
    for(const unsigned char* p=(const unsigned char*)text;*p;++p){
      if(face && *p>=face->first && *p<=face->last){const auto& g=face->glyph[*p-face->first];unsigned bit=0;
        for(int j=0;j<g.height;++j)for(int i=0;i<g.width;++i,++bit)if(face->bitmap[g.bitmapOffset+bit/8] & (0x80>>(bit%8)))for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx)pixel(cx+(i+g.xOffset)*scale+sx,cy+(j+g.yOffset)*scale+sy,color);
        cx+=g.xAdvance*scale;
      }else if(!face){for(int i=0;i<5;++i)for(int j=0;j<8;++j)if(font[*p*5+i]&(1<<j))for(int sy=0;sy<scale;++sy)for(int sx=0;sx<scale;++sx)pixel(cx+i*scale+sx,cy+j*scale+sy,color);cx+=6*scale;}
    }
  }
  void save(const char* path){std::ofstream f(path,std::ios::binary);f<<"P5\n400 300\n255\n";f.write((char*)pixels.data(),pixels.size());}
};
int main(){Raster d;paper::Frame f;f.view=paper::View::Certificate;paper::artwork(d,f);d.save("outputs/epaper_certificate_v06.pgm");
  f.view=paper::View::Reception;std::strcpy(f.tx,"SOS WE NEED HELP");std::strcpy(f.rx,"WE ARE COMING");paper::artwork(d,f);d.save("outputs/epaper_reception_v06.pgm");
  std::cout<<"PASS: production artwork text bounds; certificate and reception rendered at 400 x 300\n";
}
