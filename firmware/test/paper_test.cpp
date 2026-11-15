#include "PaperReveal.h"
#include "PaperViews.h"
#include "RadioLogic.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <utility>
struct Completed {uint32_t now=0;std::vector<std::pair<uint16_t,uint32_t>> chars;};
void record(const radio::Event& e,uint32_t,void* ctx){auto& c=*static_cast<Completed*>(ctx);if(e.charsFinished)c.chars.emplace_back(e.charsFinished,c.now);}
int main(){
  Completed c;radio::Player player;player.start("SOS",120,0,record,&c);
  for(c.now=1;player.active()&&c.now<4000;++c.now)player.tick(c.now);
  assert((c.chars==std::vector<std::pair<uint16_t,uint32_t>>{{1,600},{2,2040},{3,2760}}));
  assert(radio::normalizedMessage("  sos\n we   need # help ")=="SOS WE NEED HELP");
  assert(radio::prefixCharacters("SOS WE NEED HELP",5)=="SOS W");
  for(const char* text:{"SOS WE NEED HELP","WE ARE COMING"}){
    unsigned units=0,count=0,last=0;
    for(auto e:radio::encode(text)){units+=e.units;if(e.charsFinished){assert(e.charsFinished>last);assert(text[e.charsFinished-1]!=' ');last=e.charsFinished;++count;}}
    assert(last==std::strlen(text));assert(count==std::strlen(text)-std::count(text,text+std::strlen(text),' '));assert(units>0);
  }
  paper::Frame target{};target.generation=1;std::strcpy(target.tx,"SOS WE NEED HELP");std::strcpy(target.rx,"WE ARE COMING");
  paper::Frame shown{};shown=paper::advance(shown,target,true);
  assert(shown.generation==1 && shown.tx[0]==0 && shown.rx[0]==0);
  const char* expected[]={"S","SO","SOS","SOS W","SOS WE","SOS WE N","SOS WE NE","SOS WE NEE","SOS WE NEED","SOS WE NEED H","SOS WE NEED HE","SOS WE NEED HEL","SOS WE NEED HELP"};
  for(const char* next:expected){shown=paper::advance(shown,target);assert(std::strcmp(shown.tx,next)==0);}
  assert(paper::caughtUp(shown,target));assert(paper::caughtUp(paper::advance(shown,target),target));
  ++target.generation;target.tx[0]=target.rx[0]=0;
  shown=paper::advance(shown,target);assert(shown.tx[0]==0&&shown.rx[0]==0&&paper::caughtUp(shown,target));
  std::strcpy(target.tx,"SOS");shown=paper::advance(shown,target);assert(std::strcmp(shown.tx,"S")==0);
  // Coalesced targets still reveal each intervening letter, one refresh at a time.
  std::strcpy(target.tx,"SOS WE");shown=paper::advance(shown,target);assert(std::strcmp(shown.tx,"SO")==0);
  // A new generation cancels stale catch-up rather than displaying old message letters.
  ++target.generation;shown=paper::advance(shown,target);assert(shown.tx[0]==0);
  // A deliberate static diagnostic page appears all at once.
  target.reveal=false;shown=paper::advance(shown,target);assert(paper::caughtUp(shown,target));
  // Exactly full buffers terminate safely, and a changed target can restart a prefix.
  target.reveal=true;std::memset(target.tx,'E',47);target.tx[47]=0;shown=paper::Frame{};shown.generation=target.generation;
  for(int i=0;i<47;++i)shown=paper::advance(shown,target);
  assert(std::strlen(shown.tx)==47&&paper::caughtUp(shown,target));
  std::strcpy(target.tx,"HELP");shown=paper::advance(shown,target);assert(std::strcmp(shown.tx,"H")==0);
  target.view=paper::View::Certificate;
  assert(!paper::caughtUp(shown,target));shown=paper::advance(shown,target);
  assert(shown.view==paper::View::Certificate && paper::caughtUp(shown,target));
  target.view=paper::View::Reception;target.reveal=false;++target.generation;
  shown=paper::advance(shown,target);assert(shown.view==paper::View::Reception && paper::caughtUp(shown,target));
  paper::Views views;paper::Frame complete{};complete.generation=4;
  std::strcpy(complete.tx,"SOS WE NEED HELP");std::strcpy(complete.rx,"WE ARE COMING");
  views.complete(complete);assert(!views.tick(100000,10000)); // Wait for final pixels, not audio end.
  paper::Frame ack=complete;ack.generation=3;views.presented(ack,100000);assert(!views.tick(120000,10000));
  ack=complete;ack.rx[11]=0;views.presented(ack,120000);assert(!views.tick(140000,10000)); // Missing final letter.
  ack=complete;ack.view=paper::View::Certificate;views.presented(ack,120000);assert(!views.tick(140000,10000));
  ack=complete;views.presented(ack,140000);views.presented(ack,145000); // Duplicate must not extend hold.
  assert(!views.tick(149999,10000));assert(views.tick(150000,10000));assert(views.view==paper::View::Certificate);
  assert(!views.tick(160000,10000));views.newRound();assert(views.view==paper::View::Reception);
  views.complete(complete);views.presented(complete,UINT32_MAX-5000);
  assert(!views.tick(4998,10000));assert(views.tick(4999,10000));
  views.complete(complete);views.presented(complete,0);views.newRound();assert(!views.tick(10000,10000));
  views.complete(complete);views.presented(complete,101);assert(!views.tick(100,10000));
  assert(!views.tick(10100,10000));assert(views.tick(10101,10000));
  // Re-enabling the display during completion waits for its new generation.
  views.complete(complete);++complete.generation;views.complete(complete);
  views.presented(ack,0);assert(!views.tick(20000,10000));views.presented(complete,20000);assert(views.tick(30000,10000));
  // Stop while the final refresh is in flight: its late ack must not award a certificate.
  views.complete(complete);views.newRound();views.presented(complete,40000);
  assert(!views.tick(50000,10000));assert(views.view==paper::View::Reception);
  paper::Frame stopped{};stopped.generation=complete.generation+1;
  shown=paper::advance(complete,stopped);
  assert(paper::caughtUp(shown,stopped));assert(shown.tx[0]==0&&shown.rx[0]==0);
  target.view=paper::View::Photo;std::strcpy(target.photoPath,"/photo-000001.pbm");
  views.complete(complete);views.photo(target);views.presented(complete,60000);
  assert(!views.tick(80000,10000)&&views.view==paper::View::Photo);
  shown=paper::advance(shown,target);assert(paper::caughtUp(shown,target));
  views.presented(shown,80000);views.presented(shown,81000); // A duplicate cannot extend the hold.
  assert(!views.tick(79999,10000));assert(!views.tick(89999,10000));assert(views.tick(90000,10000));
  assert(views.view==paper::View::Reception&&!views.tick(100000,10000));
  std::strcpy(target.photoPath,"/photo-000002.pbm");assert(!paper::caughtUp(shown,target));
  views.photo(target);views.presented(shown,100000);assert(!views.tick(120000,10000));
  views.presented(target,UINT32_MAX-5000);assert(!views.tick(4998,10000));assert(views.tick(4999,10000));
  // Starting a round or requesting a newer photo invalidates old refresh acknowledgments.
  views.photo(target);views.newRound();views.presented(target,0);assert(!views.tick(20000,10000));
  views.photo(target);auto stale=target;++target.generation;views.photo(target);
  views.presented(stale,0);assert(!views.tick(20000,10000));views.presented(target,20000);assert(views.tick(30000,10000));
  views.newRound();assert(views.view==paper::View::Reception);
  std::cout<<"PASS: photo hold starts at matching refresh, expires at ten seconds, handles duplicates/rollover/cancellation/new photos\n";
  std::cout<<"PASS: completion after last tone, unchanged SOS prosign, normalized prefixes, every-letter catch-up, generation reset, static test, buffer boundary, page changes, final-letter-timed hold, timer rollover/cancellation\n";
}
