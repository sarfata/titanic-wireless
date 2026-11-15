#pragma once
#include <stdint.h>
namespace radio {
enum class UsbSupply {Unknown,Absent,Present};
// CoreS3 schematic: BUS_OUT_EN=1 isolates the external bus input; OTG must be off.
// VBUS alone is ambiguous if the DIN/barrel input can also feed the PMIC.
constexpr UsbSupply usbSupply(bool readsOk,uint8_t pmicStatus,uint8_t output0){
  return !readsOk||(output0&0x22)!=0x02?UsbSupply::Unknown:
         (pmicStatus&0x20)?UsbSupply::Present:UsbSupply::Absent;
}
constexpr uint8_t powerKeyConfig(uint8_t old,bool usb){
  return usb?uint8_t(old&~0x02):uint8_t(old|0x02); // Only PWRON cutoff enable; preserve thermal protection.
}
class UsbFilter {
 public:
  UsbSupply update(UsbSupply sample,uint32_t now){
    if(sample==UsbSupply::Unknown){candidate_=stable_=sample;since_=now;return stable_;}
    if(sample!=candidate_){candidate_=sample;since_=now;}
    if(uint32_t(now-since_)>=400)stable_=candidate_;
    return stable_;
  }
  UsbSupply value()const{return stable_;}
 private:
  UsbSupply candidate_=UsbSupply::Unknown,stable_=UsbSupply::Unknown;
  uint32_t since_=0;
};
enum class DockAction {None,Enter,Resume,Sleep};
class ChargingPolicy {
 public:
  DockAction update(UsbSupply supply,bool mayEnter){
    if(supply!=usb_){usb_=supply;pending_=supply==UsbSupply::Present;}
    if(active_&&usb_!=UsbSupply::Present){
      active_=false;return usb_==UsbSupply::Absent?DockAction::Sleep:DockAction::Resume;
    }
    if(pending_&&mayEnter){pending_=false;active_=true;return DockAction::Enter;}
    return DockAction::None;
  }
  bool dismiss(){if(!active_)return false;active_=pending_=false;return true;}
  bool standby(){pending_=false;if(usb_!=UsbSupply::Present)return false;active_=true;return true;}
  bool active()const{return active_;}
  bool usb()const{return usb_==UsbSupply::Present;}
 private:
  UsbSupply usb_=UsbSupply::Unknown;
  bool active_=false,pending_=false;
};
}
