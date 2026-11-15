#include "RadioLogic.h"
#include <cassert>
#include <iostream>
using namespace radio;
struct Seen {int tones=0,words=0;uint32_t lastDuration=0;};
void onEvent(const Event&e,uint32_t d,void* p){auto&s=*static_cast<Seen*>(p);s.tones+=e.tone;s.words+=e.wordFinished>0;s.lastDuration=d;}
int main(){
  auto sos=encode("SOS");int tones=0,total=0,gaps=0;
  for(auto e:sos){tones+=e.tone;total+=e.units;if(!e.tone&&e.units){assert(e.units==1);++gaps;}}
  assert(tones==9&&gaps==8&&total==23); // 15 keyed units + 8 inter-element gaps
  auto words=encode("E E");assert(words.size()==5);assert(words[2].units==7);
  auto letters=encode("EE");assert(letters.size()==4&&letters[1].units==3);
  // Standard PARIS word, including its following word space, is 50 dit units.
  total=0;for(auto e:encode("PARIS"))total+=e.units;assert(total+7==50);
  assert(counterDelta(0x8000,0x7fff)==1&&counterDelta(0xffff,0)==-1);
  assert(counterDelta(0,0xffff)==1&&counterDelta(0x7fff,0x8000)==-1);
  Player p;Seen s;p.start("ET",100,0xfffffff0u,onEvent,&s);
  assert(p.active()&&s.tones==1);p.tick(0x53);assert(s.tones==1);
  p.tick(0x54);p.tick(0x180);assert(s.tones==2&&s.lastDuration==300);
  p.tick(0x2ac);assert(!p.active()&&s.words==1);
  p.start("SOS",100,0,onEvent,&s);p.stop();assert(!p.active());
  Debounce key;key.reset(false,0);assert(!key.update(true,5));assert(!key.update(false,10));
  assert(!key.update(true,15));assert(!key.update(true,39));assert(key.update(true,40));assert(!key.update(true,60));
  assert(prefixWords("SOS WE NEED HELP",2)=="SOS WE");
  assert(signalStrength(500)==100&&signalStrength(475)==0&&signalStrength(525)==0);
  std::cout<<"PASS: Morse/prosign timing, word gaps, clock/counter rollover, debounce, tuning\n";
}
