#include "../Source/PerformanceSequencer.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <utility>
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){
 using namespace eg;
 for(double sr:{44100.,48000.,96000.,192000.}){
  NoteSequencer s;s.prepare(sr);SeqParams p;p.mode=1;p.length=64;p.bpm=120;p.division=2;int clock=0;std::vector<std::pair<int,int>> on,off;
  auto emit=[&](int n,float v){(v>0?on:off).emplace_back(clock,n);};
  s.configure(1,emit);s.note(60,1,emit);s.note(64,1,emit);
  for(int i=0;i<64;++i)p.steps[i].pitch=float(i%12);p.steps[63].pitch=12;
  int len=int(std::ceil(sr*.125));for(clock=0;clock<len*64+1;++clock)s.tick(p,emit);
  check(on.size()>=128,"64 polyphonic steps");check(on[126].second==72&&on[127].second==76,"step 64");s.allOff(emit);check(s.currentStep()==-1,"all off");
  s.prepare(sr);on.clear();off.clear();p={};p.mode=1;p.steps[0].ratchet=4;p.steps[1].probability=0;s.configure(1,emit);s.note(60,1,emit);
  for(clock=0;clock<int(sr*.25);++clock)s.tick(p,emit);check(on.size()==4,"ratchet and probability");
  s.prepare(sr);on.clear();off.clear();p={};p.mode=2;p.arpMode=2;s.configure(2,emit);s.note(60,1,emit);s.note(64,1,emit);s.note(67,1,emit);
  for(clock=0;clock<len*5+1;++clock)s.tick(p,emit);check(on.size()>=5&&on[0].second==60&&on[1].second==64&&on[2].second==67&&on[3].second==64&&on[4].second==60,"up down");
  s.prepare(sr);on.clear();off.clear();p={};p.mode=1;p.swing=.2f;s.configure(1,emit);s.note(60,1,emit);for(clock=0;clock<len*3;++clock)s.tick(p,emit);check(on.size()>=3&&std::abs(on[1].first-len*1.2)<3&&std::abs(on[2].first-len*2.)<3,"swing");
  MotionClock m;MotionParams q;q.mode=1;q.rate=1;q.depth=1;auto a=m.advance(q,{.5f,.5f},.25);check(std::abs(a.x-.5f)<.001&&a.y>.99f,"circle");q.mode=5;q.length=2;q.path[0]={0,0};q.path[1]={1,1};a=m.advance(q,{.5f,.5f},.25);check(std::abs(a.x-.5f)<.001&&std::abs(a.y-.5f)<.001,"path interpolate");
  std::cout<<sr<<" Hz: polyphonic sequence, step 64, ratchets, probability, arpeggio, swing and motion PASS\n";
 }
}
