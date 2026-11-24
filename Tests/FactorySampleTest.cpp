#include "../Source/FactoryPresets.h"
#include <iostream>
#include <set>
#include <memory>
#include <stdexcept>
#include <vector>
#include <cstring>
void require(bool x,const char* why){if(!x)throw std::runtime_error(why);}
int main(){
 std::set<std::string> names;std::set<uint64_t> hashes;std::array<int,11> counts{};
 auto e=std::make_unique<eg::Engine>();
 for(int i=1;i<eg::factoryCount;++i){
  auto p=eg::factorySettings(i);require(names.insert(eg::factory[i].name).second,"duplicate name");++counts[eg::factory[i].category];e->prepare(48000);e->noteOn(60,.85f);std::vector<float> l(16384),r(16384);e->render(l.data(),r.data(),int(l.size()),p);
  uint64_t h=1469598103934665603ull;double energy=0;for(size_t k=0;k<l.size();++k){require(std::isfinite(l[k])&&std::isfinite(r[k])&&std::abs(l[k])<=1&&std::abs(r[k])<=1,"invalid output");energy+=l[k]*l[k]+r[k]*r[k];uint32_t bits;std::memcpy(&bits,&l[k],4);h=(h^bits)*1099511628211ull;}
  require(energy>1e-6,"silent preset");require(hashes.insert(h).second,"duplicate audio hash");}
 require(counts==std::array<int,11>{0,15,15,10,10,10,10,10,10,5,5},"category distribution");
 auto sample=std::make_unique<eg::SampleData>();sample->rate=48000;sample->left.assign(48000,0);sample->right.assign(48000,0);sample->peak=.8f;for(int i=0;i<48000;++i)sample->right[i]=.8f*std::sin(2*eg::pi*440*i/48000.f);require(e->samples.publish(0,std::move(sample)),"publish stereo sample");
 for(int rate:{44100,48000,96000,192000}){e->prepare(rate);auto p=eg::factorySettings(0);p.osc1=p.osc2=0;p.layers[0].sampleMode=true;p.layers[0].start=.1f;p.layers[0].end=.8f;p.layers[0].fadeIn=.1f;p.layers[0].fadeOut=.1f;p.layers[0].normalize=true;e->noteOn(60,1);std::vector<float> l(8192),r(8192);e->render(l.data(),r.data(),8192,p);double a=0,b=0;for(int i=0;i<8192;++i){a+=l[i]*l[i];b+=r[i]*r[i];require(std::isfinite(r[i]),"invalid sample");}require(a<1e-8&&b>.001,"stereo sample routing");}
 std::cout<<"100 distinct factory names/audio hashes and imported stereo playback PASS\n";
}
