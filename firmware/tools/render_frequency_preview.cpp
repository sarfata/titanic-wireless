// Render exactly the production frequency artwork without ESP32 hardware.
#include "FrequencyFace.h"
#include "VolumeFace.h"
#include "ChargingFace.h"
#include "StartupFace.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
struct Canvas {
  std::vector<uint16_t> pixels=std::vector<uint16_t>(224*224);
  void pixel(int x,int y,uint16_t color){if(x>=0&&y>=0&&x<224&&y<224)pixels[y*224+x]=color;}
  void fillScreen(uint16_t color){std::fill(pixels.begin(),pixels.end(),color);}
  void fillRect(int x,int y,int w,int h,uint16_t color){for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)pixel(i,j,color);}
  void fillCircle(int x,int y,int r,uint16_t color){
    for(int j=-r;j<=r;++j){int dx=int(std::sqrt(r*r-j*j));fillRect(x-dx,y+j,dx*2+1,1,color);}
  }
  void drawLine(int x,int y,int xx,int yy,uint16_t color){
    int dx=std::abs(xx-x),sx=x<xx?1:-1,dy=-std::abs(yy-y),sy=y<yy?1:-1,err=dx+dy;
    for(;;){pixel(x,y,color);if(x==xx&&y==yy)break;int e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
  }
  void save(const std::string& path){
    std::ofstream f(path,std::ios::binary);f<<"P6\n224 224\n255\n";
    for(auto p:pixels){char rgb[3]={char(((p>>11)&31)*255/31),char(((p>>5)&63)*255/63),char((p&31)*255/31)};f.write(rgb,3);}
  }
};
int main(int argc,char** argv){
  if(argc!=2&&argc!=3)return 1;
  Canvas c;
  if(argc==3&&std::string(argv[2])=="startup"){
    for(unsigned phase=0;phase<48;++phase){radio::startupFace(c,phase);c.save(std::string(argv[1])+"/"+std::to_string(phase)+".ppm");}
    return 0;
  }
  if(argc==3&&std::string(argv[2])=="charging"){
    for(unsigned phase=0;phase<32;++phase){radio::chargingFace(c,phase);c.save(std::string(argv[1])+"/"+std::to_string(phase)+".ppm");}
    return 0;
  }
  for(int f=450;f<=550;++f){radio::frequencyFace(c,f);c.save(std::string(argv[1])+"/"+std::to_string(f)+".ppm");}
  for(int v=0;v<=100;++v){radio::volumeFace(c,v);c.save(std::string(argv[1])+"/volume-"+std::to_string(v)+".ppm");}
}
