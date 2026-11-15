const assert=require('node:assert/strict');
const RadioSimulator=require('../preview/radio-simulator.js');
const data=require('../preview/morse-data.json');
const s=new RadioSimulator(data);
assert.equal(data.dotMs,120);
assert.equal(data.txHz,650);assert.equal(data.rxHz,800);
let now=0;
function until(state,max=60000){const end=now+max;while(s.state!==state&&now<end)s.tick(++now);assert.equal(s.state,state);}
s.send(now);until('Seek');assert.equal(s.tx,'SOS WE NEED HELP');assert.equal(s.rx,'');
for(let i=0;i<2000;i++)s.tick(++now);assert.equal(s.state,'Seek');
s.tune(498);s.tick(++now);
for(let i=0;i<799;i++)s.tick(++now);assert.equal(s.state,'Seek');
s.tune(497);s.tick(++now);assert.equal(s.lockSince,null);
s.tune(502);s.tick(++now);
for(let i=0;i<799;i++)s.tick(++now);assert.equal(s.state,'Seek');
s.tick(++now);assert.equal(s.state,'Rx');assert.equal(s.pitch,800);
until('Done');assert.equal(s.rx,'WE ARE COMING');assert.equal(s.tone,false);
for(let i=0;i<data.replayDelayMs-1;i++)s.tick(++now);assert.equal(s.state,'Done');
s.tick(++now);assert.equal(s.state,'Idle');assert.equal(s.frequency,530);assert.equal(s.strength(),0);assert.equal(s.tx,'');assert.equal(s.rx,'');
s.send(++now,true);assert.equal(s.frequency,500);assert.equal(s.state,'Tx');
until('Done');assert.equal(s.tx,'SOS WE NEED HELP');assert.equal(s.rx,'WE ARE COMING');
for(let i=0;i<data.replayDelayMs;i++)s.tick(++now);assert.equal(s.state,'Idle');assert.equal(s.frequency,470);assert.equal(s.strength(),0);
s.send(++now);for(let i=0;i<4000;i++)s.tick(++now);s.reset();
for(let i=0;i<data.replayDelayMs;i++)s.tick(++now);
assert.equal(s.state,'Idle');assert.equal(s.tx,'');assert.equal(s.rx,'');assert.equal(s.tone,false);
s.tune(0);assert.equal(s.frequency,450);s.tune(900);assert.equal(s.frequency,550);
console.log('PASS: preview rescue exchange, off-frequency wait, 800 ms lock/reset, 10 s final-message hold with alternating detune, boundary tuning, cancellation');
const m=new RadioSimulator(data);let t=0;m.send(t,true);
let txPeak=0,rxPeak=0,gaps=0,previous=0;
while(t<40000){m.tick(++t);if(m.state==='Tx')txPeak=Math.max(txPeak,m.needle);if(m.state==='Rx')rxPeak=Math.max(rxPeak,m.needle);
 if(!m.tone&&m.state==='Tx'&&m.needle<previous)gaps++;previous=m.needle;}
assert(txPeak>9.8&&txPeak<=10);assert(rxPeak>5.4&&rxPeak<5.7);assert(gaps>100);assert(m.needle<.01);
m.send(++t,true);for(let i=0;i<100;i++)m.tick(++t);assert(m.needle>9);m.reset();const held=m.needle;m.tick(++t);assert(m.needle<held&&m.needle>held*.99);
for(let i=0;i<data.replayDelayMs;i++)m.tick(++t);assert(m.needle<.01);m.tune(470);assert.equal(m.meterTarget(),0);
console.log('PASS: preview keyed needle, TX/RX peaks, gaps, slow cancellation/reset decay');

const letters=new RadioSimulator(data);letters.send(0,true);
for(let t=0;t<=600;t++)letters.tick(t);assert.equal(letters.tx,'S');
for(let t=601;t<=2040;t++)letters.tick(t);assert.equal(letters.tx,'SO');
for(let t=2041;t<=2760;t++)letters.tick(t);assert.equal(letters.tx,'SOS');
console.log('PASS: preview letter completion follows Morse without changing continuous SOS');

const view=new RadioSimulator(data);let clock=0;view.send(clock,true);
while(clock<60000 && view.paperView!=='Certificate')view.tick(++clock);
assert.equal(view.paperView,'Certificate');assert.equal(view.state,'Idle');assert.equal(view.frequency,530);
view.button(clock);assert.equal(view.state,'Tx');assert.equal(view.paperView,'Reception');assert.equal(view.frequency,530);
assert.deepEqual(view.paperText(),['','']);
while(view.state!=='Seek'&&clock<90000)view.tick(++clock);assert.equal(view.state,'Seek');
view.tune(500);while(view.state!=='Done'&&clock<140000)view.tick(++clock);assert.equal(view.state,'Done');
for(let i=0;i<9999;i++)view.tick(++clock);assert.equal(view.paperView,'Reception');
view.tick(++clock);assert.equal(view.paperView,'Certificate');assert.equal(view.frequency,470);
view.button(clock,true);assert.equal(view.state,'Tx');assert.equal(view.frequency,500);assert.equal(view.paperView,'Reception');
view.reset();for(let i=0;i<11000;i++)view.tick(++clock);assert.equal(view.paperView,'Reception');
console.log('PASS: certificate, single-press restart with cleared text, complete TX/tune/RX replay, ten-second final hold, POWER auto-tuned restart, cancellation');

