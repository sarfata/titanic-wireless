#include <Arduino.h>
#include <GxEPD2_BW.h> // Adafruit font types must be declared before M5GFX.
#include <M5Unified.h>
#include <SPI.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "RadioConfig.h"
#include "RadioLogic.h"
#include "Quadrature.h"
#include "PaperReveal.h"
#include "PaperViews.h"
#include "PaperArtwork.h"
#include "PatrickHand30.h"
#include "MeterFace.h"
#include "SoftTone.h"
#include "MeterMotion.h"
#include "TuningView.h"
#include "FrequencyFace.h"
#include "ButtonGestures.h"
#include "VolumeControl.h"
#include "VolumeFace.h"
#include "ChargingFace.h"
#include "ChargingPolicy.h"
#include "StartupSequence.h"
#include "StartupFace.h"
#include "PhotoCapture.h"
#include <Preferences.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/rtc_io.h>
#include "PowerPolicy.h"
#include "EncoderWake.h"

#if RADIO_EPD_LEGACY
using EpdDriver=GxEPD2_420; // GDEW042T2, UC8176/IL0398
#else
using EpdDriver=GxEPD2_420_GDEY042T81; // GDEY042T81, SSD1683
#endif
// Dedicated HSPI (SPI3 on ESP32-S3); LCD uses the other hardware SPI host.
SPIClass epdSPI(HSPI);
GxEPD2_BW<EpdDriver,EpdDriver::HEIGHT> epd(EpdDriver(cfg::EPD_CS,cfg::EPD_DC,cfg::EPD_RST,cfg::EPD_BUSY));
using PaperFrame=paper::Frame;
bool paperTaskStarted=false;
QueueHandle_t paperQueue=nullptr;
struct PaperPresented {PaperFrame frame;uint32_t at;};
QueueHandle_t paperAckQueue=nullptr;
PaperPresented paperPresented{};
paper::Views paperViews;
std::atomic<bool> paperEnabled{false},paperWorking{false},paperTimeout{false};
// Worker owns the panel, including shutdown. It acknowledges only after finishing
// queued text and hibernating, then performs no I/O until the request is released.
std::atomic<bool> paperSleepRequested{false},paperParked{false};
std::atomic<uint32_t> paperUpdates{0},paperLastMs{0},paperLastFinishedAt{0};
uint32_t paperGeneration=0;
M5Canvas lcd(&M5.Display);
M5Canvas frequencyLcd(&M5.Display);
radio::TuningView tuningView;
bool frequencyCanvasReady=false;
int drawnMode=-1,drawnValue=-1;
int drawnNeedleX=-999,drawnNeedleY=-999;
radio::PowerPolicy powerPolicy;
radio::UsbFilter usbFilter;
radio::ChargingPolicy charging;
radio::StartupSequence startup;
boardpower::UsbReading usbReading;
uint32_t usbPolledAt=0;
bool powerButtonConfigured=false;
uint32_t audioLastAt=0;
RTC_DATA_ATTR uint32_t retainedMagic=0,deepSleepCount=0;
constexpr uint32_t SLEEP_MAGIC=0x54495410;
bool wokeFromSleep=false,forceStandby=false,preparingSleep=false;
PaperFrame sleepCertificate{};
uint32_t sleepTestSeconds=0;
uint8_t lcdBrightness=boardpower::BRIGHTNESS;
radio::VolumeControl volumeControl;
radio::ButtonGestures keyGestures;
Preferences preferences;
bool settingsReady=false,photoCountdown=false;
int savedVolume=-1,photoNotice=0;
uint32_t photoAt=0,photoNoticeAt=0;
String latestPhoto;
radio::Player player;
// Immutable after setup: Speaker retains pointers during asynchronous playback.
constexpr size_t ditSamples=audio::RATE*cfg::DOT_MS/1000;
int16_t keyedTones[2][ditSamples*4];
float needleStrength=0;
bool morseKeyed=false;
uint32_t meterLastAt=0,meterWindowAt=0,meterFrames=0,meterMaxUs=0;
radio::Debounce keyDebounce;
radio::Quadrature encoderDecoder;
portMUX_TYPE encoderMux=portMUX_INITIALIZER_UNLOCKED;
volatile int encoderPending=0;
volatile uint32_t encoderEdges=0;
int encoderSteps=cfg::ENCODER_EDGES_PER_STEP,encoderDirection=cfg::ENCODER_DIRECTION;
enum class State {Idle,Tx,Seek,Rx,Done};
State state=State::Idle;
const std::string txMessage="SOS WE NEED HELP",rxMessage="WE ARE COMING";
std::string txVisible,rxVisible;
int frequency=470;
uint16_t playPitch=cfg::TX_HZ;
bool encoderEnabled=false,keyEnabled=false,keyMonitor=false,lastManualTone=false;
uint32_t nextScreen=0,lockSince=0;
int nextRoundFrequency=cfg::NEXT_ROUND_HIGH;
bool lockedTimer=false;
String command;

