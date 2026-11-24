#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace eg {
constexpr int sourceN=30,destN=83,layerN=13;
constexpr float mp=3.14159265358979323846f;
struct Lfo {float rate=.2f,phase=0;int shape=0,sync=0;};
struct Env {float attack=.05f,decay=.3f,sustain=.7f,release=.6f;};
struct Steps {float rate=4,glide=0;int length=16,sync=0;std::array<float,64> values{};};
struct Function {float rate=.5f,curve=1;int shape=0,loop=1;};
struct Route {int source=0,dest=0;float depth=0,curve=1;};
struct ModSettings {std::array<Lfo,8> lfos{};std::array<Env,8> envs{};std::array<Steps,4> steps{};std::array<Function,4> functions{};std::array<Route,64> routes{};float bpm=120;};
using Sources=std::array<float,sourceN+1>;using Dest=std::array<float,destN+1>;
inline float wrapMod(float x){return x-std::floor(x);}
inline float waveMod(int shape,float p,float held=0){p=wrapMod(p);switch(shape){case 1:return 1-4*std::abs(p-.5f);case 2:return 2*p-1;case 3:return p<.5f?1.f:-1.f;case 4:return held;case 5:return std::sin(2*mp*p)*std::sin(6*mp*p);default:return std::sin(2*mp*p);}}
inline float functionWave(int shape,float p,float curve){float x=p;switch(shape){case 1:x=1-p;break;case 2:x=1-std::abs(2*p-1);break;case 3:x=.5f-.5f*std::cos(2*mp*p);break;}return std::pow(std::clamp(x,0.f,1.f),std::clamp(curve,.25f,4.f));}
inline float speed(float r,int sync,float bpm){return sync?r*std::clamp(bpm,20.f,400.f)/60:r;}
struct ModClock {
 std::array<float,8> phase{},held{};std::array<float,4> step{};uint32_t rng=0x285ab631u;
 float random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng)/2147483648.f-1.f;}
 void advance(const ModSettings& p,float dt,Sources& out){
  for(int i=0;i<8;++i){auto& q=p.lfos[i];float prev=phase[i];phase[i]=wrapMod(prev+speed(q.rate,q.sync,p.bpm)*dt);if(phase[i]<prev)held[i]=random();out[1+i]=waveMod(q.shape,phase[i]+q.phase,held[i]);}
  for(int i=0;i<4;++i){auto& q=p.steps[i];int len=std::clamp(q.length,1,64);float v=step[i]+speed(q.rate,q.sync,p.bpm)*dt;step[i]=v-std::floor(v/len)*len;int n=std::clamp(int(step[i]),0,len-1);float g=std::clamp(q.glide,0.f,1.f);float mix=g>0?std::min(1.f,(step[i]-n)/g):1.f;out[17+i]=q.values[(n+len-1)%len]*(1-mix)+q.values[n]*mix;}
 }
};
struct VoiceMod {
 std::array<float,8> env{};std::array<int,8> stage{};std::array<float,4> phase{};
 void advance(const ModSettings& p,bool held,float vel,int note,float dt,Sources& out){
  for(int i=0;i<8;++i){auto& q=p.envs[i];auto& e=env[i];auto& s=stage[i];if(!held)s=3;
   if(s==0){e+=dt/std::max(.001f,q.attack);if(e>=1){e=1;s=1;}}else if(s==1){e-=dt*(1-q.sustain)/std::max(.001f,q.decay);if(e<=q.sustain){e=q.sustain;s=2;}}else if(s==2)e=q.sustain;else e=std::max(0.f,e-dt/std::max(.001f,q.release));out[9+i]=std::clamp(e,0.f,1.f);}
  for(int i=0;i<4;++i){auto& q=p.functions[i];phase[i]+=q.rate*dt;phase[i]=q.loop?wrapMod(phase[i]):std::min(1.f,phase[i]);out[21+i]=functionWave(q.shape,phase[i],q.curve);}
  out[25]=vel;out[26]=note/127.f;
 }
};
inline void routes(const ModSettings& p,const Sources& src,Dest& dst){dst.fill(0);for(auto& r:p.routes){if(r.source<1||r.source>sourceN||r.dest<1||r.dest>destN||r.depth==0)continue;float x=src[r.source];dst[r.dest]+=r.depth*std::copysign(std::pow(std::min(1.f,std::abs(x)),std::clamp(r.curve,.25f,4.f)),x);}for(auto& x:dst)x=std::clamp(x,-1.f,1.f);}
}