const tuning=new RadioSimulator(data);
assert.equal(tuning.frequencyVisible(0),false);
tuning.tune(471,0);assert(tuning.frequencyVisible(0));assert(tuning.frequencyVisible(2999));assert(!tuning.frequencyVisible(3000));
tuning.tune(472,5000);tuning.tune(473,7999);assert(tuning.frequencyVisible(10998));assert(!tuning.frequencyVisible(10999));
tuning.tune(999,12000);assert.equal(tuning.frequency,550);assert(tuning.frequencyVisible(12000));
tuning.tune(551,14000);assert(tuning.frequencyVisible(16999));assert(!tuning.frequencyVisible(17000));
tuning.button(18000);assert.equal(tuning.state,'Tx');assert(!tuning.frequencyVisible(18000));
tuning.tune(500,18001);assert.equal(tuning.state,'Tx');assert(tuning.frequencyVisible(18001));
for(let t=18001;t<22000;t++)tuning.tick(t);
assert(tuning.tx.length>0);assert(!tuning.frequencyVisible(22000));assert.equal(tuning.paperView,'Reception');
for(let t=22000;t<80000;t++)tuning.tick(t);
assert.equal(tuning.paperView,'Certificate');assert(!tuning.frequencyVisible(80000));
tuning.tune(499,80000);assert(tuning.frequencyVisible(80000));assert.equal(tuning.paperView,'Certificate');
tuning.button(80001);assert.equal(tuning.state,'Tx');assert(!tuning.frequencyVisible(80001));assert.equal(tuning.paperView,'Reception');
console.log('PASS: bulb-frequency view, exact idle timeout, repeated/limit turns, SOS priority, continued Morse, independent certificate and automatic detuning');
const ending=new RadioSimulator(data);let endTime=0;ending.button(0,true);
while(ending.state!=='Done'&&endTime<60000)ending.tick(++endTime);
assert.equal(ending.state,'Done');
const completed=endTime;
while(endTime<completed+9500)ending.tick(++endTime);
ending.tune(501,endTime);
while(endTime<completed+10000)ending.tick(++endTime);
assert.equal(ending.paperView,'Certificate');assert(ending.frequencyVisible(endTime));
assert(ending.frequencyVisible(completed+12499));assert(!ending.frequencyVisible(completed+12500));
console.log('PASS: certificate transition preserves the full three seconds after a recent turn');

// Each running phase stops, stays stopped even at the rescue frequency, and restarts cleanly.
for(const phase of ['Tx','Seek','Rx','Done']){
  const stop=new RadioSimulator(data);let at=0;stop.button(at,phase==='Rx'||phase==='Done');
  while(stop.state!==phase&&at<60000)stop.tick(++at);
  assert.equal(stop.state,phase);const frequency=stop.frequency;
  stop.tune(frequency,at);stop.button(++at);
  assert.equal(stop.state,'Idle');assert.equal(stop.tone,false);assert.deepEqual(stop.paperText(),['','']);
  assert.equal(stop.paperView,'Reception');assert.equal(stop.frequency,frequency);assert(!stop.frequencyVisible(at));
  for(let i=0;i<20000;i++)stop.tick(++at);
  assert.equal(stop.state,'Idle');assert.equal(stop.paperView,'Reception');assert.equal(stop.tone,false);
  stop.button(++at);assert.equal(stop.state,'Tx');assert.deepEqual(stop.paperText(),['','']);
  stop.button(++at);assert.equal(stop.state,'Idle');
}
console.log('PASS: start/stop toggle during TX, tuning wait, RX and final hold; no delayed reply/certificate; clean restart');

const volume=new RadioSimulator(data);volume.send(0);volume.hold();assert.equal(volume.state,'Idle');assert(volume.volumeMode);
volume.turn(-1,100);assert.equal(volume.volume,75);assert.equal(volume.frequency,470);volume.tick(749);assert.equal(volume.sampleUntil,0);
volume.tick(750);assert.equal(volume.sampleUntil,870);volume.tick(800);assert.equal(volume.sampleUntil,870);
volume.button(801);assert(!volume.volumeMode);assert.equal(volume.state,'Idle');volume.button(802);assert.equal(volume.state,'Tx');
volume.reset();assert.equal(volume.volume,75);console.log('PASS: volume mode cancels story, adjusts volume independently, previews after idle and retains preference across reset');
volume.hold(1000);volume.tick(10999);assert(volume.volumeMode);volume.tick(11000);assert(!volume.volumeMode);
volume.hold(12000);volume.turn(1,20000);volume.tick(29999);assert(volume.volumeMode);
volume.tick(30000);assert(!volume.volumeMode);assert.equal(volume.volume,80);assert.equal(volume.frequency,470);
assert(!volume.frequencyVisible(30000));assert.equal(volume.state,'Idle');
console.log('PASS: volume returns to meter after ten seconds from entry or last turn, with preference and tuning retained');
