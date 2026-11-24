#include "../Source/FactoryPresets.h"
#include "../Source/SpectralEngine.h"
#include <chrono>
#include <memory>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
void check(bool x,const char* s){if(!x)throw std::runtime_error(s);}
using Clock=std::chrono::steady_clock;
int main(){
 std::filesystem::create_directories("quality-reports");std::ofstream report("quality-reports/dsp-performance.csv");report<<"scenario,sample_rate,buffer,voices,seconds_audio,mean_budget_pct,p95_budget_pct,p99_budget_pct,max_budget_pct,overruns,blocks,peak,rms,clipped_samples\n";
 auto e=std::make_unique<eg::Engine>();auto spectral=std::make_unique<eg::SpectralEngine>();
 auto bench=[&](const char* name,int scenario,double rate,int buffer,double seconds){
  e->prepare(rate);spectral->prepare(rate);eg::Settings p;for(auto& l:p.layers){l.enabled=scenario!=1;l.density=64;l.size=250;l.level=.3f;l.motion=.12f;l.spread=.8f;}p.osc1=scenario==0?0:.45f;p.osc2=scenario==0?0:.35f;p.master=-12;
  if(scenario!=0){for(auto& h:p.hybrid){h.mode=5;h.unison=16;h.detune=25;h.width=.85f;}p.hybrid[1].mode=2;}
  if(scenario==2){p.mod.routes[0]={1,1,.2f,1};p.mod.routes[1]={2,10,.3f,1};p.effects[0]={2,true,.35f,.25f};p.effects[1]={4,true,.5f,.25f};}
  eg::SpectralEngine::Params sp;sp.mode=scenario==2?eg::SpectralEngine::Blur:eg::SpectralEngine::Off;
  for(int i=0;i<16;++i)e->noteOn(42+i,.8f);
  std::vector<float> l(buffer),r(buffer);int blocks=int(std::ceil(seconds*rate/buffer));std::vector<double> times;times.reserve(blocks);double sum=0,energy=0;uint64_t clips=0;float peak=0;int overruns=0;double budget=buffer/rate;
  for(int k=0;k<blocks;++k){auto start=Clock::now();e->render(l.data(),r.data(),buffer,p);if(scenario==2)spectral->process(l.data(),r.data(),buffer,sp);double d=std::chrono::duration<double>(Clock::now()-start).count()/budget*100;times.push_back(d);sum+=d;overruns+=d>100;
   for(int i=0;i<buffer;++i)for(float x:{l[i],r[i]}){check(std::isfinite(x)&&std::abs(x)<=1,"invalid stress output");peak=std::max(peak,std::abs(x));energy+=x*x;clips+=std::abs(x)>=.9999f;}
  }
  check(energy>1e-6,"silent stress output");std::sort(times.begin(),times.end());auto pct=[&](double q){return times[size_t((times.size()-1)*q)];};report<<name<<","<<rate<<","<<buffer<<",16,"<<seconds<<","<<sum/blocks<<","<<pct(.95)<<","<<pct(.99)<<","<<times.back()<<","<<overruns<<","<<blocks<<","<<peak<<","<<std::sqrt(energy/(2.*blocks*buffer))<<","<<clips<<std::endl;std::cout<<name<<" "<<rate<<" / "<<buffer<<": mean "<<sum/blocks<<"%, p99 "<<pct(.99)<<"%, overruns "<<overruns<<"/"<<blocks<<std::endl;
 };
 bench("four_granular_layers",0,48000,128,60);bench("two_16_unison",1,48000,128,60);bench("combined_mod_fx_spectral",2,48000,128,60);
 for(double sr:{44100.,96000.,192000.})for(int buffer:{64,256,512})bench("combined_short",2,sr,buffer,1);
 // Coherent sine-grid residual: measures out-of-expected-bin energy, not a complete alias characterization.
 std::ofstream alias("quality-reports/aliasing.csv");alias<<"sample_rate,mode,fundamental_hz,residual_db,allowed_harmonics\n";
 const auto& tables=eg::HybridTables::instance();constexpr int N=1024;
 for(double sr:{44100.,48000.,96000.,192000.})for(int mode:{1,2,3,4,5,9})for(int bin:{37,83,173}){
  eg::HybridVoice v;eg::HybridConfig c;eg::HybridParams p;p.mode=mode;c.set(p);float freq=float(sr*bin/N);std::array<double,N> x{};double total=0;
  for(int i=0;i<N;++i){x[i]=v.process(tables,c,freq,freq,float(sr)).l;total+=x[i]*x[i];}
  int allowed=1<<eg::HybridTables::band(freq/float(sr));double harmonics=0;
  for(int h=1;h<=allowed;++h){double re=0,im=0;for(int i=0;i<N;++i){double phase=2*3.141592653589793*bin*h*i/N;re+=x[i]*std::cos(phase);im+=x[i]*std::sin(phase);}harmonics+=2*(re*re+im*im)/N;}
  double db=10*std::log10(std::max(1e-16,(total-harmonics)/std::max(1e-16,total)));alias<<sr<<","<<mode<<","<<freq<<","<<db<<","<<allowed<<"\n";
 }
 auto sample=[](){auto s=std::make_unique<eg::SampleData>();s->left.assign(48000,.25f);s->right=s->left;s->path="qc-identical.wav";return s;};
 check(e->samples.publish(0,sample()),"sample publish");auto bytes=e->samples.retainedBytes();for(int i=0;i<100;++i)check(e->samples.publish(i%4,sample()),"sample reuse");check(e->samples.retainedBytes()==bytes,"identical sample memory");
 auto wt=[](){auto p=std::make_unique<eg::UserWavetable>();p->data.resize(1);return p;};check(e->userTables.publish(0,wt()),"table publish");bytes=e->userTables.retainedBytes();for(int i=0;i<100;++i)check(e->userTables.publish(i%2,wt()),"table reuse");check(e->userTables.retainedBytes()==bytes,"identical table memory");
 std::cout<<"Stress stability and identical sample/wavetable reuse PASS. CPU and residual thresholds require release review.\n";
}
