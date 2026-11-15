#include "TuningView.h"
#include "RadioConfig.h"
#include "RadioLogic.h"
#include <cassert>
#include <iostream>
int main(){
  radio::TuningView v;
  constexpr uint32_t timeout=cfg::TUNING_VIEW_MS;
  static_assert(timeout==3000,"Three-second tuning view");
  static_assert(cfg::CONTROLS_AUTO_START,"Ready controls start on battery too");
  assert(!v.active(0,timeout));
  v.rotated(0);assert(v.active(0,timeout));assert(v.active(2999,timeout));assert(!v.active(3000,timeout));
  v.rotated(5000);v.rotated(7999);assert(v.active(10998,timeout));assert(!v.active(10999,timeout));
  v.rotated(UINT32_MAX-1000);assert(v.active(1998,timeout));assert(!v.active(1999,timeout));
  assert(!v.active(UINT32_MAX-1000,timeout)); // expired view cannot reappear on timer wrap
  v.rotated(100);v.reset();assert(!v.active(101,timeout));
  // A switch held at boot does not send. Release, then a debounced press sends once.
  radio::Debounce key;key.reset(true,0);assert(!key.update(true,1000));
  assert(!key.update(false,1001));assert(!key.update(false,1026));
  assert(!key.update(true,1100));assert(!key.update(false,1105));assert(!key.update(true,1109));
  assert(!key.update(true,1133));assert(key.update(true,1134));assert(!key.update(true,10000));
  std::cout<<"PASS: tuning view at boot, zero timestamp, exact 3 s expiry, repeated turns, rollover, cancellation; held/bouncing top switch\n";
}
