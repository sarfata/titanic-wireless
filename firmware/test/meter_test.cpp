#include "MeterMotion.h"
#include "RadioConfig.h"
#include "RadioLogic.h"
#include <cassert>
#include <iostream>
struct Key {bool down=false;};
void keyed(const radio::Event& e,uint32_t,void* context){if(!e.wordFinished)static_cast<Key*>(context)->down=e.tone;}
int main(){
 assert(radio::meterTarget(true,false,true,0)==10);
 assert(std::abs(radio::meterTarget(false,true,true,100)-5.6f)<.001f);
 assert(radio::meterTarget(false,true,true,0)==0);
 assert(radio::meterTarget(true,false,false,100)==0);
 for(bool tx:{true,false}){
  Key key;radio::Player player;player.start(tx?"SOS WE NEED HELP":"WE ARE COMING",cfg::DOT_MS,0,keyed,&key);
  float needle=0,peak=0;int keyedFrames=0,gapFrames=0;
  for(uint32_t now=0;now<40000;now+=33){
   player.tick(now);if(!player.active())key.down=false;
   const float target=radio::meterTarget(tx,!tx,key.down,100),previous=needle;
   needle=radio::meterStep(needle,target,33);peak=std::max(peak,needle);
   if(key.down){assert(needle>=previous);++keyedFrames;}
   else if(player.active()){assert(needle<=previous);++gapFrames;}
  }
  assert(keyedFrames>20&&gapFrames>20);assert(peak>(tx?9.8f:5.4f)&&peak<=(tx?10:5.6f));assert(needle<.001f);
 }
 float needle=10;needle=radio::meterStep(needle,0,120);assert(needle>8&&needle<10);
 needle=radio::meterStep(needle,0,840);assert(needle>2&&needle<3);
 needle=radio::meterStep(needle,0,4000);assert(needle<.01f);
 assert(radio::meterAngle(10)>radio::meterAngle(12));
 std::cout<<"PASS: real Morse events drive TX10/RX5.6, keying/gap motion, slow silence decay, overrange headroom\n";
}
