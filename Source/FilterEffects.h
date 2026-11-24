#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <complex>
namespace eg {
constexpr float fxPi=3.14159265358979323846f;
struct FilterCoeffs {
 float b0=1,b1=0,b2=0,a1=0,a2=0; int stages=1;
 static FilterCoeffs make(int mode,float fc,float q,double sr){
  FilterCoeffs c; if(mode==0)return c;
  fc=std::clamp(fc,20.f,float(sr*.45));q=std::clamp(q,.5f,12.f);
  if(mode==7){float a=std::exp(-2*fxPi*fc/float(sr));c.b0=1-a;c.a1=-a;return c;}
  const float w=2*fxPi*fc/float(sr),co=std::cos(w),si=std::sin(w),alpha=si/(2*q),a0=1+alpha;
  c.a1=-2*co/a0;c.a2=(1-alpha)/a0;
  if(mode==1||mode==5){c.b0=(1-co)*.5f/a0;c.b1=(1-co)/a0;c.b2=c.b0;}
  else if(mode==2||mode==6){c.b0=(1+co)*.5f/a0;c.b1=-(1+co)/a0;c.b2=c.b0;}
  else if(mode==3){c.b0=alpha/a0;c.b1=0;c.b2=-c.b0;}
  else if(mode==4){c.b0=1/a0;c.b1=-2*co/a0;c.b2=c.b0;}
  else return {};
  c.stages=(mode==5||mode==6)?2:1;return c;
 }
 float magnitude(float hz,double sr)const{
  auto z=std::polar(1.0,-2.0*double(fxPi)*std::min(double(hz),sr*.499)/sr),z2=z*z;
  auto h=(double(b0)+double(b1)*z+double(b2)*z2)/(1.0+double(a1)*z+double(a2)*z2);
  float m=float(std::abs(h));return stages==2?m*m:m;
 }
};
struct FilterState {
 std::array<float,2> z1{},z2{};
 float process(float x,const FilterCoeffs& c) noexcept{
  for(int i=0;i<c.stages;++i){float y=c.b0*x+z1[i];z1[i]=c.b1*x-c.a1*y+z2[i];z2[i]=c.b2*x-c.a2*y;x=y;}
  return x;
 }
};
struct StereoFilter {
 FilterState left,right;
 void process(float& l,float& r,const FilterCoeffs& c) noexcept{l=left.process(l,c);r=right.process(r,c);}
};
struct EffectParams {int type=0;bool enabled=true;float amount=.5f,mix=.5f;};
class EffectsRack {
 struct Slot {
  std::vector<float> dl,dr;size_t head=0,valid=0;
  float phase=0,env=0,lowL=0,lowR=0,feedbackL=0,feedbackR=0,wet=0,amount=.5f;
  int type=-1,hilbertHead=0;
  std::array<float,4> apL{},apR{};
  std::array<float,63> hL{},hR{};
  void reset(int t){head=valid=0;phase=env=lowL=lowR=feedbackL=feedbackR=wet=0;hilbertHead=0;apL={};apR={};hL={};hR={};type=t;}
  float read(const std::vector<float>& d,float samples)const{
   samples=std::clamp(samples,1.f,float(d.size()-2));auto n=size_t(samples);float f=samples-float(n);
   if(n>=valid)return 0;auto i=(head+d.size()-n)%d.size(),j=(i+d.size()-1)%d.size();
   return d[i]+f*((n+1<valid?d[j]:0.f)-d[i]);
  }
  void write(float l,float r){dl[head]=l;dr[head]=r;head=(head+1)%dl.size();valid=std::min(valid+1,dl.size()-1);}
 };
 std::array<Slot,12> slots{};std::array<float,63> hilbert{};float rate=48000,smooth=.001f;
 static float allpass(float x,float a,float& z){float y=-a*x+z;z=x+a*y;return y;}
public:
 void prepare(double sr){
  rate=float(std::max(1.0,sr));smooth=1-std::exp(-1/(rate*.015f));
  for(int n=0;n<63;++n){int k=n-31;hilbert[n]=(k%2!=0)?(2.f/(fxPi*k))*(.54f-.46f*std::cos(2*fxPi*n/62.f)):0.f;}
  for(auto& s:slots){s.dl.assign(size_t(rate*1.5f)+4,0);s.dr.assign(size_t(rate*1.5f)+4,0);s.reset(-1);}
 }
 void process(float& l,float& r,const std::array<EffectParams,12>& params,float space){
  if(slots[0].dl.empty())return;
  for(size_t i=0;i<slots.size();++i){auto& s=slots[i];const auto& p=params[i];int t=std::clamp(p.type,0,12);
   if(t!=s.type){s.reset(t);s.amount=std::clamp(p.amount,0.f,1.f);}
   if(t==0)continue;
   float target=p.enabled?std::clamp(p.mix*(.5f+space),0.f,1.f):0.f;
   s.wet+=smooth*(target-s.wet);s.amount+=smooth*(std::clamp(p.amount,0.f,1.f)-s.amount);if(s.wet<.00001f&&target==0)continue;
   float a=s.amount,x=l,y=r,u=x,v=y;
   switch(t){
   case 1:{float drive=1+12*a;u=std::tanh(x*drive)/std::tanh(drive);v=std::tanh(y*drive)/std::tanh(drive);break;}
   case 2:case 8:{// chorus / flanger
    bool chorus=t==2;float base=chorus?.016f:.003f,depth=chorus?.009f:.0025f;
    float d1=(base+depth*a*std::sin(s.phase))*rate,d2=(base+depth*a*std::sin(s.phase+fxPi*.5f))*rate;
    u=s.read(s.dl,d1);v=s.read(s.dr,d2);float fb=chorus?.05f:.6f*a;s.write(x+fb*u,y+fb*v);
    u=(x+u)*.5f;v=(y+v)*.5f;s.phase+=2*fxPi*(.12f+a*.8f)/rate;break;}
   case 3:{float d=(.04f+.91f*a)*rate;u=s.read(s.dl,d);v=s.read(s.dr,d*.75f);s.write(x+.45f*v,y+.45f*u);break;}
   case 4:{// four-tap stereo feedback reverb
    float a1=s.read(s.dl,.031f*rate),a2=s.read(s.dl,.047f*rate),a3=s.read(s.dr,.061f*rate),a4=s.read(s.dr,.079f*rate);
    u=.25f*(a1+a2+a3-a4);v=.25f*(a3+a4+a1-a2);float fb=.35f+.55f*a;
    s.lowL+=.18f*(u-s.lowL);s.lowR+=.18f*(v-s.lowR);s.write(x+fb*s.lowR,y+fb*s.lowL);break;}
   case 5:{float c=1-std::exp(-2*fxPi*900/rate);s.lowL+=c*(x-s.lowL);s.lowR+=c*(y-s.lowR);float g=std::pow(10.f,(a-.5f)*1.2f);u=s.lowL/g+(x-s.lowL)*g;v=s.lowR/g+(y-s.lowR)*g;break;}
   case 6:{float detector=std::max(std::abs(x),std::abs(y)),c=detector>s.env?.01f:.0005f;s.env+=c*(detector-s.env);float th=std::pow(10.f,(-6.f-30.f*a)/20.f);float gain=s.env>th?std::pow(th/std::max(s.env,1e-6f),.75f):1.f;u=x*gain;v=y*gain;break;}
   case 7:{float c=.25f+.6f*(.5f+.5f*std::sin(s.phase));float pL=x,pR=y;for(int j=0;j<4;++j){pL=allpass(pL,c,s.apL[j]);pR=allpass(pR,std::min(.95f,c+.04f),s.apR[j]);}u=.5f*(x+pL);v=.5f*(y+pR);s.phase+=2*fxPi*(.1f+a*1.2f)/rate;break;}
   case 9:{float levels=std::pow(2.f,15.f-12.f*a);u=std::round(x*levels)/levels;v=std::round(y*levels)/levels;break;}
   case 10:{// single-sideband shift using a 63-tap Hilbert transformer
    s.hL[s.hilbertHead]=x;s.hR[s.hilbertHead]=y;float ql=0,qr=0;
    for(int j=0;j<63;++j){int k=(s.hilbertHead+63-j)%63;ql+=hilbert[j]*s.hL[k];qr+=hilbert[j]*s.hR[k];}
    int d=(s.hilbertHead+63-31)%63;float co=std::cos(s.phase),si=std::sin(s.phase);u=s.hL[d]*co-ql*si;v=s.hR[d]*co-qr*si;
    s.hilbertHead=(s.hilbertHead+1)%63;s.phase+=2*fxPi*(a-.5f)*1000/rate;break;}
   case 11:{float mid=.5f*(x+y),side=.5f*(x-y)*2*a;u=mid+side;v=mid-side;break;}
   case 12:{float ceiling=std::pow(10.f,(-.1f-18.f*a)/20.f);float gain=std::min(1.f,ceiling/std::max(1e-8f,std::max(std::abs(x),std::abs(y))));u=x*gain;v=y*gain;break;}
   }
   if(s.phase>2*fxPi)s.phase-=2*fxPi;if(s.phase<0)s.phase+=2*fxPi;
   l=std::clamp(x+s.wet*(u-x),-8.f,8.f);r=std::clamp(y+s.wet*(v-y),-8.f,8.f);
  }
 }
};
}
