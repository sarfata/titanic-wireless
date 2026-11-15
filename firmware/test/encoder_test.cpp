#include "Quadrature.h"
#include <cassert>
#include <initializer_list>
#include <iostream>
int feed(radio::Quadrature& q,std::initializer_list<unsigned> samples){
 int steps=0;for(auto s:samples)steps+=q.update(s);return steps;
}
int main(){
 radio::Quadrature q;
 q.reset(3,2);assert(feed(q,{1,0,2,3})==2);
 q.reset(3,2);assert(feed(q,{2,0,1,3})==-2);
 q.reset(3,4);assert(feed(q,{1,0,2,3})==1);
 q.reset(3,4);assert(feed(q,{2,0,1,3})==-1);
 q.reset(3,2);assert(feed(q,{3,1,3,1,1,0,1,0,0,2,0,2,3,2,3})==2);
 q.reset(3,2);assert(feed(q,{1,3,1,3,3})==0); // incomplete detent and reversals
 q.reset(3,2);assert(feed(q,{0})==0); // impossible two-bit jump: resynchronize
 assert(feed(q,{2,3})==1);
 q.reset(3,4);assert(feed(q,{1,0,1,3})==0); // reverse halfway through full cycle
 q.reset(0,2);assert(feed(q,{2,3,1,0})==2);
 q.reset(1,2);assert(feed(q,{0})==0);assert(feed(q,{2,3})==1); // boot mid-transition
 q.reset(3,4);for(int i=0;i<10000;++i)assert(feed(q,{1,0,2,3})==1);
 std::cout<<"PASS: quadrature direction, half/full detents, bounce, reversal, resync and sustained rotation\n";
}
