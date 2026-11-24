#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace eg {
struct SeqStep {float pitch=0,velocity=.85f,gate=.7f,probability=1;int ratchet=1;bool enabled=true;};
struct SeqParams {
 int mode=0,length=16,division=2,arpMode=0,octaves=1,scale=0,root=0;bool sync=true;
 float bpm=120,swing=0,human=0,arpGate=.7f;
 std::array<SeqStep,64> steps{};
};
inline double stepBeats(int d){constexpr double v[]={1,.5,.25,.125,1./3,1./6};return v[std::clamp(d,0,5)];}
inline int scaleNote(int n,int scale,int root){
 n=std::clamp(n,0,127);if(!scale)return n;
 constexpr int masks[]={4095,2741,1453,661,1193}; // chromatic, major, natural minor, major pentatonic, minor pentatonic
 int mask=masks[std::clamp(scale,0,4)];
 for(int k=0;k<12;++k){int a=n+k,b=n-k;if(b>=0&&(mask&(1<<((b-root+120)%12))))return b;if(a<=127&&(mask&(1<<((a-root+120)%12))))return a;}return n;
}
class NoteSequencer {
 std::array<float,128> held{};std::array<unsigned,128> order{};unsigned serial=0;
 std::array<bool,128> sounding{};std::array<int,128> chord{};std::array<float,128> chordVel{};
 int chordCount=0,step=-1,arpIndex=0,ratchetLeft=0;bool active=false,gateOn=false;int mode=0;
 double untilStep=0,untilRatchet=0,untilOff=0,sub=1;float gate=.7f;double rate=48000;uint32_t rng=0x9182d9acu;
 float random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng>>8)/16777216.f;}
 template<class Emit> void silence(Emit&& emit){for(int n=0;n<128;++n)if(sounding[n]){emit(n,0.f);sounding[n]=false;}gateOn=false;}
 template<class Emit> void play(Emit&& emit){silence(emit);for(int j=0;j<chordCount;++j){int n=chord[j];if(!sounding[n]){emit(n,chordVel[j]);sounding[n]=true;}}gateOn=chordCount>0;untilOff=std::max(1.,sub*gate);}
 void add(int n,float v,const SeqParams& p){n=scaleNote(n,p.scale,p.root);for(int j=0;j<chordCount;++j)if(chord[j]==n)return;if(chordCount<128){chord[chordCount]=n;chordVel[chordCount++]=std::clamp(v,.01f,1.f);}}
 bool any()const{for(auto v:held)if(v>0)return true;return false;}
 void makeChord(const SeqParams& p){
  chordCount=0;auto& s=p.steps[step];if(p.mode==1){
   if(!s.enabled||random()>=s.probability)return;
   for(int n=0;n<128;++n)if(held[n]>0)add(n+int(s.pitch),held[n]*s.velocity*(1+p.human*.18f*(2*random()-1)),p);
  }else{
   std::array<int,128> notes{};int count=0;for(int n=0;n<128;++n)if(held[n]>0)notes[count++]=n;if(!count)return;
   if(p.arpMode==4){for(int j=0;j<count;++j)add(notes[j]+12*(arpIndex%std::max(1,p.octaves)),held[notes[j]],p);++arpIndex;return;}
   if(p.arpMode==5)std::sort(notes.begin(),notes.begin()+count,[&](int a,int b){return order[a]<order[b];});
   int total=count*std::clamp(p.octaves,1,4),idx=arpIndex%total;
   if(p.arpMode==1)idx=total-1-idx;
   if(p.arpMode==2){int period=std::max(1,2*total-2),k=arpIndex%period;idx=k<total?k:period-k;}
   if(p.arpMode==3)idx=std::min(total-1,int(random()*total));
   if(p.arpMode==6&&(!s.enabled||random()>=s.probability)){++arpIndex;return;}
   int n=notes[idx%count],pitch=n+12*(idx/count)+(p.arpMode==6?int(s.pitch):0);
   add(pitch,held[n]*(p.arpMode==6?s.velocity:1.f),p);++arpIndex;
  }
 }
