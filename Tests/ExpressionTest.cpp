#include "../Source/GrainEngine.h"
#include <memory>
#include <vector>
#include <stdexcept>
#include <iostream>
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){
 eg::ExpressionState x;x.setBend(2,16383);check(x.pitch(2,true,2,48)==48&&x.pitch(3,true,2,48)==0,"MPE member isolation");x.setBend(1,0);check(x.pitch(2,true,2,48)==46,"master bend");
 x.poly[1][60]=.8f;check(x.touch(2,60)==.8f&&x.touch(2,61)==0,"poly pressure isolation");
 check(eg::morphValue(.2f,.8f,.5f)>.49f&&eg::morphValue(0,1,.49f,true)==0,"morph");check(eg::macroValue(.5f,1,.5f)==1,"macro");
 for(double sr:{44100.,48000.,96000.,192000.}){
 auto e=std::make_unique<eg::Engine>();e->prepare(sr);e->configureExpression(0,true,0,2,48,0,0);e->noteOn(60,1,2);e->noteOn(60,1,3);check(e->activeHeld()==2,"same pitch different channel");e->pedal(2,true);e->noteOff(60,2);check(e->activeHeld()==2,"sustain");e->pedal(2,false);check(e->activeHeld()==1,"pedal release");e->noteOff(60,3);check(e->activeHeld()==0,"channel release");
 e->panic();e->configureExpression(2,false,100,2,48,0,0);e->noteOn(60,1);e->noteOn(72,1);check(e->activeHeld()==1&&e->latestNote()==72&&e->latestPitch()==60,"legato/glide");eg::Settings p;for(auto& l:p.layers)l.enabled=false;p.osc1=.5f;std::vector<float> l(int(sr*.05)),r(l.size());e->render(l.data(),r.data(),int(l.size()),p);check(e->latestPitch()>65&&e->latestPitch()<67,"glide midpoint");e->noteOff(72);check(e->latestNote()==60,"mono fallback");e->allOff();check(e->activeHeld()==0,"all notes off");std::cout<<sr<<" expression PASS\n";
 }
}
