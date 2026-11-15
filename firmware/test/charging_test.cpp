#include "ChargingPolicy.h"
#include "ButtonGestures.h"
#include <cassert>
#include <cstdio>
using namespace radio;
int main(){
  assert(usbSupply(true,0x20,0x02)==UsbSupply::Present);
  assert(usbSupply(true,0,0x02)==UsbSupply::Absent);
  assert(usbSupply(false,0x20,0x02)==UsbSupply::Unknown);
  assert(usbSupply(true,0x20,0)==UsbSupply::Unknown); // DIN input must not masquerade as USB.
  assert(usbSupply(true,0x20,0x22)==UsbSupply::Unknown); // Own OTG output is not charging.
  for(unsigned reg=0;reg<256;++reg){assert((powerKeyConfig(reg,true)^reg)==(reg&2));assert((powerKeyConfig(reg,false)^reg)==((~reg)&2));}
  UsbFilter f;assert(f.update(UsbSupply::Present,100)==UsbSupply::Unknown);
  assert(f.update(UsbSupply::Present,499)==UsbSupply::Unknown);
  assert(f.update(UsbSupply::Present,500)==UsbSupply::Present);
  f.update(UsbSupply::Absent,600);assert(f.update(UsbSupply::Present,700)==UsbSupply::Present); // cable bounce
  assert(f.update(UsbSupply::Unknown,701)==UsbSupply::Unknown); // failed reads never claim USB
  UsbFilter wrap;wrap.update(UsbSupply::Present,0xffffff00);assert(wrap.update(UsbSupply::Present,144)==UsbSupply::Present);
  ChargingPolicy p;assert(p.update(UsbSupply::Absent,true)==DockAction::None);assert(!p.standby());
  assert(p.update(UsbSupply::Present,false)==DockAction::None);assert(!p.active()); // preserve active story
  assert(p.update(UsbSupply::Present,true)==DockAction::Enter);assert(p.active());
  assert(p.dismiss());assert(!p.active());assert(!p.dismiss());
  for(int i=0;i<100;++i)assert(p.update(UsbSupply::Present,true)==DockAction::None); // dismissal persists
  assert(p.standby());assert(p.active());
  assert(p.update(UsbSupply::Absent,true)==DockAction::Sleep);assert(!p.active());assert(!p.standby());
  assert(p.update(UsbSupply::Present,true)==DockAction::Enter);
  assert(p.update(UsbSupply::Unknown,true)==DockAction::Resume);assert(!p.active());
  assert(p.update(UsbSupply::Present,true)==DockAction::Enter); // recovered readings
  // Top press dismisses immediately and is blocked through hold/release.
  ButtonGestures g;g.reset(true,100);assert(g.update(true,1500)==Gesture::None);
  assert(g.update(false,1501)==Gesture::None);assert(g.update(false,2000)==Gesture::None);
  g.update(true,2100);g.update(false,2200);assert(g.update(false,2550)==Gesture::Click);
  puts("USB charging policy tests passed: source isolation, debounce/failure/wrap, thermal-bit preservation, dock/return/unplug, consumed wake press");
}
