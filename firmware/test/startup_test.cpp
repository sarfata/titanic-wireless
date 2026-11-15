#include "StartupSequence.h"
#include <cassert>
#include <cstdio>
int main(){
  using radio::StartupResult;radio::StartupSequence intro;
  paper::Frame target{},shown{};target.generation=4;target.view=paper::View::Certificate;
  intro.begin(100,target,false);
  assert(intro.bootAnimation());
  assert(intro.update(2899,true,target)==StartupResult::Waiting); // even fast paper gets a prelude
  assert(intro.update(2900,true,shown)==StartupResult::Waiting); // stale generation/view
  assert(intro.update(4000,true,target)==StartupResult::Ready&&!intro.transmit()); // boot never plays
  assert(!intro.bootAnimation());
  intro.begin(5000,target,true);shown=target;--shown.generation;
  assert(!intro.bootAnimation()); // a button-start keeps the meter even while waiting
  assert(intro.update(9000,true,shown)==StartupResult::Waiting);
  shown=target;shown.view=paper::View::Reception;
  assert(intro.update(9000,true,shown)==StartupResult::Waiting);
  assert(intro.update(9001,true,target)==StartupResult::Ready&&intro.transmit());
  intro.begin(10000,target,true);intro.cancel();
  assert(!intro.bootAnimation());
  assert(intro.update(13000,true,target)==StartupResult::Waiting&&!intro.transmit());
  intro.begin(0,target,true);
  assert(intro.update(20000,true,shown)==StartupResult::TimedOut&&!intro.transmit());
  intro.begin(0,target,false);assert(intro.update(2800,false,shown)==StartupResult::Ready);
  intro.begin(0xffffff00,target,true);assert(intro.update(2600,true,target)==StartupResult::Ready);
  puts("Startup tests passed: minimum prelude, exact paper acknowledgment, silent boot, deliberate transmit, cancellation, timeout, absent paper and timer wrap");
}
