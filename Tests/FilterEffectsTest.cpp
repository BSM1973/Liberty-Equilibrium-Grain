#include "../Source/FilterEffects.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
void require(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  for(int mode=0;mode<=7;++mode){
   auto c=eg::FilterCoeffs::make(mode,1500,.707f,sr);
   eg::StereoFilter st;
   double energy=0;
   for(int n=0;n<16384;++n){float l=n==0?1.f:0.f,r=l;st.process(l,r,c);require(std::isfinite(l)&&std::abs(l)<10,"filter stability");energy+=l*l;}
   require(energy>0,"filter silence");
   require(std::isfinite(c.magnitude(1000,sr)),"filter curve");
  }
  auto lp=eg::FilterCoeffs::make(1,1500,.707f,sr),hp=eg::FilterCoeffs::make(2,1500,.707f,sr);
  require(lp.magnitude(100,sr)>lp.magnitude(10000,sr),"lowpass");
  require(hp.magnitude(100,sr)<hp.magnitude(10000,sr),"highpass");
  eg::EffectsRack rack;rack.prepare(sr);std::array<eg::EffectParams,12> p{};
  float l=.2f,r=-.1f;rack.process(l,r,p,.5f);require(l==.2f&&r==-.1f,"empty rack transparency");
  for(int t=1;t<=12;++t){
   p={};p[0]={t,true,.81f,1.f};double diff=0;int N=int(sr*.32);
   for(int n=0;n<N;++n){float x=.6f*std::sin(2*eg::fxPi*440*n/float(sr)),y=.37f*std::sin(2*eg::fxPi*613*n/float(sr));float a=x,b=y;rack.process(a,b,p,.5f);require(std::isfinite(a)&&std::isfinite(b)&&std::abs(a)<=8&&std::abs(b)<=8,"effect stability");diff+=std::abs(a-x)+std::abs(b-y);}
   require(diff>1,"effect must alter audio");
   p[0].enabled=false;
   for(int n=0;n<int(sr*.2);++n){float a=.3f,b=-.2f;rack.process(a,b,p,.5f);if(n==int(sr*.2)-1)require(std::abs(a-.3f)<.001f&&std::abs(b+.2f)<.001f,"bypass settles");}
  }
  for(int i=0;i<12;++i)p[i]={i+1,true,.6f,.35f};
  for(int n=0;n<8192;++n){float a=.4f*std::sin(n*.06f),b=.4f*std::cos(n*.07f);rack.process(a,b,p,.5f);require(std::isfinite(a)&&std::isfinite(b),"all slots stable");}
  std::cout<<sr<<" Hz: filters 0-7, 12 effects, bypass and full rack PASS\n";
 }
}
