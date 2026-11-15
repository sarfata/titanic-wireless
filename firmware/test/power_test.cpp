#include "PowerPolicy.h"
#include <assert.h>
#include <stdio.h>
int main(){
  radio::PowerPolicy p;p.activity(100);
  assert(!p.sleep(15100,false,true,false));
  assert(!p.sleep(60099,false,true,false));
  assert(p.sleep(60100,false,true,false)); // idle and abandoned tuning both use 60 s
  assert(!p.sleep(200000,true,true,false)); // audio, photo, volume, final hold
  assert(!p.sleep(200000,false,false,false)); // no working wake key
  assert(!p.sleep(200000,false,true,true)); // held key / immediate wake loop
  p.activity(200000);assert(!p.sleep(200001,false,true,false));
  assert(p.idleMs(199999)==0); // stale clock captured before a callback
  p.activity(0xfffffff0);assert(p.sleep(60000,false,true,false));
  assert(!p.sleep(1000,false,true,false));
  p.enabled=false;assert(!p.sleep(200000,false,true,false));
  puts("Power policy tests passed");
}
