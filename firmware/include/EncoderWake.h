#pragma once
#include <stdint.h>
namespace radio {
// S3 EXT1 shares one polarity across its pins. Select contacts whose current
// level is opposite the wake level, including encoders that rest at 00 or 11.
// Bits match Quadrature: A=2, B=1. EXT0 independently handles the top button.
struct EncoderWake {uint8_t phases;bool high;};
constexpr EncoderWake encoderWake(uint8_t ab){
  return (ab&3)?EncoderWake{uint8_t(ab&3),false}:EncoderWake{3,true};
}
}