public:
 void prepare(double sr){rate=std::max(1.,sr);held={};order={};sounding={};serial=0;mode=0;step=-1;arpIndex=0;active=gateOn=false;untilStep=untilRatchet=untilOff=0;ratchetLeft=0;chordCount=0;rng=0x9182d9acu;}
 int currentStep()const{return active?step:-1;}
 template<class Emit> void configure(int m,Emit&& emit){
  if(m==mode)return;
  if(mode==0){for(int n=0;n<128;++n)if(held[n]>0)emit(n,0.f);}else silence(emit);
  mode=m;active=false;step=-1;arpIndex=0;ratchetLeft=0;
  if(mode==0){for(int n=0;n<128;++n)if(held[n]>0)emit(n,held[n]);}
 }
 template<class Emit> void note(int n,float v,Emit&& emit){
  if(n<0||n>127)return;held[n]=std::clamp(v,0.f,1.f);if(v>0)order[n]=++serial;
  if(mode==0){emit(n,v);return;}
  if(!any()){silence(emit);active=false;step=-1;arpIndex=0;ratchetLeft=0;}
 }
 template<class Emit> void allOff(Emit&& emit){if(mode==0){for(int n=0;n<128;++n)if(held[n]>0)emit(n,0.f);}else silence(emit);held={};active=false;step=-1;ratchetLeft=0;arpIndex=0;}
 template<class Emit> void tick(const SeqParams& p,Emit&& emit){
  if(!mode)return;
  if(!active){if(!any())return;active=true;untilStep=0;step=-1;arpIndex=0;}
  if(untilStep<=0){
   silence(emit);step=(step+1)%std::clamp(p.length,1,64);
   double base=rate*60/std::clamp(double(p.bpm),20.,400.)*stepBeats(p.division);
   double duration=base*(1+(step%2==0?1:-1)*std::clamp(p.swing,0.f,.49f));
   duration*=1+double(p.human)*.08*(2*random()-1);
   untilStep+=std::max(8.,duration);auto& s=p.steps[step];
   int count=(mode==1||p.arpMode==6)?std::clamp(s.ratchet,1,4):1;
   sub=std::max(2.,duration/count);gate=std::clamp(mode==1||p.arpMode==6?s.gate:p.arpGate,.05f,.98f);
   makeChord(p);play(emit);ratchetLeft=count-1;untilRatchet=sub;
  }else{
   if(gateOn&&untilOff<=0)silence(emit);
   if(ratchetLeft>0&&untilRatchet<=0){play(emit);--ratchetLeft;untilRatchet+=sub;}
  }
  untilStep-=1;untilRatchet-=1;untilOff-=1;
 }
};
struct MotionPoint {float x=.5f,y=.5f;};
struct MotionParams {
 int mode=0,length=16;bool sync=false;float rate=.2f,depth=.75f,bpm=120;
 std::array<MotionPoint,64> path{};
};
class MotionClock {
 double phase=0;uint32_t rng=0x56b1c4u;MotionPoint previous{},next{};int lastMode=0;
 float random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng>>8)/16777216.f;}
public:
 void reset(){phase=0;lastMode=0;previous=next={.5f,.5f};rng=0x56b1c4u;}
 MotionPoint advance(const MotionParams& p,MotionPoint base,double seconds){
  if(p.mode!=lastMode){phase=0;previous=next=base;lastMode=p.mode;}
  if(!p.mode)return base;
  constexpr double tau=6.283185307179586;
  double old=phase;phase+=seconds*std::clamp(double(p.rate),.01,20.)*(p.sync?std::clamp(double(p.bpm),20.,400.)/60.:1.);phase-=std::floor(phase);
  if(phase<old){previous=next;next={random(),random()};}
  float t=float(phase),a=float(tau*phase);MotionPoint q=base;
  switch(p.mode){
   case 1:q={.5f+.5f*std::cos(a),.5f+.5f*std::sin(a)};break;
   case 2:q={.5f+.5f*t*std::cos(3*a),.5f+.5f*t*std::sin(3*a)};break;
   case 3:q={previous.x+(next.x-previous.x)*t,previous.y+(next.y-previous.y)*t};break;
   case 4:q={.5f+.5f*std::sin(a),.5f+.25f*std::sin(2*a)};break;
   case 5:{int len=std::clamp(p.length,1,64);float pos=t*len;int i=std::min(len-1,int(pos)),j=(i+1)%len;float f=pos-i;q={p.path[i].x+(p.path[j].x-p.path[i].x)*f,p.path[i].y+(p.path[j].y-p.path[i].y)*f};break;}
  }
  float d=std::clamp(p.depth,0.f,1.f);return {std::clamp(base.x+(q.x-.5f)*d,0.f,1.f),std::clamp(base.y+(q.y-.5f)*d,0.f,1.f)};
 }
};
}
