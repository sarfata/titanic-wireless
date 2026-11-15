// Behavioral port of main.cpp's game loop; peripherals and RTOS are not emulated.
// Morse events and timing constants are exported from the actual C++ headers.
class RadioSimulator {
  constructor(data) { this.data=data;this.frequency=470;this.volume=80;this.nextRoundFrequency=data.nextRoundHigh;this.needle=0;this.meterAt=null;this.reset(); }
  reset(keepTuning=false) { this.volumeMode=false;this.volumeChangedAt=null;this.sampleUntil=0;this.state='Idle';this.tx='';this.rx='';this.tone=false;this.events=[];this.index=0;this.lockSince=null;this.paperView='Reception';if(!keepTuning)this.lastTuneAt=null; }
  hold(now=0){if(this.volumeMode){this.volumeMode=false;this.volumeChangedAt=null;this.sampleUntil=0;return;}if(this.state!=='Idle')this.reset();this.lastTuneAt=null;this.volumeMode=true;this.volumeActiveAt=now;}
  setVolume(value,now){this.volume=Math.max(0,Math.min(100,Math.round(value/5)*5));this.volumeChangedAt=this.volumeActiveAt=now;}
  turn(steps,now){if(this.volumeMode)this.setVolume(this.volume+steps*5,now);else this.tune(this.frequency+steps,now);}
  button(now,demo=false){
    if(this.volumeMode){this.hold();return;}
    if(!demo && this.state!=='Idle'){this.reset();return;}
    this.send(now,demo);
  }
  paperText(){return [this.tx,this.rx];}
  tune(value,now=null) { this.frequency=Math.min(550,Math.max(450,Math.round(value)));if(now!==null)this.lastTuneAt=now; }
  frequencyVisible(now) {return this.lastTuneAt!==null && now-this.lastTuneAt<this.data.tuningViewMs;}
  strength() { return Math.max(0,100-Math.abs(this.frequency-500)*4); }
  send(now,demo=false) {
    if(this.state==='Tx'||this.state==='Rx')return;
    this.reset();if(demo)this.frequency=500;this.start(0,now);
  }
  start(message,now) {
    this.message=this.data.messages[message];this.state=message===0?'Tx':'Rx';
    this.pitch=message===0?this.data.txHz:this.data.rxHz;
    this.events=this.message.events;this.index=0;this.dispatch(now);
  }
  dispatch(now) {
    while(this.index<this.events.length) {
      const [tone,units,word,chars]=this.events[this.index];this.tone=Boolean(tone);
      if(chars)this[this.state==='Tx'?'tx':'rx']=this.message.text.slice(0,chars);
      else if(word)this[this.state==='Tx'?'tx':'rx']=this.message.text.split(' ').slice(0,word).join(' ');
      if(units){this.deadline=now+units*this.data.dotMs;return;}
      this.index++;
    }
    this.tone=false;this.state=this.state==='Tx'?'Seek':'Done';this.lockSince=null;
    if(this.state==='Done')this.roundFinishedAt=now;
  }
  tick(now) {
    if(this.volumeMode&&now-this.volumeActiveAt>=10000)this.hold(now);
    if(this.volumeMode&&this.volumeChangedAt!==null&&now-this.volumeChangedAt>=650){this.sampleUntil=now+120;this.volumeChangedAt=null;}
    if(this.meterAt!==null){const target=this.meterTarget(),dt=Math.max(0,now-this.meterAt);
      this.needle+=(target-this.needle)*(1-Math.exp(-dt/(target>this.needle?this.data.meterRiseMs:this.data.meterDecayMs)));}
    this.meterAt=now;
    if((this.state==='Tx'||this.state==='Rx')&&now>=this.deadline){this.index++;this.dispatch(now);}
    if(this.state==='Seek') {
      if(Math.abs(this.frequency-500)<=2) {
        if(this.lockSince===null)this.lockSince=now;
        if(now-this.lockSince>=800)this.start(1,now);
      }else this.lockSince=null;
    }
    if(this.state==='Done'&&now-this.roundFinishedAt>=this.data.replayDelayMs){
      this.reset(true);this.frequency=this.nextRoundFrequency;
      this.paperView='Certificate';
      this.nextRoundFrequency=this.nextRoundFrequency===this.data.nextRoundHigh?this.data.nextRoundLow:this.data.nextRoundHigh;
    }
  }
  meterTarget() {return !this.tone?0:this.state==='Tx'?this.data.meterTx:this.state==='Rx'?this.data.meterRx*this.strength()/100:0;}
  status() { return {Idle:'Press SOS to send',Tx:'TITANIC transmitting',Seek:'Turn knob: find 500',Rx:'CARPATHIA replying',Done:'Help is on its way'}[this.state]; }
}
if(typeof module!=='undefined')module.exports=RadioSimulator;
