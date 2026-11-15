#pragma once
#include <stdint.h>
#include <string.h>
namespace paper {
enum class View : uint8_t { Reception, Certificate, Photo };
struct Frame {char tx[48]{};char rx[48]{};char photoPath[32]{};uint32_t generation=0;bool reveal=true;View view=View::Reception;};
inline bool caughtUp(const Frame& shown,const Frame& target){
  return shown.generation==target.generation && shown.view==target.view &&
    (target.view==View::Certificate || (target.view==View::Photo?strcmp(shown.photoPath,target.photoPath)==0:(strcmp(shown.tx,target.tx)==0 && strcmp(shown.rx,target.rx)==0)));
}
inline void stepText(char* out,const char* shown,const char* target){
  const size_t total=strlen(target),have=strlen(shown);
  size_t next=have<=total && strncmp(shown,target,have)==0?have:0;
  // A space appears with the next letter, never as its own invisible refresh.
  if(next<total){while(next<total && target[next]==' ')++next;if(next<total)++next;}
  memcpy(out,target,next);out[next]='\0';
}
inline Frame advance(const Frame& shown,const Frame& target,bool initial=false){
  if(target.view!=View::Reception || !target.reveal)return target;
  Frame next{};next.generation=target.generation;next.reveal=target.reveal;next.view=target.view;
  if(initial || shown.generation!=target.generation)return next; // fresh blank page first
  stepText(next.tx,shown.tx,target.tx);stepText(next.rx,shown.rx,target.rx);
  return next;
}
}
