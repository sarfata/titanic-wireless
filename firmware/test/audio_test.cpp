#include "SoftTone.h"
#include "RadioConfig.h"
#include <cassert>
#include <vector>
#include <iostream>
int main(){
  static_assert(cfg::DOT_MS==120,"10 WPM required");
  for(auto hz:{cfg::TX_HZ,cfg::RX_HZ})for(int units:{1,3}){
    const size_t n=audio::RATE*cfg::DOT_MS*units/1000;
    std::vector<int16_t> pcm(n);audio::render(pcm.data(),n,hz);
    assert(pcm.front()==0&&pcm.back()==0);
    int crossings=0;for(size_t i=audio::RAMP+1;i<n-audio::RAMP;++i)if(pcm[i-1]<=0&&pcm[i]>0)++crossings;
    const double measured=crossings*double(audio::RATE)/(n-2*audio::RAMP);
    assert(std::abs(measured-hz)<12);
    double edge=0,body=0;
    for(size_t i=0;i<audio::RAMP/2;++i)edge+=double(pcm[i])*pcm[i];
    for(size_t i=audio::RAMP;i<audio::RAMP+audio::RAMP/2;++i)body+=double(pcm[i])*pcm[i];
    assert(edge<body*.2);
  }
  std::cout<<"PASS: 10 WPM, both pitches, zero endpoints, softened attack energy\n";
}
