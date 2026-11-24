#pragma once
#include <array>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <cmath>
#include <algorithm>
namespace eg {
struct UserWavetable {
 static constexpr int N=1024,Tiers=9,MaxFrames=16;
 using Frame=std::array<std::array<float,N+1>,Tiers>;
 std::vector<Frame> data;
 static std::unique_ptr<UserWavetable> build(const std::vector<float>& input,int frameSize){
  if(frameSize<32||frameSize>8192||input.size()<size_t(frameSize)||input.size()%frameSize!=0||input.size()/frameSize>MaxFrames)return {};
  auto bank=std::make_unique<UserWavetable>();bank->data.resize(input.size()/frameSize);
  constexpr double pi=3.141592653589793;
  for(size_t frame=0;frame<bank->data.size();++frame){std::array<float,N> x{},acc{};double mean=0;for(int i=0;i<N;++i){double p=double(i)*frameSize/N;int k=int(p);float f=float(p-k);size_t a=frame*frameSize+k,b=frame*frameSize+(k+1)%frameSize;x[i]=input[a]+f*(input[b]-input[a]);if(!std::isfinite(x[i]))x[i]=0;mean+=x[i]/N;}for(auto& v:x)v-=float(mean);
   int tier=0;for(int h=1;h<=256;++h){double re=0,im=0;for(int i=0;i<N;++i){double a=2*pi*h*i/N;re+=x[i]*std::cos(a);im+=x[i]*std::sin(a);}re*=2./N;im*=2./N;for(int i=0;i<N;++i){double a=2*pi*h*i/N;acc[i]+=float(re*std::cos(a)+im*std::sin(a));}if(h==(1<<tier)){auto& t=bank->data[frame][tier];for(int i=0;i<N;++i)t[i]=acc[i];t[N]=t[0];if(tier<Tiers-1)++tier;}}
   float peak=0;for(float v:bank->data[frame][Tiers-1])peak=std::max(peak,std::abs(v));if(peak>1e-6f)for(auto& tierData:bank->data[frame])for(auto& v:tierData)v/=peak;
  }return bank;
 }
 float read(int tier,float phase,float shape)const{if(data.empty())return 0;float t=std::clamp(shape,0.f,1.f)*float(data.size()-1);size_t a=size_t(t),b=std::min(data.size()-1,a+1);float f=t-a;float p=(phase-std::floor(phase))*N;int k=std::min(N-1,int(p));float frac=p-k;auto get=[&](size_t i){auto& v=data[i][tier];return v[k]+frac*(v[k+1]-v[k]);};return get(a)*(1-f)+get(b)*f;}
};
class WavetableStore {
 std::array<std::atomic<const UserWavetable*>,2> current{};std::vector<std::unique_ptr<const UserWavetable>> kept;std::mutex mutex;size_t bytes=0;
public:
 size_t retainedBytes(){std::lock_guard<std::mutex> lock(mutex);return bytes;}
 const UserWavetable* get(int i)const{return current[i].load(std::memory_order_acquire);}
 void clear(int i){current[i].store(nullptr,std::memory_order_release);}
 bool publish(int i,std::unique_ptr<UserWavetable> p){if(!p)return false;std::lock_guard<std::mutex> lock(mutex);for(auto& old:kept)if(old->data==p->data){current[i].store(old.get(),std::memory_order_release);return true;}size_t n=p->data.size()*sizeof(UserWavetable::Frame);if(bytes+n>32u*1024u*1024u)return false;auto* raw=p.get();kept.push_back(std::move(p));bytes+=n;current[i].store(raw,std::memory_order_release);return true;}
};
}
