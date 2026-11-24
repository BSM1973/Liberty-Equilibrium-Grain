#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "FilterEffects.h"
#include "Modulation.h"
#include "SampleStore.h"
#include "Expression.h"
namespace eg {
constexpr float pi=3.14159265358979323846f;
struct Layer {float level=.65f,pan=0,size=180,density=24,position=.35f,scatter=.05f,pitch=0,drift=0,spread=.4f,motion=.04f,cutoff=9000; float cutoff2=18000,resonance=.707f,resonance2=.707f; int filterMode=7,filterMode2=0,filterRouting=0; int source=0,direction=0,shape=0; bool enabled=true,sampleMode=false,normalize=false;float start=0,end=1,fadeIn=0,fadeOut=0;int root=60;};
struct Settings {std::array<Layer,4> layers{}; float osc1=.2f,osc2=0,oscPitch1=0,oscPitch2=0,fm=0,ring=0,attack=.02f,release=.9f,master=-8,velocity=.7f,lfoRate=.2f,lfoDepth=0,xyX=.5f,xyY=.5f,xyPos=0,xyDensity=0,drive=0,xyPitch=0,xyPan=0,xyTone=0,xySpread=0; ModSettings mod;std::array<EffectParams,12> effects{}; std::array<float,8> macros{.5f,.5f,.5f,.5f,.5f,.5f,.5f,.5f};};
class Engine {
public:
 static constexpr int sourceLength=32768, sourceCount=8, maxVoices=16, maxGrains=16;
 std::array<std::array<float,sourceLength>,sourceCount> sources{};SampleStore samples;
 Engine(){for(int s=0;s<sourceCount;++s)for(int i=0;i<sourceLength;++i){float t=float(i)/sourceLength, x=2*pi*t*64; float y=0; for(int k=1;k<=8;++k){float a=std::pow(float(k),-1.f-.22f*s); y+=a*std::sin(x*k+std::sin(2*pi*t*(s+1))*s*.16f*k); } sources[s][i]=std::tanh(y*.6f);}}
 void prepare(double r){rate=std::max(1.0,r);voices={}; clock=0; rack.prepare(rate);modClock={};modTick=0;globalSrc={};masterDest={};wheel=pressure=0;expression.reset();keys={};keyOrder={};keySerial=0;voiceMode=0;lastPitch=-1;}
 void setWheel(float x){wheel=std::clamp(x,0.f,1.f);}
 void setPressure(float x){pressure=std::clamp(x,0.f,1.f);}
 void configureExpression(int mode,bool mpe,float glideMs,float bendRange,float mpeRange,float pressureAmount,float timbreAmount){
  if(mode!=voiceMode){allOff();keys={};lastPitch=-1;voiceMode=mode;}
  mpeEnabled=mpe;glide=std::clamp(glideMs,0.f,2000.f);normalBend=bendRange;memberBend=mpeRange;touchAmount=pressureAmount;colourAmount=timbreAmount;
 }
 void pitchBend(int channel,int value){expression.setBend(channel,value);}
 void channelPressure(int channel,float v){expression.pressure[ExpressionState::ch(channel)]=std::clamp(v,0.f,1.f);if(channel==1)setPressure(v);}
 void polyPressure(int channel,int note,float v){expression.poly[ExpressionState::ch(channel)][std::clamp(note,0,127)]=std::clamp(v,0.f,1.f);}
 void timbre(int channel,float v){expression.timbre[ExpressionState::ch(channel)]=std::clamp(v,0.f,1.f);}
 void pedal(int channel,bool down){
  int c=ExpressionState::ch(channel);expression.sustain[c]=down;
  if(!down){for(auto& v:voices)if(v.active&&v.channel==c+1&&!keys[c][v.note])v.held=false;if(voiceMode)restoreMono();}
 }
 void noteOn(int n,float v,int channel=1){
  n=std::clamp(n,0,127);channel=std::clamp(channel,1,16);int c=channel-1;keys[c][n]=v;keyOrder[c][n]=++keySerial;expression.poly[c][n]=0;
  if(voiceMode){
   Voice* old=nullptr;for(auto& a:voices)if(a.active&&a.held){old=&a;break;}
   if(old&&voiceMode==2){old->note=n;old->channel=channel;old->velocity=v;old->held=true;lastPitch=n;return;}
   for(auto& a:voices)a.held=false;
  }
  startVoice(n,v,channel);
 }
 void noteOff(int n,int channel=1){n=std::clamp(n,0,127);int c=ExpressionState::ch(channel);keys[c][n]=0;
  if(!expression.sustain[c])for(auto& v:voices)if(v.active&&v.note==n&&v.channel==c+1)v.held=false;
  if(voiceMode)restoreMono();
 }
 void allOff(){for(auto& v:voices)v.held=false;keys={};expression.sustain={};}
 void panic(){voices={};keys={};expression.reset();lastPitch=-1;}
 int activeHeld()const{int n=0;for(auto& v:voices)if(v.active&&v.held)++n;return n;}
 int latestNote()const{for(auto& v:voices)if(v.active&&v.held)return v.note;return -1;}
 float latestPitch()const{for(auto& v:voices)if(v.active&&v.held)return v.currentPitch;return -1;}
 void render(float* left,float* right,int count,const Settings& p){
 bool matrix=false;for(auto& r:p.mod.routes)if(r.source>0&&r.dest>0&&r.depth!=0){matrix=true;break;}
 std::array<FilterCoeffs,4> f1{},f2{};
 for(int j=0;j<4;++j){const auto& c=p.layers[j];float tone=std::pow(4.f,p.macros[5]-.5f)*std::pow(8.f,p.xyTone*(p.xyY-.5f));f1[j]=FilterCoeffs::make(c.filterMode,c.cutoff*tone,c.resonance,rate);f2[j]=FilterCoeffs::make(c.filterMode2,c.cutoff2*tone,c.resonance2,rate);}
 for(int i=0;i<count;++i){bool tick=(modTick++%32)==0;float dt=32.f/float(rate);if(matrix&&tick){modClock.advance(p.mod,dt,globalSrc);globalSrc[27]=wheel;globalSrc[28]=pressure;globalSrc[29]=2*p.xyX-1;globalSrc[30]=2*p.xyY-1;routes(p.mod,globalSrc,masterDest);}float a=0,b=0;const float lfo=std::sin(clock*2*pi);clock+=p.lfoRate/rate;if(clock>=1)clock-=1;for(auto& v:voices){if(!v.active)continue;if(matrix&&(tick||!v.modReady)){auto ss=globalSrc;ss[28]=std::max(ss[28],expression.touch(v.channel,v.note));v.mod.advance(p.mod,v.held,v.velocity,v.note,dt,ss);routes(p.mod,ss,v.dest);v.modReady=true;for(int j=0;j<4;++j){auto& c=p.layers[j];int m=j*layerN;float tone=std::pow(4.f,p.macros[5]-.5f)*std::pow(8.f,p.xyTone*(p.xyY-.5f));v.mf1[j]=FilterCoeffs::make(c.filterMode,c.cutoff*tone*std::pow(16.f,v.dest[m+10]),c.resonance+5*v.dest[m+12],rate);v.mf2[j]=FilterCoeffs::make(c.filterMode2,c.cutoff2*tone*std::pow(16.f,v.dest[m+11]),c.resonance2+5*v.dest[m+13],rate);}}else if(!matrix)v.modReady=false;auto md=[&](int n){return matrix?v.dest[n]:0.f;};const float step=1.f/(float(rate)*std::max(.001f,v.held?p.attack*std::pow(4.f,md(57)):p.release*std::pow(4.f,md(58))));v.env=std::clamp(v.env+(v.held?step:-step),0.f,1.f);if(!v.held&&v.env<=0){v.active=false;continue;}float pitchTarget=float(v.note);float glideStep=glide>0?12.f/(float(rate)*glide*.001f):128.f;v.currentPitch+=std::clamp(pitchTarget-v.currentPitch,-glideStep,glideStep);float playedPitch=v.currentPitch+expression.pitch(v.channel,mpeEnabled,normalBend,memberBend);float pitchScale=std::exp2(playedPitch/12.f);float touch=expression.touch(v.channel,v.note);float x=0,y=0;for(int j=0;j<4;++j){auto c=p.layers[j];if(!c.enabled)continue;if(mpeEnabled)c.position=std::clamp(c.position+colourAmount*expression.timbre[ExpressionState::ch(v.channel)],0.f,1.f);c.pitch+=p.xyPitch*(p.xyX-.5f)*48;c.pan=std::clamp(c.pan+p.xyPan*(p.xyX-.5f)*2,-1.f,1.f);c.spread=std::clamp(c.spread+p.xySpread*(p.xyY-.5f)*2,0.f,1.f);int m=j*layerN;if(matrix){c.position=std::clamp(c.position+md(m+1)*.5f,0.f,1.f);c.size*=std::pow(4.f,md(m+2));c.density*=std::pow(4.f,md(m+3));c.pitch+=md(m+4)*24;c.scatter=std::clamp(c.scatter+md(m+5)*.5f,0.f,1.f);c.drift=std::clamp(c.drift+md(m+6)*12,0.f,24.f);c.spread=std::clamp(c.spread+md(m+7),0.f,1.f);c.pan=std::clamp(c.pan+md(m+8),-1.f,1.f);c.level=std::clamp(c.level+md(m+9),0.f,1.f);}auto& l=v.layers[j];const auto* imported=c.sampleMode?samples.get(j):nullptr;if(c.sampleMode&&!imported)continue;
 float density=std::clamp(c.density*(.5f+p.macros[1])+p.xyDensity*(p.xyY-.5f)*30,1.f,160.f);if(--l.countdown<=0){l.countdown=std::max(1,int(rate/density));for(auto& g:l.grains)if(!g.active){g.active=true;g.elapsed=0;g.duration=std::max(2,int(rate*.001f*std::clamp(c.size*(.5f+p.macros[1]),1.f,2000.f)));float pos=c.position+(p.macros[0]-.5f)+p.xyPos*(p.xyX-.5f)+c.scatter*random()+p.lfoDepth*lfo*.3f+c.motion*std::sin(l.scan*2*pi);g.pos=wrap(pos)*sourceLength;float dir=c.direction==1?-1.f:c.direction==2?(random()>0?1.f:-1.f):1.f;g.increment=dir*std::pow(2.f,(v.note-36+c.pitch+c.drift*random())/12.f)*(sourceLength/float(rate));g.pan=std::clamp(c.pan+c.spread*(.5f+p.macros[2])*random(),-1.f,1.f);g.source=std::clamp(c.source,0,sourceCount-1);g.shape=c.shape;g.refNote=v.note;g.refRatio=std::exp2(-float(v.note)/12.f);g.sample=imported;g.begin=0;g.end=float(sourceLength);g.sampleGain=1;g.fadeIn=g.fadeOut=0;if(imported){float n=float(imported->frames());g.begin=std::clamp(c.start*n,0.f,n-2);g.end=std::clamp(c.end*n,g.begin+2,n);g.pos=g.begin+wrap(pos)*(g.end-g.begin);g.increment=dir*float(imported->rate/rate)*std::pow(2.f,(v.note-c.root+c.pitch+c.drift*random())/12.f);g.sampleGain=c.normalize?std::min(16.f,1.f/std::max(.0001f,imported->peak)):1.f;g.fadeIn=std::clamp(c.fadeIn,0.f,.5f);g.fadeOut=std::clamp(c.fadeOut,0.f,.5f);}break;}}
 l.scan=wrap(l.scan+c.motion*(.5f+p.macros[3])/float(rate));float gl=0,gr=0;int live=0;for(auto& g:l.grains){if(!g.active)continue;float t=float(g.elapsed)/g.duration;float win=g.shape==1?1-std::abs(2*t-1):g.shape==2?std::exp(-18*(t-.5f)*(t-.5f))*std::sin(pi*t):.5f-.5f*std::cos(2*pi*t);float pos=g.pos;float sl=0,sr=0;
 if(g.sample){const auto& a=*g.sample;size_t k=std::min(a.frames()-1,size_t(std::max(0.f,pos)));size_t k2=std::min(a.frames()-1,k+1);if(float(k2)>=g.end)k2=std::min(a.frames()-1,size_t(g.begin));float f=pos-std::floor(pos);float t=(pos-g.begin)/(g.end-g.begin);float fade=1;if(g.fadeIn>0)fade=std::min(fade,t/g.fadeIn);if(g.fadeOut>0)fade=std::min(fade,(1-t)/g.fadeOut);float scale=win*g.sampleGain*std::clamp(fade,0.f,1.f);sl=(a.left[k]+f*(a.left[k2]-a.left[k]))*scale;sr=(a.right[k]+f*(a.right[k2]-a.right[k]))*scale;}
 else{int k=std::clamp(int(pos),0,sourceLength-1);float f=pos-k;auto& s=sources[g.source];sl=sr=(s[k]+f*(s[(k+1)%sourceLength]-s[k]))*win;}
 gl+=sl*std::sqrt(.5f*(1-g.pan));gr+=sr*std::sqrt(.5f*(1+g.pan));g.pos+=g.increment*((playedPitch==float(g.refNote))?1.f:pitchScale*g.refRatio);if(g.pos>=g.end||g.pos<g.begin){float span=g.end-g.begin;g.pos=g.begin+wrap((g.pos-g.begin)/span)*span;}if(++g.elapsed>=g.duration)g.active=false;++live;}
 float norm=c.level/std::sqrt(float(std::max(1,live)));float ll=gl*norm,rr=gr*norm;float l2=ll,r2=rr;
 if(c.filterMode!=0)l.filter1.process(ll,rr,matrix?v.mf1[j]:f1[j]);
 if(c.filterRouting==0){if(c.filterMode2!=0)l.filter2.process(ll,rr,matrix?v.mf2[j]:f2[j]);}
 else if(c.filterMode2!=0){l.filter2.process(l2,r2,matrix?v.mf2[j]:f2[j]);if(c.filterMode!=0){ll=.5f*(ll+l2);rr=.5f*(rr+r2);}else{ll=l2;rr=r2;}}
 x+=ll;y+=rr;}
 float f1=440*std::pow(2.f,(playedPitch-69+p.oscPitch1)/12.f),f2=440*std::pow(2.f,(playedPitch-69+p.oscPitch2)/12.f);v.phase1=wrap(v.phase1+std::min(f1/float(rate),.45f));v.phase2=wrap(v.phase2+std::min(f2/float(rate),.45f));float o2=std::sin(2*pi*v.phase2),o1=std::sin(2*pi*v.phase1+std::clamp(p.fm+md(55)*8,0.f,8.f)*o2);float osc=(std::clamp(p.osc1+md(53),0.f,1.f)*o1+std::clamp(p.osc2+md(54),0.f,1.f)*o2+std::clamp(p.ring+md(56),0.f,1.f)*o1*o2)*p.macros[4];float gain=v.env*((1-p.velocity)+p.velocity*v.velocity)*.22f*(1+touchAmount*touch);a+=(x+osc)*gain;b+=(y+osc)*gain;}
 float gain=std::pow(10.f,p.master/20.f), d=1+std::clamp(p.drive+(matrix?masterDest[59]:0.f),0.f,1.f)*4+p.macros[7];a=std::tanh(a*d);b=std::tanh(b*d);if(matrix){auto fx=p.effects;for(int k=0;k<12;++k){fx[k].amount=std::clamp(fx[k].amount+masterDest[60+2*k],0.f,1.f);fx[k].mix=std::clamp(fx[k].mix+masterDest[61+2*k],0.f,1.f);}rack.process(a,b,fx,p.macros[6]);}else rack.process(a,b,p.effects,p.macros[6]);left[i]=std::clamp(a*gain,-1.f,1.f);right[i]=std::clamp(b*gain,-1.f,1.f);}}
private:
 struct Grain{bool active=false;int elapsed=0,duration=0,source=0,shape=0,refNote=0;float refRatio=1;float pos=0,increment=0,pan=0,begin=0,end=float(sourceLength),sampleGain=1,fadeIn=0,fadeOut=0;const SampleData* sample=nullptr;};struct LayerVoice{std::array<Grain,maxGrains> grains{};int countdown=0;float scan=0;StereoFilter filter1,filter2;};struct Voice{bool active=false,held=false;int note=0,channel=1;float currentPitch=0,velocity=0,env=0,phase1=0,phase2=0;uint64_t age=0;std::array<LayerVoice,4> layers{};VoiceMod mod;Dest dest{};bool modReady=false;std::array<FilterCoeffs,4> mf1{},mf2{};};std::array<Voice,maxVoices> voices{};EffectsRack rack;ExpressionState expression;std::array<std::array<float,128>,16> keys{};std::array<std::array<uint64_t,128>,16> keyOrder{};uint64_t keySerial=0;int voiceMode=0,lastPitch=-1;bool mpeEnabled=false;float glide=0,normalBend=2,memberBend=48,touchAmount=0,colourAmount=0;ModClock modClock;Sources globalSrc{};Dest masterDest{};uint64_t modTick=0;float wheel=0,pressure=0;double rate=48000,clock=0;uint64_t age=0;uint32_t rng=123456789;
 void startVoice(int n,float v,int channel){Voice* p=nullptr;for(auto& a:voices)if(!a.active){p=&a;break;}if(!p)p=&*std::min_element(voices.begin(),voices.end(),[](auto& a,auto& b){return a.age<b.age;});*p={};p->active=p->held=true;p->note=n;p->channel=channel;p->currentPitch=glide>0&&lastPitch>=0?float(lastPitch):float(n);p->velocity=v;p->age=++age;lastPitch=n;}
 void restoreMono(){int n=-1,c=0;uint64_t newest=0;for(int i=0;i<16;++i)for(int j=0;j<128;++j)if(keys[i][j]>0&&keyOrder[i][j]>newest){newest=keyOrder[i][j];n=j;c=i;}if(n<0)return;for(auto& v:voices)if(v.active&&v.held&&v.note==n&&v.channel==c+1)return;
 if(voiceMode==2){Voice* old=nullptr;for(auto& v:voices)if(v.active&&(!old||v.age>old->age))old=&v;if(old){old->held=true;old->note=n;old->channel=c+1;old->velocity=keys[c][n];lastPitch=n;return;}}
 for(auto& v:voices)v.held=false;startVoice(n,keys[c][n],c+1);
 }
 float random(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return float(rng)/2147483648.f-1.f;}static float wrap(float x){return x-std::floor(x);}
};
}
