#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "UserWavetable.h"
namespace eg {
constexpr float hyPi=3.14159265358979323846f;
inline float hyWrap(float p){return p-std::floor(p);}
struct HybridParams {
 int mode=0,unison=1;float detune=0,fine=0,width=.7f,shape=.5f,ratio=2,index=1,sub=0;bool sync=false;
 bool legacy()const{return mode==0&&unison==1&&fine==0&&sub==0&&!sync;}
};
struct HybridStereo {float l=0,r=0;float mid()const{return .5f*(l+r);}};
class HybridTables {
public:
 static constexpr int N=1024,Tiers=9,Shapes=8;
 // sine, triangle, saw, square, organ, metallic, spectral A and B
 std::array<std::array<std::array<float,N+1>,Tiers>,Shapes> data{};
 HybridTables(){
  std::array<std::array<float,N>,Shapes> accum{};
  int tier=0;
  for(int h=1;h<=256;++h){
   float inv=1.f/h;bool odd=(h&1)!=0;float coeff[Shapes]={h==1?1.f:0.f,odd?((h%4==1?1.f:-1.f)*inv*inv)*.81f:0.f,-.62f*inv,odd?1.15f*inv:0.f,
    h==1?.65f:h==2?.25f:h==3?.20f:h==4?.12f:h==6?.08f:0.f,
    h==1?.43f:h==3?.30f:h==5?.22f:h==8?.16f:h==13?.12f:h==21?.08f:0.f,
    h==1?.45f:.40f*std::exp(-.5f*std::pow((h-5.f)/2.0f,2.f))*inv*3,
    h==1?.32f:.55f*std::exp(-.5f*std::pow((h-13.f)/3.5f,2.f))*inv*8};
   for(int i=0;i<N;++i){float wave=std::sin(2*hyPi*h*i/N);for(int s=0;s<Shapes;++s)accum[s][i]+=coeff[s]*wave;}
   if(h==(1<<tier)){for(int s=0;s<Shapes;++s){float peak=1;for(float x:accum[s])peak=std::max(peak,std::abs(x));for(int i=0;i<N;++i)data[s][tier][i]=accum[s][i]/peak;data[s][tier][N]=data[s][tier][0];}if(tier<Tiers-1)++tier;}
  }
 }
 static const HybridTables& instance(){static const HybridTables b;return b;}
 static int band(float dt){float harmonics=.44f/std::max(dt,1e-7f);int t=0;while(t<Tiers-1&&harmonics>=float(2<<t))++t;return t;}
 float lookup(int shape,int tier,float phase)const noexcept{float x=hyWrap(phase)*N;int i=std::min(N-1,int(x));float f=x-i;auto& a=data[shape][tier];return a[i]+f*(a[i+1]-a[i]);}
 float wave(int mode,int tier,float phase,float shape)const noexcept{
  shape=std::clamp(shape,0.f,1.f);
  switch(mode){
   case 1:return lookup(1,tier,phase);
   case 2:return lookup(2,tier,phase);
   case 3:return lookup(3,tier,phase);
   case 4:{float duty=.05f+.9f*shape;return .8f*(lookup(2,tier,phase)-lookup(2,tier,phase+duty));}
   case 5:{constexpr int frames[]={0,4,5,2};float x=shape*3;int i=std::min(2,int(x));float f=x-i;float a=lookup(frames[i],tier,phase),b=lookup(frames[i+1],tier,phase);return a+(b-a)*f;}
   case 9:{float a=lookup(6,tier,phase),b=lookup(7,tier,phase);return a+(b-a)*shape;}
   default:return lookup(0,tier,phase);
  }
 }
};
struct HybridConfig {
 HybridParams p{};const UserWavetable* user=nullptr;std::array<float,16> ratio{},left{},right{};int count=1;
 void set(const HybridParams& x){
  p=x;count=std::clamp(p.unison,1,16);
  for(int i=0;i<count;++i){float pos=count==1?0.f:2.f*i/(count-1)-1.f;ratio[i]=std::exp2((p.fine+pos*p.detune)/1200.f);float pan=pos*std::clamp(p.width,0.f,1.f);left[i]=std::sqrt(1-pan)/count;right[i]=std::sqrt(1+pan)/count;}
 }
};
class HybridVoice {
 std::array<float,16> phases{},mods{},subPhases{},carry{};
 float syncPhase=0;uint32_t rng=0x7abf361du;int previousCount=0;
 float noise(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng>>8)/8388608.f-1.f;}
public:
 void seed(uint32_t x){rng=x?x:1;}
 HybridStereo process(const HybridTables& table,const HybridConfig& c,float frequency,float fundamental,float sampleRate,float pm=0)noexcept{
  auto& p=c.p;HybridStereo result{};
  float masterDt=std::clamp(fundamental/sampleRate,0.f,.45f);float before=syncPhase;syncPhase=hyWrap(syncPhase+masterDt);bool reset=p.sync&&syncPhase<before;
  float remainder=reset?syncPhase/std::max(masterDt,1e-8f):0;
  if(c.count!=previousCount){for(int i=previousCount;i<c.count;++i){phases[i]=c.count==1?phases[0]:hyWrap(phases[0]+i*.381966f);mods[i]=phases[i];subPhases[i]=phases[i];carry[i]=0;}previousCount=c.count;}
  for(int i=0;i<c.count;++i){
   float dt=std::clamp(frequency*c.ratio[i]/sampleRate,0.f,.45f);
   float phase=hyWrap(phases[i]+dt),m=0;
   // Internal FM is sine phase modulation with a conservative bandwidth cap.
   float ratio=std::clamp(p.ratio,.25f,8.f);
   if(p.mode==6){mods[i]=hyWrap(mods[i]+std::min(dt*ratio,.45f));float index=std::min(p.index,std::max(0.f,(.43f/std::max(dt,1e-7f)-1.f)/ratio-2.f));m=index*std::sin(2*hyPi*mods[i]);}
   float phaseOffset=(m+pm)/(2*hyPi);
   int band=HybridTables::band(dt*(1+std::min(8.f,std::abs(pm))));
   auto read=[&](float at){return p.mode==10?(c.user?c.user->read(band,at+phaseOffset,p.shape):0.f):table.wave(p.mode,band,at+phaseOffset,p.shape);};
   float out=carry[i];carry[i]=0;
   if(p.mode==7)out=noise();
   else if(p.mode==8){subPhases[i]=hyWrap(subPhases[i]+dt*.5f);out=table.lookup(0,0,subPhases[i]+phaseOffset);}
   else{
    if(reset){float prior=read(phase-dt*remainder);float jump=read(0)-prior;phase=dt*remainder;
     // Short fractional-reset smoothing; not a full bandlimited hard-sync model.
     carry[i]=-.125f*jump*(1-remainder);out-=.5f*jump*(1-remainder)*(1-remainder);}
    out+=read(phase);
   }
   phases[i]=phase;
   if(p.sub>0&&p.mode!=8){subPhases[i]=hyWrap(subPhases[i]+dt*.5f);out=out*(1-.5f*p.sub)+table.lookup(0,0,subPhases[i])*.5f*p.sub;}
   result.l+=out*c.left[i];result.r+=out*c.right[i];
  }
  return result;
 }
};
}
