#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include <LittleFS.h>
#include <esp_camera.h>
#include <img_converters.h>
#include <esp_partition.h>
#include "PhotoDither.h"
#include "BoardPower.h"
namespace photo {
inline bool storageReady(){
  static bool ready=false;if(ready)return true;
  if(LittleFS.begin(false)){ready=true;return true;}
  // Never format a nonempty/unrecognized filesystem. Only initialize erased space.
  const auto* p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");
  if(!p)return false;
  uint8_t bytes[512];
  for(size_t offset=0;offset<p->size;offset+=sizeof(bytes)){
    if(esp_partition_read(p,offset,bytes,sizeof(bytes))!=ESP_OK)return false;
    for(auto byte:bytes)if(byte!=0xff){Serial.println("PHOTO storage not blank; refusing automatic format");return false;}
  }
  if(!LittleFS.format()||!LittleFS.begin(false))return false;
  Serial.println("PHOTO initialized empty flash filesystem");ready=true;return true;
}
inline bool read(const char* path,uint8_t* bits){
  auto f=LittleFS.open(path,"r");if(!f)return false;
  String magic=f.readStringUntil('\n'),size=f.readStringUntil('\n');
  return magic=="P4"&&size=="400 300"&&f.read(bits,BYTES)==BYTES;
}
inline bool capture(uint8_t* bits){
  boardpower::CameraSession power;
  camera_config_t c{};
  c.pin_pwdn=-1;c.pin_reset=-1;c.pin_xclk=-1;c.pin_sccb_sda=12;c.pin_sccb_scl=11;
  c.pin_d0=39;c.pin_d1=40;c.pin_d2=41;c.pin_d3=42;c.pin_d4=15;c.pin_d5=16;c.pin_d6=48;c.pin_d7=47;
  c.pin_vsync=46;c.pin_href=38;c.pin_pclk=45;c.xclk_freq_hz=20000000;
  c.ledc_timer=LEDC_TIMER_0;c.ledc_channel=LEDC_CHANNEL_0;
  c.pixel_format=PIXFORMAT_RGB565;c.frame_size=FRAMESIZE_QVGA;c.fb_count=1;
  c.fb_location=CAMERA_FB_IN_PSRAM;c.grab_mode=CAMERA_GRAB_WHEN_EMPTY;c.sccb_i2c_port=-1;
  // CoreS3 camera shares the internal control bus, not the external EC11 pins.
  M5.In_I2C.release();
  esp_err_t err=esp_camera_init(&c);
  if(err!=ESP_OK){esp_camera_deinit();M5.In_I2C.begin();Serial.printf("PHOTO camera init failed: 0x%x\n",err);return false;}
  bool ok=false;
  // Let exposure/white balance settle; discard the queued first frames.
  for(int i=0;i<5;++i){auto* frame=esp_camera_fb_get();if(frame)esp_camera_fb_return(frame);delay(60);}
  auto* frame=esp_camera_fb_get();
  if(frame){
    auto* rgb=static_cast<uint8_t*>(ps_malloc(frame->width*frame->height*3));
    if(rgb){
      ok=fmt2rgb888(frame->buf,frame->len,frame->format,rgb)&&dither(rgb,frame->width,frame->height,bits);
      free(rgb);
    }
    Serial.printf("PHOTO captured %u x %u, %u bytes; converted=%d\n",unsigned(frame->width),unsigned(frame->height),unsigned(frame->len),ok);
    esp_camera_fb_return(frame);
  }
  esp_camera_deinit();M5.In_I2C.begin();return ok;
}
inline bool save(const char* path,const uint8_t* bits){
  if(LittleFS.exists(path)||LittleFS.totalBytes()-LittleFS.usedBytes()<BYTES+8192)return false;
  String temporary=String(path)+".tmp";
  auto f=LittleFS.open(temporary,"w");if(!f)return false;
  bool ok=f.print("P4\n400 300\n")==11&&f.write(bits,BYTES)==BYTES;
  f.close();
  if(ok)ok=LittleFS.rename(temporary,path);
  if(!ok)LittleFS.remove(temporary); // Only our incomplete new file, never an old photo.
  return ok;
}
}
