// Export the real firmware Morse encoder/configuration for the browser preview.
#include "RadioConfig.h"
#include "RadioLogic.h"
#include "MeterMotion.h"
#include <iostream>
int main() {
  std::cout << "{\"dotMs\":" << cfg::DOT_MS << ",\"txHz\":" << cfg::TX_HZ
            << ",\"rxHz\":" << cfg::RX_HZ << ",\"replayDelayMs\":" << cfg::REPLAY_DELAY_MS
            << ",\"tuningViewMs\":" << cfg::TUNING_VIEW_MS
            << ",\"nextRoundLow\":" << cfg::NEXT_ROUND_LOW << ",\"nextRoundHigh\":" << cfg::NEXT_ROUND_HIGH
            << ",\"meterTx\":" << radio::METER_TX << ",\"meterRx\":" << radio::METER_RX
            << ",\"meterMax\":" << radio::METER_MAX << ",\"meterRiseMs\":" << radio::METER_RISE_MS
            << ",\"meterDecayMs\":" << radio::METER_DECAY_MS << ",\"messages\":[";
  bool firstMessage=true;
  for(const char* message:{"SOS WE NEED HELP","WE ARE COMING"}) {
    if(!firstMessage)std::cout << ',';firstMessage=false;
    std::cout << "{\"text\":\"" << message << "\",\"events\":[";
    bool first=true;
    for(auto e:radio::encode(message)) {
      if(!first)std::cout << ',';first=false;
      std::cout << '[' << int(e.tone) << ',' << int(e.units) << ',' << int(e.wordFinished) << ',' << int(e.charsFinished) << ']';
    }
    std::cout << "]}";
  }
  std::cout << "]}\n";
}
