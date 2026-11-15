#pragma once
#include <stdint.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <algorithm>
namespace radio {
inline bool reached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}
inline int counterDelta(uint16_t now, uint16_t before) {
  uint16_t raw = uint16_t(now - before);
  return raw < 0x8000 ? int(raw) : int(raw) - 65536;
}
inline int signalStrength(int frequency) {
  return std::max(0, 100 - std::abs(frequency - 500) * 4);
}
inline const char* code(char c) {
  static const char* letters[]={".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--.."};
  static const char* numbers[]={"-----",".----","..---","...--","....-",".....","-....","--...","---..","----."};
  c=char(toupper(static_cast<unsigned char>(c)));
  if(c>='A' && c<='Z') return letters[c-'A'];
  if(c>='0' && c<='9') return numbers[c-'0'];
  if(c=='.') return ".-.-.-";
  if(c=='?') return "..--..";
  return "";
}
// Completion offsets refer to the normalized message (one space between words).
inline std::string normalizedMessage(const std::string& text) {
  std::string out;bool space=false;
  for(char c:text){
    if(isspace(static_cast<unsigned char>(c))){space=!out.empty();continue;}
    if(!*code(c))continue;
    if(space){out+=' ';space=false;}
    out+=char(toupper(static_cast<unsigned char>(c)));
  }
  return out;
}
struct Event {
  bool tone;uint8_t units,wordFinished;uint16_t charsFinished;
  Event(bool keyed,uint8_t duration,uint8_t word,uint16_t chars=0)
    :tone(keyed),units(duration),wordFinished(word),charsFinished(chars){}
};
inline std::vector<Event> encode(const std::string& text) {
  const std::string clean=normalizedMessage(text);
  std::vector<Event> out;uint8_t word=0;
  for(size_t begin=0;begin<clean.size();){
    size_t end=clean.find(' ',begin);if(end==std::string::npos)end=clean.size();
    const bool sos=clean.substr(begin,end-begin)=="SOS";
    ++word;
    for(size_t ci=begin;ci<end;++ci){
      const std::string symbols=code(clean[ci]);
      for(size_t si=0;si<symbols.size();++si){
        out.push_back({true,uint8_t(symbols[si]=='-'?3:1),0});
        if(si+1<symbols.size())out.push_back({false,1,0});
      }
      // Attach completion to the following gap, after the final tone ends.
      // SOS stays one continuous prosign: use a one-unit gap between its letters.
      if(ci+1<end)out.push_back({false,uint8_t(sos?1:3),0,uint16_t(ci+1)});
      else out.push_back({false,0,word,uint16_t(ci+1)});
    }
    if(end<clean.size())out.push_back({false,7,0});
    begin=end+1;
  }
  return out;
}
inline std::string prefixCharacters(const std::string& text,uint16_t count){
  return normalizedMessage(text).substr(0,count);
}
class Player {
 public:
  using Callback=void(*)(const Event&,uint32_t,void*);
  void start(const std::string& text,uint16_t dot,uint32_t now,Callback cb,void* ctx=nullptr) {
    events_=encode(text);index_=0;dot_=dot;cb_=cb;ctx_=ctx;active_=!events_.empty();
    if(active_) dispatch(now);
  }
  void tick(uint32_t now) {if(active_ && reached(now,deadline_)){++index_;dispatch(now);}}
  void stop(){active_=false;events_.clear();}
  bool active() const{return active_;}
 private:
  void dispatch(uint32_t now) {
    while(index_<events_.size()) {
      const auto e=events_[index_];
      if(cb_) cb_(e,uint32_t(e.units)*dot_,ctx_);
      if(e.units){deadline_=now+uint32_t(e.units)*dot_;return;}
      ++index_;
    }
    active_=false;
  }
  std::vector<Event> events_;size_t index_=0;uint16_t dot_=120;
  uint32_t deadline_=0;bool active_=false;Callback cb_=nullptr;void* ctx_=nullptr;
};
class Debounce {
 public:
  void reset(bool pressed,uint32_t now){raw_=stable_=pressed;changed_=now;}
  // True exactly once when a stable rising edge is accepted.
  bool update(bool pressed,uint32_t now) {
    if(pressed!=raw_){raw_=pressed;changed_=now;}
    if(raw_!=stable_ && uint32_t(now-changed_)>=25){stable_=raw_;return stable_;}
    return false;
  }
  bool held() const{return stable_;}
 private:bool raw_=false,stable_=false;uint32_t changed_=0;
};
inline std::string prefixWords(const std::string& text,uint8_t count) {
  if(!count)return "";
  size_t p=0;
  while(--count){p=text.find(' ',p);if(p==std::string::npos)return text;++p;}
  p=text.find(' ',p);return p==std::string::npos?text:text.substr(0,p);
}
}
