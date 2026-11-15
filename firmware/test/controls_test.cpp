#include "ButtonGestures.h"
#include "VolumeControl.h"
#include "PhotoDither.h"
#include <cassert>
#include <iostream>
#include <vector>
int main(){
  using radio::Gesture;radio::ButtonGestures b;
  b.reset(false,0);assert(b.update(true,10)==Gesture::None);
  assert(b.update(false,60)==Gesture::None);assert(b.update(false,409)==Gesture::None);
  assert(b.update(false,410)==Gesture::Click);assert(b.update(false,999)==Gesture::None);
  b.reset(false,0);
  for(int i=0;i<3;++i){assert(b.update(true,10+i*150)==Gesture::None);assert(b.update(false,60+i*150)==(i==2?Gesture::Triple:Gesture::None));}
  assert(b.update(false,1000)==Gesture::None);
  b.reset(false,0);b.update(true,10);assert(b.update(true,909)==Gesture::None);
  assert(b.update(true,910)==Gesture::Hold);assert(b.update(true,2000)==Gesture::None);
  assert(b.update(false,2100)==Gesture::None);assert(b.update(false,2500)==Gesture::None);
  b.reset(true,0);assert(b.update(true,5000)==Gesture::None);b.update(false,5001);
  b.update(true,5010);b.update(false,5060);assert(b.update(false,5410)==Gesture::Click);
  b.reset(false,0);b.update(true,10);b.update(false,60);b.update(true,160);b.update(false,200);
  assert(b.update(false,550)==Gesture::Click);assert(b.update(false,900)==Gesture::None);
  b.reset(false,UINT32_MAX-100);b.update(true,UINT32_MAX-90);b.update(false,UINT32_MAX-50);
  assert(b.update(false,298)==Gesture::None);assert(b.update(false,299)==Gesture::Click);
  radio::VolumeControl v;v.enter(0);v.turn(1,100);assert(v.percent==85);
  assert(!v.settled(99)); // This loop's clock can predate an encoder callback.
  assert(!v.settled(749));assert(v.settled(750));assert(!v.settled(751));
  v.turn(20,800);assert(v.percent==100);v.turn(-30,900);assert(v.percent==0);
  assert(!v.settled(1450));assert(v.settled(1550));
  v.turn(10,UINT32_MAX-100);assert(!v.settled(548));assert(v.settled(549));
  v.turn(1,600);v.leave();assert(!v.enabled&&!v.settled(2000));
  assert(!v.expired(20000,10000));
  v.enter(100);assert(!v.expired(99,10000));assert(!v.expired(10099,10000));assert(v.expired(10100,10000));
  v.enter(100);v.turn(1,9000);assert(!v.expired(10100,10000));assert(!v.expired(18999,10000));assert(v.expired(19000,10000));
  v.percent=100;v.turn(1,19500);assert(v.percent==100&&!v.expired(20000,10000));assert(v.expired(29500,10000));
  v.enter(UINT32_MAX-5000);assert(!v.expired(4998,10000));assert(v.expired(4999,10000));
  v.leave();assert(v.percent==100&&!v.expired(10000,10000));
  for(int p=0;p<=100;++p)assert(radio::volumeToPercent(radio::percentToVolume(p))==p);
  std::vector<uint8_t> out(photo::BYTES+2,0x5a);uint8_t rgb[3]={255,255,255};
  assert(photo::dither(rgb,1,1,out.data()+1));for(int i=1;i<=photo::BYTES;++i)assert(out[i]==0);
  rgb[0]=rgb[1]=rgb[2]=0;assert(photo::dither(rgb,1,1,out.data()+1));
  for(int i=1;i<=photo::BYTES;++i)assert(out[i]==255);
  rgb[0]=rgb[1]=rgb[2]=128;assert(photo::dither(rgb,1,1,out.data()+1));
  int black=0;for(int i=1;i<=photo::BYTES;++i)black+=__builtin_popcount(out[i]);
  assert(black>57000&&black<63000);assert(out.front()==0x5a&&out.back()==0x5a);
  assert(!photo::dither(nullptr,1,1,out.data()));assert(!photo::dither(rgb,0,1,out.data()));
  std::cout<<"PASS: single/triple/hold, no release double-trigger, boot-held, double click, clock wrap; volume range/settle/save timing; photo extremes/halftone/guards\n";
}