void logStatus();
bool resumeCharging();
void activity(){powerPolicy.activity(millis());paperSleepRequested=false;preparingSleep=false;forceStandby=false;sleepTestSeconds=0;}
const char* paperViewName(paper::View v){return v==paper::View::Photo?"PHOTO":v==paper::View::Certificate?"CERTIFICATE":"RECEPTION";}
void saveVolume(){
  int value=M5.Speaker.getVolume();
  if(settingsReady&&savedVolume!=value){
    if(preferences.putUChar("volume",value)==1){savedVolume=value;Serial.printf("Volume saved: %d/255\n",value);}
    else Serial.println("Volume save FAILED");
  }
}
void volumeSample(){M5.Speaker.playRaw(keyedTones[0],ditSamples,audio::RATE,false,1,0,true);}
PaperFrame paperTarget(bool reveal=true){
  PaperFrame f{};
  snprintf(f.tx,sizeof(f.tx),"%s",txVisible.c_str());
  snprintf(f.rx,sizeof(f.rx),"%s",rxVisible.c_str());
  snprintf(f.photoPath,sizeof(f.photoPath),"%s",latestPhoto.c_str());
  f.generation=paperGeneration;f.reveal=reveal;f.view=paperViews.view;return f;
}
void paperSubmit(bool reveal=true){
  if(!paperEnabled||!paperQueue)return;
  const PaperFrame f=paperTarget(reveal);
  xQueueOverwrite(paperQueue,&f); // latest target; worker reveals every letter while catching up
}
void requestPhoto(){
  ++paperGeneration;
  auto target=paperTarget(false);target.view=paper::View::Photo;
  paperViews.photo(target);paperSubmit(false);
}
void paperBusy(const void*) {delay(1);} // let the audio and main loop run
void drawPaper(const PaperFrame& f){
  if(f.view==paper::View::Photo){
    // Immutable, numbered files avoid races when a later shot is queued.
    static uint8_t* bitmap=static_cast<uint8_t*>(ps_malloc(photo::BYTES));
    epd.fillScreen(GxEPD_WHITE);
    if(bitmap&&photo::read(f.photoPath,bitmap))epd.drawBitmap(0,0,bitmap,photo::WIDTH,photo::HEIGHT,GxEPD_BLACK);
    else{epd.setFont(nullptr);epd.setTextSize(2);epd.setCursor(30,140);epd.print("Photo unavailable");}
  }else paper::artwork(epd,f);
}
void paperTask(void*){
  epdSPI.begin(cfg::EPD_SCK,-1,cfg::EPD_MOSI,cfg::EPD_CS);
  epd.epd2.setBusyCallback(paperBusy);
  paperWorking=true;
  epd.init(0,true,2,false,epdSPI,SPISettings(2000000,MSBFIRST,SPI_MODE0));
  epd.setRotation(0);epd.setTextWrap(false);
  uint32_t refreshStartedAt=0;
  PaperFrame shown{},target{};
  unsigned partials=0;bool first=true,haveTarget=false;
  bool poweredOff=false,forceFull=false;
  paperWorking=false;
  for(;;){
    PaperFrame incoming;
    const bool pending=haveTarget && !paper::caughtUp(shown,target);
    if(paperSleepRequested&&!pending&&uxQueueMessagesWaiting(paperQueue)==0){
      paperWorking=true;epd.hibernate();poweredOff=true;forceFull=true;
      paperWorking=false;paperParked=true;
      while(paperSleepRequested&&uxQueueMessagesWaiting(paperQueue)==0)vTaskDelay(pdMS_TO_TICKS(10));
      paperParked=false;
    }
    if(xQueueReceive(paperQueue,&incoming,pending?0:pdMS_TO_TICKS(100))==pdTRUE){target=incoming;haveTarget=true;}
    else if(!pending){
      if(!poweredOff&&uint32_t(millis()-paperLastFinishedAt.load())>=1000){
        paperWorking=true;epd.powerOff();poweredOff=true;paperWorking=false;
      }
      continue;
    }
    if(!paperEnabled){haveTarget=false;continue;}
    uint32_t elapsed=millis()-refreshStartedAt;
    if(!first && elapsed<cfg::PARTIAL_INTERVAL_MS)vTaskDelay(pdMS_TO_TICKS(cfg::PARTIAL_INTERVAL_MS-elapsed));
    while(xQueueReceive(paperQueue,&incoming,0)==pdTRUE){target=incoming;haveTarget=true;}
    if(!paperEnabled){haveTarget=false;continue;}
    if(!first && paper::caughtUp(shown,target))continue;
    const bool full=first || forceFull || target.generation!=shown.generation || target.view!=shown.view || partials>=cfg::FULL_REFRESH_EVERY;
    PaperFrame f=paper::advance(shown,target,first);
    paperWorking=true;
    if(full)epd.setFullWindow();else epd.setPartialWindow(0,40,400,216);
    refreshStartedAt=millis();
    epd.firstPage();do{drawPaper(f);}while(epd.nextPage());
    poweredOff=false;forceFull=false;
    const uint32_t ms=millis()-refreshStartedAt;
    // SPI panels do not acknowledge pixels; a completed call is not a visual PASS.
    if(ms>12000)paperTimeout=true;
    paperLastMs=ms;paperLastFinishedAt=millis();
    Serial.printf("EPD %s %s request done in %lu ms; TX=[%s] RX=[%s]; visually verify panel\n",full?"FULL":"PARTIAL",paperViewName(f.view),(unsigned long)ms,f.tx,f.rx);
    PaperPresented ack{f,paperLastFinishedAt.load()};xQueueOverwrite(paperAckQueue,&ack);
    shown=f;partials=full?0:partials+1;first=false;
    ++paperUpdates;paperWorking=false;
  }
}
void enablePaper(bool reveal=true,bool submitInitial=true){
  if(paperEnabled){paperSubmit(reveal);return;}
  if(!paperTaskStarted){
    paperQueue=xQueueCreate(1,sizeof(PaperFrame));
    paperAckQueue=xQueueCreate(1,sizeof(PaperPresented));
    if(!paperQueue || !paperAckQueue){
      if(paperQueue)vQueueDelete(paperQueue);
      if(paperAckQueue)vQueueDelete(paperAckQueue);
      paperQueue=paperAckQueue=nullptr;Serial.println("EPD queue allocation failed");return;
    }
    paperEnabled=true;
    if(xTaskCreatePinnedToCore(paperTask,"paper",8192,nullptr,1,nullptr,0)!=pdPASS){
      paperEnabled=false;vQueueDelete(paperQueue);vQueueDelete(paperAckQueue);paperQueue=paperAckQueue=nullptr;Serial.println("EPD task allocation failed");return;
    }
    paperTaskStarted=true;
  }else{
    ++paperGeneration;paperEnabled=true;
    if(state==State::Done)paperViews.complete(paperTarget());
    else if(paperViews.view==paper::View::Photo)paperViews.photo(paperTarget(false));
  }
  if(submitInitial)paperSubmit(reveal);
  Serial.println("EPD enabled; letter-by-letter updates. Confirm pixels visually.");
}
void disablePaper(){
  paperEnabled=false;
  if(paperQueue)xQueueReset(paperQueue);
  Serial.println("EPD paused; any in-progress refresh may finish. epd on resumes; reboot restores primary display.");
}
void onMorse(const radio::Event& e,uint32_t duration,void*){
  morseKeyed=e.tone;
  if(e.charsFinished){
    if(state==State::Tx)txVisible=radio::prefixCharacters(txMessage,e.charsFinished);
    if(state==State::Rx)rxVisible=radio::prefixCharacters(rxMessage,e.charsFinished);
    paperSubmit();Serial.printf("%s: %s\n",state==State::Rx?"RX":"TX",state==State::Rx?rxVisible.c_str():txVisible.c_str());
  }
  if(e.tone){const int pitchIndex=playPitch==cfg::RX_HZ?1:0;
    const bool dah=duration==3*cfg::DOT_MS;
    const int16_t* pcm=keyedTones[pitchIndex]+(dah?ditSamples:0);
    if(!M5.Speaker.playRaw(pcm,ditSamples*(dah?3:1),audio::RATE,false,1,0,true))Serial.println("Speaker tone request FAILED");}
  // Finite PCM ends naturally: stopping on the gap can cut the queued release.
}
void resetGame(bool submit=true){
  startup.cancel();
  player.stop();M5.Speaker.stop();morseKeyed=false;state=State::Idle;txVisible.clear();rxVisible.clear();lockedTimer=false;
  keyMonitor=false;lastManualTone=false;photoCountdown=false;
  // Automatic certificate transition must not shorten a just-started tuning view.
  if(submit)tuningView.reset();
  paperViews.newRound();++paperGeneration;if(submit)paperSubmit();
}
void showCharging(){
  activity();saveVolume();volumeControl.leave();resetGame(false);
  photoNotice=0;tuningView.reset();keyGestures.reset(keyEnabled&&(keyDebounce.held()||digitalRead(cfg::KEY)==LOW),millis());
  if(M5.Speaker.isRunning())M5.Speaker.end();
  paperViews.certificate();paperSubmit(false);drawnMode=-1;
  lcdBrightness=boardpower::CHARGING_BRIGHTNESS;M5.Display.setBrightness(lcdBrightness);
  Serial.println("USB shore-power view: alembic on LCD, certificate on paper; press to resume idle.");
}
bool resumeCharging(){
  if(!charging.dismiss())return false;
  activity();tuningView.reset();drawnMode=-1;
  lcdBrightness=boardpower::BRIGHTNESS;M5.Display.setBrightness(lcdBrightness);
  keyGestures.reset(keyEnabled&&(keyDebounce.held()||digitalRead(cfg::KEY)==LOW),millis());
  Serial.println("USB view dismissed; normal idle. This action did not start a story or change frequency.");
  return true;
}
void pollUsbPower(){
  const uint32_t now=millis();
  if(uint32_t(now-usbPolledAt)<200)return;
  usbPolledAt=now;usbReading=boardpower::readUsb();
  const auto before=usbFilter.value(),supply=usbFilter.update(usbReading.supply(),now);
  if(before!=supply)Serial.printf("USB supply=%s AXP00=%02x AW02=%02x reads_ok=%d\n",
    supply==radio::UsbSupply::Present?"PRESENT":supply==radio::UsbSupply::Absent?"ABSENT":"UNKNOWN",usbReading.pmic,usbReading.output0,usbReading.ok);
  const bool configured=boardpower::configurePowerButton(supply==radio::UsbSupply::Present);
  if(configured!=powerButtonConfigured){powerButtonConfigured=configured;Serial.printf("POWER button configuration verified=%d\n",configured);}
  const bool idle=!startup.active()&&state==State::Idle&&!volumeControl.enabled&&!photoCountdown&&!photoNotice&&!keyMonitor
    &&!player.active()&&!M5.Speaker.isPlaying()&&paperViews.view!=paper::View::Photo
    &&!tuningView.active(now,cfg::TUNING_VIEW_MS)
    &&!forceStandby&&(!keyEnabled||digitalRead(cfg::KEY)!=LOW)&&powerPolicy.idleMs(now)>=1000;
  const auto action=charging.update(supply,idle);
  if(action==radio::DockAction::Enter)showCharging();
  else if(action==radio::DockAction::Sleep){
    forceStandby=true;drawnMode=-1;
    Serial.println("USB unplugged while docked: prepare certificate/deep sleep.");
  }else if(action==radio::DockAction::Resume){
    activity();drawnMode=-1;
    Serial.println("USB detection unavailable: hide charging artwork, resume idle; hardware POWER cutoff restored when I2C permits.");
  }
}
void sendSOS(){
  if(startup.active()||state==State::Tx||state==State::Rx)return;
  saveVolume();volumeControl.leave();photoNotice=0;
  resetGame(false);tuningView.reset();playPitch=cfg::TX_HZ;
  startup.begin(millis(),paperTarget(false),true);paperSubmit(false);drawnMode=-1;
  Serial.println("STARTUP prepare story: keep S-meter; wait for reception page before TX; press again to cancel.");
}
void startDemo(){
  if(state==State::Tx||state==State::Rx){Serial.println("Demo already playing; click ignored");return;}
  frequency=500;sendSOS();Serial.println("Demo: automatically tuned to 500");
}
void activateButton(bool demo=false){
  if(startup.active()){
    if(startup.transmit()){resetGame();activity();Serial.println("Startup cancelled; no transmission.");}
    return;
  }
  if(resumeCharging())return;
  activity();
  if(volumeControl.enabled){saveVolume();volumeControl.leave();M5.Speaker.stop();return;}
  if(photoCountdown){photoCountdown=false;photoNotice=0;tuningView.reset();Serial.println("Photo countdown cancelled");return;}
  if(demo){startDemo();return;} // POWER keeps its existing automatic demo behavior.
  if(state!=State::Idle){
    resetGame(); // Stop audio, cancel reply/hold, invalidate queued paper text.
    Serial.println("Exchange stopped by button; press again to start from the beginning.");
    return;
  }
  sendSOS();
}
void toggleVolume(){
  if(resumeCharging())return;
  activity();
  if(volumeControl.enabled){saveVolume();volumeControl.leave();M5.Speaker.stop();return;}
  if(state!=State::Idle)resetGame();
  photoCountdown=false;photoNotice=0;tuningView.reset();
  volumeControl.enter(millis());Serial.println("Volume mode: turn encoder; click or wait ten seconds to leave");
}
void schedulePhoto(){
  if(resumeCharging())return;
  activity();
  saveVolume();volumeControl.leave();resetGame();photoNotice=0;
  photoCountdown=true;photoAt=millis();Serial.println("PHOTO countdown: three seconds");
}
void capturePhoto(){
  photoCountdown=false;
  bool ok=false;
  if(photo::storageReady()){
    auto* bits=static_cast<uint8_t*>(ps_malloc(photo::BYTES));
    if(bits&&photo::capture(bits)){
      uint32_t sequence=settingsReady?preferences.getUInt("photo_seq",0):0;
      char path[32];do{snprintf(path,sizeof(path),"/photo-%06lu.pbm",(unsigned long)++sequence);}while(LittleFS.exists(path));
      if(photo::save(path,bits)){
        latestPhoto=path;
        if(settingsReady){preferences.putUInt("photo_seq",sequence);preferences.putString("photo_last",latestPhoto);}
        requestPhoto();ok=true;
        Serial.printf("PHOTO saved %s (%d bitmap bytes)\n",path,photo::BYTES);
      }else Serial.println("PHOTO save failed or storage full; previous photographs retained");
    }
    free(bits);
  }else Serial.println("PHOTO storage unavailable");
  photoNotice=ok?1:2;photoNoticeAt=millis();
  if(ok)volumeSample();
  // Do not interpret a held contact across capture as a new gesture.
  bool down=digitalRead(cfg::KEY)==LOW;keyDebounce.reset(down,millis());keyGestures.reset(down,millis());
}
void tune(int delta){
  if(!delta||startup.active())return;
  if(resumeCharging())return;
  activity();
  if(volumeControl.enabled){volumeControl.turn(delta,millis());M5.Speaker.setVolume(radio::percentToVolume(volumeControl.percent));return;}
  frequency=constrain(frequency+delta,450,550);
  tuningView.rotated(millis()); // Also show a limit value when turning against the end stop.
}
const char* stateName(){
  switch(state){case State::Idle:return "Press SOS to send";case State::Tx:return "TITANIC transmitting";case State::Seek:return "Turn knob: find 500";case State::Rx:return "CARPATHIA replying";case State::Done:return "Help is on its way";}return "";
}
// Capture edges during LCD transfers and ePaper work; keep ISR work bounded.
void ARDUINO_ISR_ATTR encoderEdge(){
  portENTER_CRITICAL_ISR(&encoderMux);
  uint8_t ab=(digitalRead(cfg::ENCODER_A)==HIGH?2:0)|(digitalRead(cfg::ENCODER_B)==HIGH?1:0);
  int step=encoderDecoder.update(ab);
  if(encoderPending+step<=1000 && encoderPending+step>=-1000)encoderPending+=step;
  ++encoderEdges;
  portEXIT_CRITICAL_ISR(&encoderMux);
}
void disableEncoder(){
  detachInterrupt(digitalPinToInterrupt(cfg::ENCODER_A));detachInterrupt(digitalPinToInterrupt(cfg::ENCODER_B));
  encoderEnabled=false;pinMode(cfg::ENCODER_A,INPUT);pinMode(cfg::ENCODER_B,INPUT);
  portENTER_CRITICAL(&encoderMux);encoderPending=0;portEXIT_CRITICAL(&encoderMux);
}
void enableEncoder(){
  if(encoderEnabled)disableEncoder();
  // Port A is GPIO here. Never initialize/scan external I2C with contacts attached.
  pinMode(cfg::ENCODER_A,INPUT_PULLUP);pinMode(cfg::ENCODER_B,INPUT_PULLUP);
  uint8_t ab=(digitalRead(cfg::ENCODER_A)==HIGH?2:0)|(digitalRead(cfg::ENCODER_B)==HIGH?1:0);
  portENTER_CRITICAL(&encoderMux);encoderDecoder.reset(ab,encoderSteps);encoderPending=0;encoderEdges=0;portEXIT_CRITICAL(&encoderMux);
  attachInterrupt(digitalPinToInterrupt(cfg::ENCODER_A),encoderEdge,CHANGE);
  attachInterrupt(digitalPinToInterrupt(cfg::ENCODER_B),encoderEdge,CHANGE);encoderEnabled=true;
  Serial.printf("EC11 enabled: Port A yellow GPIO2=A, white GPIO1=B, black=common GND; %d edges/step\n",encoderSteps);
}
void pollEncoder(){
  if(!encoderEnabled)return;
  portENTER_CRITICAL(&encoderMux);int steps=encoderPending;encoderPending=0;portEXIT_CRITICAL(&encoderMux);
  if(steps){tune(steps*encoderDirection);Serial.printf("EC11 steps=%d tuning=%d\n",steps*encoderDirection,frequency);}
}
void scan(){Serial.println("Port A is reserved for EC11 GPIO contacts; I2C scan disabled. Use encoder on / status.");}
void enableKey(){
  pinMode(cfg::KEY,INPUT_PULLUP);keyDebounce.reset(digitalRead(cfg::KEY)==LOW,millis());keyEnabled=true;
  keyGestures.reset(keyDebounce.held(),millis());
  Serial.println("Key enabled on GPIO9 (Port B yellow), switch to GND.");
}
void logStatus(){
  Serial.printf("state=%s tuning=%d signal=%d encoder=%s key=%s EPD=%s requests=%lu slow=%d\n",stateName(),frequency,radio::signalStrength(frequency),encoderEnabled?"EC11_ON":"OFF",keyEnabled?"ON":"OFF",paperEnabled?"ENABLED":"OFF",(unsigned long)paperUpdates.load(),int(paperTimeout.load()));
  Serial.printf("EPD working=%d last_ms=%lu age_ms=%lu uptime_ms=%lu auto_start=%d\n",int(paperWorking.load()),(unsigned long)paperLastMs.load(),(unsigned long)(paperUpdates.load()?millis()-paperLastFinishedAt.load():0),(unsigned long)millis(),cfg::EPD_AUTO_START);
  Serial.printf("EPD view=%s; volume=%u/255 (%.1f%%)\n",paperViewName(paperViews.view),unsigned(M5.Speaker.getVolume()),M5.Speaker.getVolume()*100.0f/255.0f);
  Serial.printf("LCD view=%s; frequency=%d kHz; controls_auto_start=%d\n",startup.bootAnimation()?"SPOOKY_STARTUP":charging.active()?"SHORE_POWER":volumeControl.enabled?"VOLUME":photoCountdown?"PHOTO_COUNTDOWN":tuningView.active(millis(),cfg::TUNING_VIEW_MS)?"FREQUENCY":"S_METER",frequency,cfg::CONTROLS_AUTO_START);
  Serial.printf("STARTUP active=%d transmit_pending=%d elapsed_ms=%lu; firmware=v0.12.1\n",startup.active(),startup.transmit(),(unsigned long)startup.elapsed(millis()));
  Serial.printf("USB state=%d docked=%d AXP00=%02x AW02=%02x reads_ok=%d power_button_configured=%d; DIN charge percent unavailable\n",int(usbFilter.value()),charging.active(),usbReading.pmic,usbReading.output0,usbReading.ok,powerButtonConfigured);
  Serial.printf("Settings: NVS=%d saved_volume=%d; latest_photo=%s\n",settingsReady,savedVolume,latestPhoto.c_str());
  Serial.printf("EC11: %d edges/step, direction=%d; dry contacts cannot be auto-detected\n",encoderSteps,encoderDirection);
  const int mv=M5.Power.getBatteryVoltage();
  if(mv>=2800&&mv<=4350)Serial.printf("Battery: %d mV; runtime requires a measured discharge test\n",mv);
  else Serial.printf("Battery telemetry unavailable/implausible (%d mV); do not use as state of charge\n",mv);
  Serial.printf("POWER saving=%d CPU=%lu MHz backlight=%u/255 idle_ms=%lu amp_running=%d paper_parked=%d deep_sleeps=%lu boot_from_sleep=%d wake_cause=%d wake_mask=%llu\n",
    powerPolicy.enabled,(unsigned long)getCpuFrequencyMhz(),lcdBrightness,(unsigned long)powerPolicy.idleMs(millis()),M5.Speaker.isRunning(),int(paperParked.load()),(unsigned long)deepSleepCount,wokeFromSleep,int(esp_sleep_get_wakeup_cause()),(unsigned long long)esp_sleep_get_ext1_wakeup_status());
  Serial.printf("POWER rails: mic=%d camera=%d SD=%d; no early dim; standby after 60 s inactivity; GPIO9 or encoder GPIO2/1 wakes to idle\n",
    M5.Power.Axp2101.getALDO2Enabled(),M5.Power.Axp2101.getALDO3Enabled(),M5.Power.Axp2101.getALDO4Enabled());
}
void help(){Serial.println("Commands: help | scan | status | reboot | power on/off | sleep | sleep test | camera test | encoder on/off | encoder steps 2/4 | encoder reverse | tone | send | demo | button | tune 500 | + | - | reset | stop | epd on/off/test/clear | key on/test/off | volume mode | volume 204 (0..255, saved) | photo | photo show | photo dump");}
void processCommand(String s){
  s.trim();s.toLowerCase();
  if(startup.active()&&s!="status"&&s!="help"&&s!="?"&&s!="button"&&s!="stop"&&s!="reset"&&s!="reboot"){
    Serial.println("STARTUP busy; command ignored until paper/prelude completes. button/stop cancels a pending story.");return;
  }
  // Bench commands explicitly operate the device; the 'button' command still
  // consumes the first press to dismiss the dock, like the physical key.
  if(s!="status"&&s!="help"&&s!="?"&&s!="button"&&s!="sleep")resumeCharging();
  if(s!="status"&&s!="help"&&s!="?")activity();
  if(s=="help"||s=="?")help();
  else if(s=="scan")scan();
  else if(s=="status")logStatus();
  else if(s=="reboot"){saveVolume();Serial.println("Reboot requested; silent startup, no autoplay");Serial.flush();ESP.restart();}
  else if(s=="power on"||s=="power off"){
    powerPolicy.enabled=s=="power on";Serial.printf("POWER automatic standby=%d (temporary; reboot restores ON)\n",powerPolicy.enabled);
  }
  else if(s=="sleep"||s=="sleep test"){
    forceStandby=true;sleepTestSeconds=s=="sleep test"?10:0;
    Serial.println("POWER standby requested: USB returns to alembic, battery sleeps. 'sleep test' forces a ten-second deep reboot.");
  }
  else if(s=="camera test"){
    if(state!=State::Idle||photoCountdown||volumeControl.enabled||M5.Speaker.isPlaying()){
      Serial.println("CAMERA test requires idle radio");return;
    }
    auto* bits=static_cast<uint8_t*>(ps_malloc(photo::BYTES));
    bool ok=bits&&photo::capture(bits);free(bits);
    Serial.printf("CAMERA power-cycle test=%d; temporary frame discarded, saved photos unchanged; CPU=%lu camera_rail=%d\n",ok,(unsigned long)getCpuFrequencyMhz(),M5.Power.Axp2101.getALDO3Enabled());
    activity();
  }
  else if(s=="encoder on")enableEncoder();
  else if(s=="encoder off"){disableEncoder();Serial.println("EC11 off");}
  else if(s=="encoder steps 2"||s=="encoder steps 4"){
    encoderSteps=s.endsWith("2")?2:4;if(encoderEnabled)enableEncoder();
    Serial.printf("EC11 edges per step: %d\n",encoderSteps);
  }
  else if(s=="encoder reverse"){encoderDirection=-encoderDirection;Serial.printf("EC11 direction: %d\n",encoderDirection);}

  else if(s=="tone"){resetGame();M5.Speaker.tone(650,700,0,true);Serial.println("650 Hz speaker test, 700 ms");}
  else if(s=="send")sendSOS();
  else if(s=="demo")startDemo();
  else if(s=="button")activateButton();
  else if(s=="volume mode")toggleVolume();
  else if(s=="photo")schedulePhoto();
  else if(s=="photo show"){
    if(latestPhoto.length()&&photo::storageReady()){
      saveVolume();volumeControl.leave();photoNotice=0;
      resetGame(false);requestPhoto();enablePaper(false);
    }else Serial.println("No saved photo available");
  }
  else if(s=="photo dump"){
    auto* bits=static_cast<uint8_t*>(ps_malloc(photo::BYTES));
    if(bits&&latestPhoto.length()&&photo::storageReady()&&photo::read(latestPhoto.c_str(),bits)){
      Serial.printf("PHOTOHEX %s %d\n",latestPhoto.c_str(),photo::BYTES);
      for(int offset=0;offset<photo::BYTES;offset+=64){char line[145];int n=snprintf(line,sizeof(line),"PH %04x ",offset);
        for(int j=offset;j<std::min(offset+64,photo::BYTES);++j)n+=snprintf(line+n,sizeof(line)-n,"%02x",bits[j]);
        Serial.println(line);
      }
      Serial.println("PHOTOHEX END");
    }else Serial.println("No saved photo available");
    free(bits);
  }
  else if(s=="+"||s=="-")tune(s=="+"?1:-1);
  else if(s.startsWith("tune "))tune(constrain(s.substring(5).toInt(),450,550)-frequency);
  else if(s=="reset"||s=="stop"){saveVolume();volumeControl.leave();resetGame();}
  else if(s=="epd on")enablePaper();
  else if(s=="epd off")disablePaper();
  else if(s=="epd test"){
    if(player.active()){Serial.println("Stop demo before EPD test");return;}
    resetGame();txVisible="SOS WE NEED HELP";rxVisible="WE ARE COMING";++paperGeneration;enablePaper(false);
  }
  else if(s=="epd clear"){resetGame();}
  else if(s=="key on")enableKey();
  else if(s=="key test"){resetGame();enableKey();keyMonitor=true;Serial.println("Manual key monitor: hold to sound, release to stop");}
  else if(s=="key off"){keyEnabled=false;keyMonitor=false;M5.Speaker.stop();lastManualTone=false;pinMode(cfg::KEY,INPUT);}
  else if(s.startsWith("volume ")){int v=constrain(s.substring(7).toInt(),0,255);M5.Speaker.setVolume(v);volumeControl.percent=radio::volumeToPercent(v);saveVolume();Serial.printf("Volume %d/255 (%.1f%%)\n",v,v*100.0f/255.0f);}
  else if(s.length())Serial.println("Unknown command; type help");
}
void screen(){
  const uint32_t started=micros(),now=millis();
  bool transferred=false;
  const float elapsed=meterLastAt?uint32_t(now-meterLastAt):33;
  meterLastAt=now;
  const float target=radio::meterTarget(state==State::Tx||lastManualTone,state==State::Rx,
      morseKeyed||lastManualTone,radio::signalStrength(frequency));
  needleStrength=radio::meterStep(needleStrength,target,elapsed);
  if(photoNotice&&uint32_t(now-photoNoticeAt)>=2000)photoNotice=0;
  int mode=startup.bootAnimation()?6:charging.active()?5:volumeControl.enabled?2:photoCountdown?3:photoNotice?4:tuningView.active(now,cfg::TUNING_VIEW_MS)?1:0;
  if(!frequencyCanvasReady)mode=0;
  int value=mode==6?int((startup.elapsed(now)/100)%48):mode==5?int((now/150)%32):mode==2?volumeControl.percent:mode==3?3-int(uint32_t(now-photoAt)/1000):mode==4?photoNotice:frequency;
  if(mode){
    if(drawnMode!=mode||drawnValue!=value){
      if(mode==1)radio::frequencyFace(frequencyLcd,frequency);
      else if(mode==2)radio::volumeFace(frequencyLcd,volumeControl.percent);
      else if(mode==5)radio::chargingFace(frequencyLcd,value);
      else if(mode==6)radio::startupFace(frequencyLcd,value);
      else{
        frequencyLcd.fillScreen(TFT_BLACK);frequencyLcd.fillCircle(112,112,110,uint16_t(0xa3e8));
        frequencyLcd.fillCircle(112,112,108,uint16_t(0x1081));
        frequencyLcd.setTextDatum(lgfx::textdatum_t::middle_center);frequencyLcd.setFont(&fonts::Font2);
        frequencyLcd.setTextColor(uint16_t(0xfdae));frequencyLcd.setTextSize(1);
        frequencyLcd.drawString(mode==3?"PORTRAIT":value==1?"PHOTO SAVED":"PHOTO FAILED",112,65);
        frequencyLcd.setTextSize(3);char number[4];snprintf(number,sizeof(number),"%d",value);
        frequencyLcd.drawString(mode==3?number:value==1?"OK":"!",112,114);
        frequencyLcd.setTextSize(1);frequencyLcd.drawString(mode==3?"HOLD STILL":value==1?"CHECK THE PAPER":"SEE SERIAL LOG",112,163);
      }
      frequencyLcd.pushSprite(48,8);drawnValue=value;transferred=true;
    }
  }else{
    if(drawnMode!=0)M5.Display.pushImage(48,8,224,224,meterFace);
    const float a=radio::meterAngle(needleStrength)*DEG_TO_RAD;
    const int x=112+cosf(a)*85,y=95-sinf(a)*85;
    if(drawnMode!=0||drawnNeedleX!=x||drawnNeedleY!=y){
    lcd.pushImage(0,-48,224,224,meterFace);
    lcd.drawWideLine(114,97,x+2,y+2,1.0f,uint16_t(0x7350));
    lcd.drawWideLine(112,95,x,y,.8f,uint16_t(0x8823));
    lcd.fillSmoothCircle(112,95,6,uint16_t(0x2924));
    lcd.fillSmoothCircle(111,94,2,uint16_t(0xA4C8));
    lcd.pushSprite(48,56);
    transferred=true;
    drawnNeedleX=x;drawnNeedleY=y;
    }
  }
  drawnMode=mode;
  meterMaxUs=std::max(meterMaxUs,uint32_t(micros()-started));if(transferred)++meterFrames;
  if(uint32_t(now-meterWindowAt)>=5000){
    Serial.printf("LCD transfers: %.1f/s, max screen call %lu us (software counters)\n",meterFrames*1000.0f/uint32_t(now-meterWindowAt),(unsigned long)meterMaxUs);
    meterWindowAt=now;meterFrames=0;meterMaxUs=0;
  }
}
void standby(){
  if(!boardpower::configurePowerButton(false)){
    activity();Serial.println("POWER cutoff restore failed; staying awake for retry rather than deep-sleeping with unverified button settings.");return;
  }
  saveVolume();if(M5.Speaker.isRunning())M5.Speaker.end();
  const auto key=static_cast<gpio_num_t>(cfg::KEY);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  // Retain RTC pullups: the EC11 contacts have no external pullup resistors.
  esp_err_t configured=esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
  if(keyEnabled&&configured==ESP_OK){
    rtc_gpio_pullup_en(key);rtc_gpio_pulldown_dis(key);
    configured=esp_sleep_enable_ext0_wakeup(key,0);
  }
  uint64_t mask=0;radio::EncoderWake plan{};
  if(encoderEnabled&&configured==ESP_OK){
    detachInterrupt(digitalPinToInterrupt(cfg::ENCODER_A));detachInterrupt(digitalPinToInterrupt(cfg::ENCODER_B));
    const uint8_t ab=(digitalRead(cfg::ENCODER_A)==HIGH?2:0)|(digitalRead(cfg::ENCODER_B)==HIGH?1:0);
    plan=radio::encoderWake(ab);
    if(plan.phases&2)mask|=1ULL<<cfg::ENCODER_A;
    if(plan.phases&1)mask|=1ULL<<cfg::ENCODER_B;
    for(int pin:{cfg::ENCODER_A,cfg::ENCODER_B}){
      rtc_gpio_pullup_en(gpio_num_t(pin));rtc_gpio_pulldown_dis(gpio_num_t(pin));
    }
    configured=esp_sleep_enable_ext1_wakeup(mask,plan.high?ESP_EXT1_WAKEUP_ANY_HIGH:ESP_EXT1_WAKEUP_ANY_LOW);
  }
  if(configured==ESP_OK&&sleepTestSeconds)configured=esp_sleep_enable_timer_wakeup(uint64_t(sleepTestSeconds)*1000000);
  if(configured!=ESP_OK){
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);boardpower::releaseSleepPins();
    if(keyEnabled)enableKey();
    if(encoderEnabled)enableEncoder();
    activity();
    Serial.printf("POWER wake configuration failed: %d; staying awake\n",configured);return;
  }
  M5.Display.sleep();boardpower::prepareDeepSleep();
  retainedMagic=SLEEP_MAGIC;++deepSleepCount;
  Serial.printf("POWER entering DEEP SLEEP #%lu; certificate retained; TOP or tuning knob reboots to idle; encoder_mask=%llu level=%s timer=%lu s\n",
    (unsigned long)deepSleepCount,(unsigned long long)mask,plan.high?"HIGH":"LOW",(unsigned long)sleepTestSeconds);
  Serial.flush();
  esp_deep_sleep_start(); // Wake is a fresh setup(), never automatic playback.
}
void updatePower(){
  const uint32_t now=millis();
  const bool audioBusy=player.active()||M5.Speaker.isPlaying()||lastManualTone;
  if(audioBusy)audioLastAt=now;
  else if(M5.Speaker.isRunning()&&uint32_t(now-audioLastAt)>=1000)M5.Speaker.end();
  if(charging.active()||startup.active())return; // USB dock or silent paper preparation.
  const bool protectedView=state==State::Tx||state==State::Rx||state==State::Done||volumeControl.enabled||photoCountdown||photoNotice||paperViews.view==paper::View::Photo;
  const bool busy=protectedView||audioBusy||keyMonitor;
  const bool held=keyEnabled&&digitalRead(cfg::KEY)==LOW;
  // A held contact is activity, even before a click/hold gesture has resolved.
  if(held)activity();
  const uint8_t brightness=boardpower::BRIGHTNESS;
  if(brightness!=lcdBrightness){lcdBrightness=brightness;M5.Display.setBrightness(brightness);Serial.printf("POWER backlight=%u/255\n",brightness);}
  const bool wakeEnabled=keyEnabled||encoderEnabled;
  const bool ready=(forceStandby&&!busy&&wakeEnabled&&!held)||powerPolicy.sleep(now,busy,wakeEnabled,held);
  if(!ready){paperSleepRequested=false;preparingSleep=false;return;}
  if(!sleepTestSeconds&&charging.standby()){showCharging();return;}
  if(!preparingSleep){
    resetGame(false);paperViews.certificate();sleepCertificate=paperTarget(false);
    paperSubmit(false);preparingSleep=true;
    Serial.println("POWER preparing sleep: waiting for certificate's completed refresh");
  }
  if(paperEnabled&&!paper::caughtUp(paperPresented.frame,sleepCertificate))return;
  paperSleepRequested=true;
  // Check the queue before the parked flag: the worker clears parked before
  // removing new work, so a stale acknowledgement cannot hide a queued frame.
  if(!paperTaskStarted||(uxQueueMessagesWaiting(paperQueue)==0&&paperParked.load()))standby();
}
void setup(){
  wokeFromSleep=esp_reset_reason()==ESP_RST_DEEPSLEEP;
  if(!wokeFromSleep||retainedMagic!=SLEEP_MAGIC)deepSleepCount=0;
  boardpower::releaseSleepPins();
  auto c=M5.config();c.internal_mic=false;c.internal_spk=true;c.internal_imu=false;c.external_display_value=0;c.output_power=true;
  M5.begin(c);Serial.begin(115200); // do not wait for a USB host
  boardpower::begin();
  M5.Display.setRotation(1);M5.Display.setBrightness(boardpower::BRIGHTNESS);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setSwapBytes(true);M5.Display.pushImage(48,8,224,224,meterFace);
  lcd.setColorDepth(16);lcd.setSwapBytes(true);if(!lcd.createSprite(224,112)){Serial.println("LCD sprite allocation FAILED");}
  frequencyLcd.setColorDepth(16);frequencyLcd.setPsram(true);
  frequencyCanvasReady=frequencyLcd.createSprite(224,224)!=nullptr;
  if(!frequencyCanvasReady)Serial.println("Frequency sprite allocation FAILED; S-meter remains available");
  for(int p=0;p<2;++p){
    audio::render(keyedTones[p],ditSamples,p?cfg::RX_HZ:cfg::TX_HZ);
    audio::render(keyedTones[p]+ditSamples,ditSamples*3,p?cfg::RX_HZ:cfg::TX_HZ);
  }
  settingsReady=preferences.begin("titanic",false);
  savedVolume=settingsReady?preferences.getUChar("volume",cfg::VOLUME):cfg::VOLUME;
  volumeControl.percent=radio::volumeToPercent(savedVolume);
  if(settingsReady)latestPhoto=preferences.getString("photo_last","");
  M5.Speaker.setVolume(savedVolume); // Port A external I2C intentionally not started.
  Serial.println("\nTitanic CoreS3 v0.12.1. Ghost only on boot/wake; USB shore-power alembic; no early dim; 60-second standby.");
  Serial.printf("POWER boot reset=%d from_deep_sleep=%d cause=%d count=%lu; wake never starts a sequence\n",
    int(esp_reset_reason()),wokeFromSleep,int(esp_sleep_get_wakeup_cause()),(unsigned long)deepSleepCount);
  Serial.printf("Saved volume %d/255; NVS=%d; camera GC0308 available on demand\n",savedVolume,settingsReady);
  Serial.printf("EPD build: %s; HSPI=%d; GPIO MOSI17 SCK18 CS5 DC6 RST7 BUSY8\n",RADIO_EPD_LEGACY?"GDEW042T2":"GDEY042T81",HSPI);
  Serial.println("USB: press/turn leaves alembic for idle; hold side POWER returns to alembic. Normal POWER short click: auto-tuned demo. Battery hardware long-off and RST retained.");
  if(cfg::CONTROLS_AUTO_START){enableEncoder();enableKey();}
  help();
  // Every reboot refreshes the certificate while the LCD animates silently.
  paperViews.certificate();++paperGeneration;
  startup.begin(millis(),paperTarget(false),false);screen();
  if(cfg::EPD_AUTO_START)enablePaper(false);
  Serial.println("STARTUP silent boot prelude: wait for certificate; no autoplay.");
  activity();
}
void loop(){
  M5.update();uint32_t now=millis();
  pollUsbPower();
  if(paperAckQueue && xQueueReceive(paperAckQueue,&paperPresented,0)==pdTRUE){
    paperViews.presented(paperPresented.frame,paperPresented.at);
    Serial.printf("EPD presented view=%s generation=%lu at_ms=%lu\n",paperViewName(paperPresented.frame.view),(unsigned long)paperPresented.frame.generation,(unsigned long)paperPresented.at);
  }
  // M5Unified maps AXP's one-second long-press IRQ to wasHold(). Hardware cutoff
  // is disabled only with verified USB power; battery and unknown retain it.
  if(!startup.active()&&M5.BtnPWR.wasHold()&&charging.usb()){
    if(!resumeCharging()&&charging.standby())showCharging();
  }
  if(M5.BtnPWR.wasClicked()){Serial.println("Physical POWER short click");activateButton(true);}
  while(Serial.available()){
    char ch=char(Serial.read());
    if(ch=='\n'||ch=='\r'){if(command.length()){processCommand(command);command="";}}
    else if(command.length()<96)command+=ch;
  }
  if(startup.active()){
    // Consume waking/early input; it must never become a delayed play command.
    portENTER_CRITICAL(&encoderMux);encoderPending=0;portEXIT_CRITICAL(&encoderMux);
    const bool pressed=keyEnabled&&keyDebounce.update(digitalRead(cfg::KEY)==LOW,now);
    if(pressed&&startup.transmit())activateButton();
    if(M5.Touch.getCount()&&M5.Touch.getDetail().wasPressed()&&startup.transmit())activateButton();
    keyGestures.reset(keyEnabled&&keyDebounce.held(),now);
    const auto result=startup.update(now,paperEnabled,paperPresented.frame);
    if(result!=radio::StartupResult::Waiting){
      const bool transmit=result==radio::StartupResult::Ready&&startup.transmit();
      startup.cancel();drawnMode=-1;activity();
      if(result==radio::StartupResult::TimedOut)Serial.println("STARTUP paper acknowledgment timed out; returning idle without transmission.");
      else Serial.printf("STARTUP ready: paper complete; transmit=%d\n",transmit);
      if(transmit){state=State::Tx;player.start(txMessage,cfg::DOT_MS,millis(),onMorse);Serial.println("TX starting AFTER paper refresh/prelude");}
    }
    if(radio::reached(now,nextScreen)){screen();nextScreen=millis()+33;}
    updatePower();delay(2);return;
  }
  pollEncoder();
  if(keyEnabled){
    keyDebounce.update(digitalRead(cfg::KEY)==LOW,now);
    if(charging.active()&&keyDebounce.held())resumeCharging(); // consume through release, even a hold
    if(keyMonitor){bool held=keyDebounce.held();if(held!=lastManualTone){lastManualTone=held;if(held)M5.Speaker.tone(cfg::TX_HZ,UINT32_MAX,0,true);else M5.Speaker.stop(0);Serial.println(held?"KEY DOWN":"KEY UP");}}
    else{
      auto gesture=keyGestures.update(keyDebounce.held(),now);
      if(gesture==radio::Gesture::Click){Serial.println("Top key click");activateButton();}
      else if(gesture==radio::Gesture::Hold){Serial.println("Top key hold");toggleVolume();}
      else if(gesture==radio::Gesture::Triple){Serial.println("Top key triple click");schedulePhoto();}
    }
  }
  if(M5.Touch.getCount()){
    auto t=M5.Touch.getDetail();
    int dx=int(t.x)-160,dy=int(t.y)-120;
    if(t.wasPressed()&&dx*dx+dy*dy<=112*112)activateButton();
  }
  bool wasPlaying=player.active();player.tick(now);
  if(wasPlaying&&!player.active()){
    morseKeyed=false;
    M5.Speaker.stop(0);
    if(state==State::Tx){state=State::Seek;lockedTimer=false;}
    else if(state==State::Rx){state=State::Done;paperViews.complete(paperTarget());Serial.println("Rescue exchange complete; wait for final ePaper letter, then hold ten seconds");}
  }
  if(state==State::Seek){
    if(abs(frequency-500)<=2){
      if(!lockedTimer){lockSince=now;lockedTimer=true;}
      if(uint32_t(now-lockSince)>=800){state=State::Rx;playPitch=cfg::RX_HZ;player.start(rxMessage,cfg::DOT_MS,now,onMorse);}
    }else lockedTimer=false;
  }
  if(state==State::Done && !paperEnabled)paperViews.presented(paperTarget(),now);
  const bool photoShowing=paperViews.view==paper::View::Photo;
  if(photoShowing && paperViews.tick(now,cfg::PHOTO_VIEW_MS)){
    ++paperGeneration;paperSubmit(false);
    Serial.println("Photo hold complete after ten seconds; reception view restored; saved photo retained.");
    activity();
  }else if(state==State::Done && paperViews.tick(now,cfg::REPLAY_DELAY_MS)){
    resetGame(false);paperViews.certificate();paperSubmit(false);frequency=nextRoundFrequency;
    nextRoundFrequency=nextRoundFrequency==cfg::NEXT_ROUND_HIGH?cfg::NEXT_ROUND_LOW:cfg::NEXT_ROUND_HIGH;
    Serial.println("Certificate requested after ten-second final-message hold; press to start a new exchange.");
    activity();
  }
  if(volumeControl.enabled&&volumeControl.settled(now)){saveVolume();volumeSample();Serial.println("Volume settled: preview tone");}
  if(volumeControl.expired(now,cfg::VOLUME_IDLE_MS)){
    saveVolume();volumeControl.leave();M5.Speaker.stop();
    Serial.println("Volume mode ended after ten seconds idle; S-meter restored.");
  }
  if(photoCountdown&&uint32_t(now-photoAt)>=3000)capturePhoto();
  if(radio::reached(now,nextScreen)){screen();nextScreen+=33;if(radio::reached(millis(),nextScreen))nextScreen=millis()+33;}
  updatePower();
  delay(2);
}
