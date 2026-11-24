#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <algorithm>
#include <cstdint>
namespace eg {
// Fixed-hop stereo STFT. Analysis and synthesis are preallocated.
// Stretch is a bounded rolling spectral-history time effect, not offline file stretching.
class SpectralEngine {
public:
 static constexpr int N=1024,H=N/4,B=N/2+1,History=64,Display=64;
 struct Params {int mode=0;float mix=.65f,freeze=1.f,blur=.45f,stretch=2.f,morph=.5f;};
 enum {Off,Freeze,Blur,Stretch,Morph};
 enum {CaptureA=1,CaptureB=2,RecaptureFreeze=4,Clear=8};
 std::array<std::atomic<float>,Display> inputView{},outputView{};
 std::atomic<int> captured{0}; // bit 0 A, bit 1 B, bit 2 freeze
 void request(unsigned flags){requests.fetch_or(flags,std::memory_order_release);}
 void prepare(double sr){
  sampleRate=std::max(1.0,sr);head=hop=filled=historyCount=historyHead=0;readOffset=0;prevMode=Off;wet=0;pending=0;
  requests.store(0);captured.store(0);for(auto& v:inputView)v.store(0);for(auto& v:outputView)v.store(0);
  for(int i=0;i<N;++i)window[i]=.5f-.5f*std::cos(2*pi*i/N);
  for(int i=0;i<N/2;++i)twiddle[i]=std::polar(1.f,-2*pi*i/N);
  for(auto& c:channels)c={};
 }
 void process(float* l,float* r,int count,const Params& p){
  unsigned q=requests.exchange(0,std::memory_order_acq_rel);if(q&Clear){captured.store(0);pending=0;}pending|=q&7u;
  const int mode=std::clamp(p.mode,0,4);
  if(mode!=prevMode){if(mode==Freeze){captured.fetch_and(~4);pending|=RecaptureFreeze;}if(mode==Stretch)readOffset=0;for(auto& c:channels)c.phaseReady=false;prevMode=mode;}
  float target=mode==Off?0.f:std::clamp(p.mix,0.f,1.f);
  // Off with no fading or pending capture is bit-exact and avoids FFT work.
  if(mode==Off&&wet<1e-6f&&pending==0){wet=0;for(int i=0;i<count;++i){channels[0].input[head]=l[i];channels[1].input[head]=r[i];channels[0].ola[head]=channels[1].ola[head]=0;head=(head+1)%N;filled=std::min(N,filled+1);}hop=0;historyCount=0;for(auto& c:channels)c.analysisReady=c.phaseReady=false;return;}
  float smooth=1-std::exp(-1.f/float(sampleRate*.012));
  for(int i=0;i<count;++i){
   float x=l[i],y=r[i];float wl=channels[0].ola[head],wr=channels[1].ola[head];
   channels[0].ola[head]=channels[1].ola[head]=0;channels[0].input[head]=x;channels[1].input[head]=y;
   head=(head+1)%N;filled=std::min(N,filled+1);
   if(++hop>=H){hop=0;if(filled==N)frame(p,mode);}
   // Keep the direct path immediate. The spectral branch has N samples of creative-effect delay.
   float effectiveTarget=(mode==Freeze&&!(captured.load(std::memory_order_relaxed)&4))||
       (mode==Morph&&((captured.load(std::memory_order_relaxed)&3)!=3))?0.f:target;
   wet+=smooth*(effectiveTarget-wet);
   l[i]=std::clamp(x+(wl-x)*wet,-1.f,1.f);r[i]=std::clamp(y+(wr-y)*wet,-1.f,1.f);
  }
 }
private:
 static constexpr float pi=3.14159265358979323846f;
 using Complex=std::complex<float>;
 struct Frame {std::array<float,B> mag{},advance{};};
 struct Channel {
  std::array<float,N> input{},ola{};
  std::array<Complex,N> fft{};
  std::array<float,B> previousPhase{},synthPhase{},blurMemory{},livePhase{};
  Frame live{},freeze{},a{},b{};
  std::array<Frame,History> history{};
  bool analysisReady=false,phaseReady=false,blurReady=false;
 };
 std::array<Channel,2> channels{};
 std::array<float,N> window{};std::array<Complex,N/2> twiddle{};
 std::atomic<unsigned> requests{0};unsigned pending=0;
 int head=0,hop=0,filled=0,historyCount=0,historyHead=0,prevMode=Off;float readOffset=0,wet=0;double sampleRate=48000;
 static float principal(float x){return std::remainder(x,2*pi);}
 void fft(std::array<Complex,N>& a,bool inverse){
  for(int i=1,j=0;i<N;++i){int bit=N>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
  for(int len=2;len<=N;len*=2){int half=len/2,stride=N/len;for(int i=0;i<N;i+=len)for(int j=0;j<half;++j){Complex w=twiddle[j*stride];if(inverse)w=std::conj(w);auto u=a[i+j],v=a[i+j+half]*w;a[i+j]=u+v;a[i+j+half]=u-v;}}
  if(inverse)for(auto& x:a)x/=float(N);
 }
 void analyse(Channel& c){
  for(int i=0;i<N;++i)c.fft[i]={c.input[(head+i)%N]*window[i],0};
  fft(c.fft,false);
  for(int k=0;k<B;++k){float ph=std::arg(c.fft[k]),expected=2*pi*k*H/N;float delta=c.analysisReady?principal(ph-c.previousPhase[k]-expected):0;c.live.mag[k]=std::abs(c.fft[k]);c.live.advance[k]=expected+delta;c.livePhase[k]=ph;c.previousPhase[k]=ph;}
  c.analysisReady=true;c.history[historyHead]=c.live;
 }
 static float interpolate(float a,float b,float t){return a+(b-a)*t;}
 void frame(const Params& p,int mode){
  analyse(channels[0]);analyse(channels[1]);
  float energy=0;for(auto& c:channels)for(float m:c.live.mag)energy+=m*m;
  if(energy>1e-5f&&pending){int flags=captured.load();for(auto& c:channels){if(pending&CaptureA)c.a=c.live;if(pending&CaptureB)c.b=c.live;if(pending&RecaptureFreeze)c.freeze=c.live;}flags|=int(pending&7u);captured.store(flags);pending=0;}
  historyCount=std::min(History,historyCount+1);
  int i0=historyHead,i1=historyHead;float frac=0;
  if(mode==Stretch){
   const float factor=std::clamp(p.stretch,1.f,8.f);
   readOffset+=1.f-1.f/factor;
   // The read head moves more slowly than the incoming frames; re-anchor when the finite history is exhausted.
   if(readOffset>float(std::max(0,historyCount-2)))readOffset=0;
   int lag=int(readOffset);frac=readOffset-lag;i0=(historyHead+History-lag)%History;i1=(i0+History-1)%History;
  }else readOffset=0;
  int flags=captured.load(std::memory_order_relaxed);
  std::array<float,Display> before{},after{};
  for(auto& c:channels){
   for(int k=0;k<B;++k){
    float m=c.live.mag[k],inc=c.live.advance[k],dry=m;
    if(mode==Freeze&&(flags&4)){float t=std::clamp(p.freeze,0.f,1.f);m=interpolate(m,c.freeze.mag[k],t);inc=interpolate(inc,c.freeze.advance[k],t);}
    if(mode==Blur){float t=std::clamp(p.blur,0.f,1.f);int radius=1+int(t*15);float sum=0,weights=0;for(int j=-radius;j<=radius;++j){float w=float(radius+1-std::abs(j));sum+=w*c.live.mag[std::clamp(k+j,0,B-1)];weights+=w;}float spatial=interpolate(m,sum/weights,t);float keep=.94f*t;if(!c.blurReady)c.blurMemory[k]=spatial;c.blurMemory[k]=interpolate(spatial,c.blurMemory[k],keep);m=c.blurMemory[k];}
    if(mode==Stretch){auto& a=c.history[i0];auto& b=c.history[i1];m=interpolate(a.mag[k],b.mag[k],frac);inc=interpolate(a.advance[k],b.advance[k],frac);}
    if(mode==Morph&&(flags&3)==3){float t=std::clamp(p.morph,0.f,1.f);m=interpolate(c.a.mag[k],c.b.mag[k],t);inc=interpolate(c.a.advance[k],c.b.advance[k],t);}
    if(!c.phaseReady)c.synthPhase[k]=c.livePhase[k];else c.synthPhase[k]=principal(c.synthPhase[k]+inc);
    // Blur uses live phases to avoid excessive phase drift; other modes propagate phase.
    float phase=mode==Blur||mode==Off?c.livePhase[k]:c.synthPhase[k];
    if(k==0||k==N/2)c.fft[k]={m*std::cos(phase),0};else{c.fft[k]=std::polar(m,phase);c.fft[N-k]=std::conj(c.fft[k]);}
    int band=std::min(Display-1,k*Display/B);before[band]=std::max(before[band],dry);after[band]=std::max(after[band],m);
   }
   c.phaseReady=true;c.blurReady=mode==Blur;
   if(mode!=Off){fft(c.fft,true);for(int i=0;i<N;++i)c.ola[(head+i)%N]+=c.fft[i].real()*window[i]*(2.f/3.f);}
  }
  for(int k=0;k<Display;++k){inputView[k].store(before[k]/(N*.25f),std::memory_order_relaxed);outputView[k].store(after[k]/(N*.25f),std::memory_order_relaxed);}
  historyHead=(historyHead+1)%History;
 }
};
}
