#pragma once
#include <M5Unified.h>
#include <utility/imu/BMI270_Class.hpp>
#include <esp32-hal-cpu.h>
#include <driver/rtc_io.h>
#include "RadioConfig.h"
#include "ChargingPolicy.h"
namespace boardpower {
constexpr uint8_t BRIGHTNESS=96;
constexpr uint8_t CHARGING_BRIGHTNESS=48;
constexpr uint32_t CPU_MHZ=80;
struct UsbReading {
  bool ok=false;uint8_t pmic=0,output0=0;
  radio::UsbSupply supply()const{return radio::usbSupply(ok,pmic,output0);}
};
inline UsbReading readUsb(){
  UsbReading r;
  r.ok=M5.In_I2C.readRegister(0x34,0x00,&r.pmic,1,400000);
  r.ok=M5.In_I2C.readRegister(0x58,0x02,&r.output0,1,400000)&&r.ok;
  return r;
}
inline bool configurePowerButton(bool usb){
  // AXP2101 REG22 bit1 only. USB: let software handle the POWER hold.
  // Battery/unknown/deep sleep: restore hardware cutoff. Never alter bit2
  // (thermal shutdown), charger current/voltage or power-path routing.
  uint8_t old=0,check=0;
  if(!M5.In_I2C.readRegister(0x34,0x22,&old,1,400000))return false;
  const auto wanted=radio::powerKeyConfig(old,usb);
  if(old==wanted)return true;
  return M5.In_I2C.writeRegister8(0x34,0x22,wanted,400000)
      &&M5.In_I2C.readRegister(0x34,0x22,&check,1,400000)&&check==wanted;
}
constexpr int PAPER_OUTPUTS[]={cfg::EPD_MOSI,cfg::EPD_SCK,cfg::EPD_CS,cfg::EPD_DC,cfg::EPD_RST};
inline void releaseSleepPins(){
  for(int pin:PAPER_OUTPUTS){rtc_gpio_hold_dis(gpio_num_t(pin));rtc_gpio_deinit(gpio_num_t(pin));}
  for(int pin:{cfg::EPD_BUSY,cfg::KEY,cfg::ENCODER_A,cfg::ENCODER_B}){
    rtc_gpio_hold_dis(gpio_num_t(pin));rtc_gpio_deinit(gpio_num_t(pin));
  }
}
inline void prepareDeepSleep(){
  // Called only after the paper worker has hibernated and stopped all SPI I/O.
  // Hold every outbound signal low so an unpowered module cannot be back-fed.
  for(int pin:PAPER_OUTPUTS){
    const auto gpio=gpio_num_t(pin);
    rtc_gpio_init(gpio);rtc_gpio_set_direction(gpio,RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pullup_dis(gpio);rtc_gpio_pulldown_dis(gpio);
    rtc_gpio_set_level(gpio,0);rtc_gpio_hold_en(gpio);
  }
  rtc_gpio_isolate(gpio_num_t(cfg::EPD_BUSY));
  // CoreS3 schematic: DCDC3 = external M-Bus VCC_3V3; DCDC1 = core VDD_3V3.
  // Never disable DCDC1: the RTC wake circuitry needs it.
  M5.In_I2C.bitOff(0x34,0x80,0x04,400000);
  M5.In_I2C.bitOff(0x34,0x90,0x01,400000); // ALDO1: amplifier 1.8 V
  M5.In_I2C.bitOff(0x58,0x02,0x01,400000); // Touch controller held in reset
  Serial.printf("POWER parked external_3V3=%d amp_rail=%d; 5V/charging routing retained\n",
    !!(M5.In_I2C.readRegister8(0x34,0x80,400000)&0x04),M5.Power.Axp2101.getALDO1Enabled());
}
// M5Unified's BMI270 driver has no sleep override. Use its existing AUX bus
// helpers to park the attached BMM150 before disabling the BMI sensors.
class UnusedImu : public m5::BMI270_Class {
 public:
  bool park(){
    if(WhoAmI()!=0x24)return false;
    bool ok=auxSetupMode(0x10)&&auxWriteRegister8(0x4B,0x00);
    ok=writeRegister8(PWR_CTRL_ADDR,0x00)&&ok;
    ok=writeRegister8(PWR_CONF_ADDR,0x01)&&ok;
    return ok&&readRegister8(PWR_CTRL_ADDR)==0;
  }
};
inline void cameraOff(){
  M5.In_I2C.bitOff(0x58,0x03,0x01,400000); // AW9523 P1_0: camera reset, active low
  // Schematic J2: ALDO3 supplies I/O + oscillator; BLDO1/2 supply AVDD/DVDD.
  // Switch all three rails together, retaining their factory voltage settings.
  M5.In_I2C.bitOff(0x34,0x90,0x34,400000);
}
inline void cameraOn(){
  // The ES7210 shares SCCB/I2C; power its interface during camera transactions.
  M5.Power.Axp2101.setALDO2(3300);
  M5.In_I2C.bitOn(0x34,0x90,0x34,400000);
  delay(20);
  M5.In_I2C.bitOn(0x58,0x03,0x01,400000);
  delay(50);
}
// Also restores power on camera failure paths; control I2C must be restored first.
struct CameraSession {
  uint32_t previousMHz=getCpuFrequencyMhz();
  CameraSession(){setCpuFrequencyMhz(240);cameraOn();}
  ~CameraSession(){cameraOff();M5.Power.Axp2101.setALDO2(0);setCpuFrequencyMhz(previousMHz);}
};
inline void begin(){
  configurePowerButton(false); // Clear a previous USB session's setting after reset.
  M5.In_I2C.bitOn(0x34,0x80,0x04,400000); // Restore external 3V3 after deep sleep.
  M5.In_I2C.bitOff(0x58,0x02,0x04,400000); // Assert amp reset before the first tone.
  M5.Power.Axp2101.setALDO2(0); // unused ES7210 microphones
  M5.Power.Axp2101.setALDO4(0); // unused microSD; photos use LittleFS
  cameraOff();
  UnusedImu imu;
  Serial.printf("POWER unused IMU park=%d\n",imu.park());
  // Keep 5V bus/charging routing unchanged. Neither Wi-Fi nor Bluetooth starts.
  setCpuFrequencyMhz(CPU_MHZ);
}
}
