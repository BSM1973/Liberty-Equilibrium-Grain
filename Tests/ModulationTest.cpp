#include "../Source/GrainEngine.h"
#include <stdexcept>
#include <iostream>
#include <memory>
#include <vector>
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){for(double sr:{44100.,48000.,96000.,192000.}){
 eg::ModSettings m;eg::Sources s{};eg::Dest d{};eg::ModClock clk;eg::VoiceMod voice;
 m.steps[0].values[0]=.75f;m.steps[0].rate=.05f;
 clk.advance(m,32.f/float(sr),s);voice.advance(m,true,.8f,60,.1f,s);
 check(s[17]>.7f,"step");check(s[9]>.9f,"envelope attack");check(s[25]>.79f,"velocity");
 for(int i=0;i<64;++i)m.routes[i]={25,(i%eg::destN)+1,.5f,1};
 eg::routes(m,s,d);for(int i=1;i<=64;++i)check(std::abs(d[i]-.4f)<.001f,"all routes");
 m.routes[0]={25,1,-1,2};eg::routes(m,s,d);check(std::abs(d[1]+.64f)<.001f,"bipolar curve");
 auto a=std::make_unique<eg::Engine>(),b=std::make_unique<eg::Engine>();a->prepare(sr);b->prepare(sr);eg::Settings p,q;for(int j=1;j<4;++j)p.layers[j].enabled=false;p.osc1=p.osc2=0;q=p;q.mod.routes[63]={25,9,-1,1};
 a->noteOn(60,.9f);b->noteOn(60,.9f);std::vector<float> x(8192),y(8192),u(8192),v(8192);a->render(x.data(),y.data(),8192,p);b->render(u.data(),v.data(),8192,q);double e=0,f=0;for(int i=0;i<8192;++i){check(std::isfinite(u[i]),"finite");e+=x[i]*x[i];f+=u[i]*u[i];}check(e>.001&&f<e*.1,"route 64 actually changes audio");
 std::cout<<sr<<" Hz: modulation PASS\n";
 }}
