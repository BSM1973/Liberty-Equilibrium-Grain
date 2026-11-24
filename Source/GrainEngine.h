#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>
namespace eg {
constexpr float pi=3.14159265358979323846f;
struct Layer {float level=.65f,pan=0,size=180,density=24,position=.35f,scatter=.05f,pitch=0,drift=0,spread=.4f,motion=.04f,cutoff=9000; int source=0,direction=0,shape=0; bool enabled=true;};
struct Settings {std::array<Layer,4> layers{}; float osc1=.2f,osc2=0,oscPitch1=0,oscPitch2=0,fm=0,ring=0,attack=.02f,release=.9f,master=-8,velocity=.7f,lfoRate=.2f,lfoDepth=0,xyX=.5f,xyY=.5f,xyPos=0,xyDensity=0,drive=0; std::array<float,8> macros{.5f,.5f,.5f,.5f,.5f,.5f,.5f,.5f};};
class Engine {
public:
 static constexpr int sourceLength=32768, sourceCount=8, maxVoices=16, maxGrains=16;
 std::array<std::array<float,sourceLength>,sourceCount> sources{};
 Engine(){for(int s=0;s<sourceCount;++s)for(int i=0;i<sourceLength;++i){float t=float(i)/sourceLength, x=2*pi*t*64; float y=0; for(int k=1;k<=8;++k){float a=std::pow(float(k),-1.f-.22f*s); y+=a*std::sin(x*k+std::sin(2*pi*t*(s+1))*s*.16f*k); } sources[s][i]=std::tanh(y*.6f);}}
 void prepare(double r){rate=std::max(1.0,r);voices={}; clock=0;}
 void noteOn(int n,float v){Voice* p=nullptr;for(auto& a:voices)if(!a.active){p=&a;break;}if(!p)p=&*std::min_element(voices.begin(),voices.end(),[](auto& a,auto& b){return a.age<b.age;});*p={};p->active=p->held=true;p->note=n;p->velocity=v;p->age=++age;for(auto& l:p->layers)l.countdown=0;}
 void noteOff(int n){for(auto& v:voices)if(v.active&&v.note==n)v.held=false;}
 void allOff(){for(auto& v:voices)v.held=false;}
 void render(float* left,float* right,int count,const Settings& p){for(int i=0;i<count;++i){float a=0,b=0;const float lfo=std::sin(clock*2*pi);clock+=p.lfoRate/rate;if(clock>=1)clock-=1;for(auto& v:voices){if(!v.active)continue;const float step=1.f/(float(rate)*std::max(.001f,v.held?p.attack:p.release));v.env=std::clamp(v.env+(v.held?step:-step),0.f,1.f);if(!v.held&&v.env<=0){v.active=false;continue;}float x=0,y=0;for(int j=0;j<4;++j){const auto& c=p.layers[j];if(!c.enabled)continue;auto& l=v.layers[j];float density=std::clamp(c.density*(.5f+p.macros[1])+p.xyDensity*(p.xyY-.5f)*30,1.f,160.f);if(--l.countdown<=0){l.countdown=std::max(1,int(rate/density));for(auto& g:l.grains)if(!g.active){g.active=true;g.elapsed=0;g.duration=std::max(2,int(rate*.001f*std::clamp(c.size*(.5f+p.macros[1]),1.f,2000.f)));float pos=c.position+(p.macros[0]-.5f)+p.xyPos*(p.xyX-.5f)+c.scatter*random()+p.lfoDepth*lfo*.3f+c.motion*std::sin(l.scan*2*pi);g.pos=wrap(pos)*sourceLength;float dir=c.direction==1?-1.f:c.direction==2?(random()>0?1.f:-1.f):1.f;g.increment=dir*std::pow(2.f,(v.note-36+c.pitch+c.drift*random())/12.f)*(sourceLength/float(rate));g.pan=std::clamp(c.pan+c.spread*(.5f+p.macros[2])*random(),-1.f,1.f);g.source=std::clamp(c.source,0,sourceCount-1);g.shape=c.shape;break;}}
 l.scan=wrap(l.scan+c.motion*(.5f+p.macros[3])/float(rate));float gl=0,gr=0;int live=0;for(auto& g:l.grains){if(!g.active)continue;float t=float(g.elapsed)/g.duration;float win=g.shape==1?1-std::abs(2*t-1):g.shape==2?std::exp(-18*(t-.5f)*(t-.5f))*std::sin(pi*t):.5f-.5f*std::cos(2*pi*t);float pos=g.pos;int k=int(pos);float f=pos-k;auto& s=sources[g.source];float sample=(s[k]+f*(s[(k+1)%sourceLength]-s[k]))*win;gl+=sample*std::sqrt(.5f*(1-g.pan));gr+=sample*std::sqrt(.5f*(1+g.pan));g.pos+=g.increment;while(g.pos>=sourceLength)g.pos-=sourceLength;while(g.pos<0)g.pos+=sourceLength;if(++g.elapsed>=g.duration)g.active=false;++live;}
 float norm=c.level/std::sqrt(float(std::max(1,live)));float fc=std::clamp(c.cutoff*std::pow(4.f,p.macros[5]-.5f),20.f,float(rate*.45));float coeff=1-std::exp(-2*pi*fc/float(rate));l.fl+=coeff*(gl*norm-l.fl);l.fr+=coeff*(gr*norm-l.fr);x+=l.fl;y+=l.fr;}
 float f1=440*std::pow(2.f,(v.note-69+p.oscPitch1)/12.f),f2=440*std::pow(2.f,(v.note-69+p.oscPitch2)/12.f);v.phase1=wrap(v.phase1+std::min(f1/float(rate),.45f));v.phase2=wrap(v.phase2+std::min(f2/float(rate),.45f));float o2=std::sin(2*pi*v.phase2),o1=std::sin(2*pi*v.phase1+p.fm*o2);float osc=(p.osc1*o1+p.osc2*o2+p.ring*o1*o2)*p.macros[4];float gain=v.env*((1-p.velocity)+p.velocity*v.velocity)*.22f;a+=(x+osc)*gain;b+=(y+osc)*gain;}
 float gain=std::pow(10.f,p.master/20.f), d=1+p.drive*4+p.macros[7];left[i]=std::tanh(a*d)*gain;right[i]=std::tanh(b*d)*gain;}}
private:
 struct Grain{bool active=false;int elapsed=0,duration=0,source=0,shape=0;float pos=0,increment=0,pan=0;};struct LayerVoice{std::array<Grain,maxGrains> grains{};int countdown=0;float scan=0,fl=0,fr=0;};struct Voice{bool active=false,held=false;int note=0;float velocity=0,env=0,phase1=0,phase2=0;uint64_t age=0;std::array<LayerVoice,4> layers{};};std::array<Voice,maxVoices> voices{};double rate=48000,clock=0;uint64_t age=0;uint32_t rng=123456789;float random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng)/2147483648.f-1.f;}static float wrap(float x){return x-std::floor(x);}
};
}
