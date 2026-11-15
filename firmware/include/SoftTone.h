#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace audio {
constexpr uint32_t RATE=24000;
constexpr size_t RAMP=RATE*6/1000; // 6 ms raised-cosine keying, both ends.
inline void render(int16_t* samples,size_t count,uint16_t hz){
  for(size_t i=0;i<count;++i){
    float envelope=1.0f;
    if(i<RAMP)envelope=.5f-.5f*std::cos(3.14159265358979323846f*i/RAMP);
    if(count-1-i<RAMP)envelope=.5f-.5f*std::cos(3.14159265358979323846f*(count-1-i)/RAMP);
    samples[i]=int16_t(30000*envelope*std::sin(6.28318530717958647692f*hz*i/RATE));
  }
}
}
