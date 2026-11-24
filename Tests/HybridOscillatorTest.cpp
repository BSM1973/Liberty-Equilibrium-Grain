#include "../Source/GrainEngine.h"
#include <memory>
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <chrono>
void require(bool x,const char* s){if(!x)throw std::runtime_error(s);}
int main(){
 using namespace eg;const auto& bank=HybridTables::instance();
 for(double sr:{44100.,48000.,96000.,192000.}){
  std::set<uint64_t> fingerprints;
  for(int mode=0;mode<10;++mode){HybridVoice v;HybridConfig c;HybridParams p;p.mode=mode;p.shape=.27f;p.index=2;c.set(p);double e=0;uint64_t hash=1469598103934665603ull;for(int i=0;i<8192;++i){auto x=v.process(bank,c,440,220,float(sr));require(std::isfinite(x.l)&&std::abs(x.l)<3,"finite oscillator");e+=x.l*x.l;uint32_t b;std::memcpy(&b,&x.l,4);hash=(hash^b)*1099511628211ull;}require(e>1,"audible oscillator");require(fingerprints.insert(hash).second,"unique mode");}
  HybridVoice v;HybridConfig c;HybridParams p;p.mode=5;p.unison=16;p.detune=30;p.width=1;p.sync=true;p.sub=.3f;c.set(p);double side=0;for(int i=0;i<8192;++i){auto x=v.process(bank,c,880,440,float(sr));require(std::isfinite(x.l)&&std::isfinite(x.r),"sync finite");side+=std::abs(x.l-x.r);}require(side>1,"unison stereo");
  HybridVoice a,b;HybridConfig ca,cb;p={};p.mode=9;p.shape=0;ca.set(p);p.shape=1;cb.set(p);double diff=0;for(int i=0;i<4096;++i)diff+=std::abs(a.process(bank,ca,220,220,float(sr)).l-b.process(bank,cb,220,220,float(sr)).l);require(diff>1,"spectral morph");
  auto e=std::make_unique<Engine>();e->prepare(sr);Settings s;for(auto& l:s.layers)l.enabled=false;s.hybrid[0].mode=2;s.hybrid[0].unison=16;s.hybrid[0].detune=20;s.hybrid[1].mode=6;s.osc1=.6f;s.osc2=.3f;s.fm=.4f;s.ring=.3f;e->noteOn(60,.9f);e->noteOn(67,.8f);std::vector<float> l(4096),r(l.size());e->render(l.data(),r.data(),int(l.size()),s);double energy=0;for(float x:l){require(std::isfinite(x)&&std::abs(x)<=1,"engine stability");energy+=x*x;}require(energy>.001,"engine routing");
  // A coherent high note exercises the harmonic-limited saw table.
  HybridVoice alias;HybridConfig cfg;HybridParams ap;ap.mode=2;cfg.set(ap);double total=0,real1=0,imag1=0,real2=0,imag2=0;constexpr int N=1024;
  for(int i=0;i<N;++i){double x=alias.process(bank,cfg,float(sr/8),float(sr/8),float(sr)).l;total+=x*x;double ph=2*3.141592653589793*i/8;real1+=x*std::cos(ph);imag1+=x*std::sin(ph);real2+=x*std::cos(2*ph);imag2+=x*std::sin(2*ph);}
  double allowed=2*(real1*real1+imag1*imag1+real2*real2+imag2*imag2)/N;require(total>1&&std::abs(total-allowed)/total<.001,"bandlimited saw harmonic check");
  std::cout<<sr<<" Hz: ten modes, sync, spectral, unison, bandlimited saw and engine PASS\n";
  if(sr==48000){e->panic();s.hybrid[1].unison=16;s.hybrid[1].detune=22;s.hybrid[1].mode=5;for(int n=0;n<16;++n)e->noteOn(48+n,.8f);auto begin=std::chrono::steady_clock::now();for(int k=0;k<8;++k)e->render(l.data(),r.data(),int(l.size()),s);auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();std::cout<<"16 notes / 2 x 16 unison, hybrid only, render ratio "<<elapsed/(8*l.size()/sr)<<"\n";}

 }
}
