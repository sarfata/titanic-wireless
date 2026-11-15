#pragma once
#include <cmath>
namespace radio {
// Game meter units: 0..10 normal range, 11/12 decorative +20/+40 overrange.
constexpr float METER_MAX=12, METER_TX=10, METER_RX=5.6f;
constexpr float METER_RISE_MS=25, METER_DECAY_MS=650;
inline float meterTarget(bool tx,bool rx,bool keyed,int strength){
  if(!keyed)return 0;
  return tx?METER_TX:(rx?METER_RX*strength/100.0f:0);
}
inline float meterStep(float reading,float target,float elapsedMs){
  const float tau=target>reading?METER_RISE_MS:METER_DECAY_MS;
  return reading+(target-reading)*(1-std::exp(-elapsedMs/tau));
}
inline float meterAngle(float reading){return 155-reading*(130/METER_MAX);}
}
