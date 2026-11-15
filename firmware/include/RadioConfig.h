#pragma once
#include <stdint.h>
#ifndef RADIO_EPD_LEGACY
#define RADIO_EPD_LEGACY 0
#endif
// Standard CoreS3 K128 + bare EC11 encoder. These are ESP32 GPIO NUMBERS.
// Do not reuse the Core2/M123 wiring from the old CAD package.
namespace cfg {
constexpr int EPD_MOSI = 17;  // DinBase Port C yellow
constexpr int EPD_SCK  = 18;  // DinBase Port C white
constexpr int EPD_CS   = 5;   // M-Bus pin 8
constexpr int EPD_DC   = 6;   // M-Bus pin 21
constexpr int EPD_RST  = 7;   // M-Bus pin 22
constexpr int EPD_BUSY = 8;   // DinBase Port B white
constexpr int KEY      = 9;   // DinBase Port B yellow, switch to GND
constexpr int ENCODER_A = 2; // Port A yellow, contact to common GND
constexpr int ENCODER_B = 1; // Port A white; red 5V must remain unused
constexpr int ENCODER_DIRECTION = 1;  // change to -1 if rotation is reversed
constexpr int ENCODER_EDGES_PER_STEP = 2; // EC11 variants: serial 'encoder steps 4' if needed
constexpr bool CONTROLS_AUTO_START = true; // User reports both EC11 controls wired/ready.
constexpr uint32_t TUNING_VIEW_MS = 3000; // Return to S-meter after the last turn.
constexpr uint32_t VOLUME_IDLE_MS = 10000; // Save and return to S-meter after inactivity.
constexpr uint32_t PHOTO_VIEW_MS = 10000; // Start counting after the photo's full refresh.
constexpr uint8_t VOLUME = 204;       // 0..255; user-requested 80% default
constexpr uint16_t DOT_MS = 120;      // 10 WPM, standard PARIS timing
constexpr uint16_t TX_HZ = 650;
constexpr uint16_t RX_HZ = 800;
constexpr uint32_t REPLAY_DELAY_MS = 10000; // Hold the final rendered reply, then show certificate.
constexpr int NEXT_ROUND_LOW = 470, NEXT_ROUND_HIGH = 530;
// User confirmed the wired primary display works; restore it after every reset.
constexpr bool EPD_AUTO_START = !RADIO_EPD_LEGACY;
constexpr uint32_t PARTIAL_INTERVAL_MS = 650; // minimum START-to-START cadence
constexpr int FULL_REFRESH_EVERY = 40; // each new round also gets a full refresh
// Controls and primary ePaper start automatically after setup.
// ePaper driver selection below is NOT automatic detection.
}
static_assert(cfg::ENCODER_EDGES_PER_STEP==2 || cfg::ENCODER_EDGES_PER_STEP==4, "Use two or four edges per detent");
