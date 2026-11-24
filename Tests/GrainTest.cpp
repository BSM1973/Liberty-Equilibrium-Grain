#include "../Source/GrainEngine.h"
#include <memory>
#include <iostream>
#include <vector>
#include <stdexcept>
int main(){auto e=std::make_unique<eg::Engine>();eg::Settings p; for(int j=1;j<4;++j)p.layers[j].enabled=false;for(int rate:{44100,48000,96000,192000}){e->prepare(rate);e->noteOn(60,.8);std::vector<float> l(8192),r(8192);e->render(l.data(),r.data(),int(l.size()),p);double energy=0;for(float x:l){if(!std::isfinite(x)||std::abs(x)>1)throw std::runtime_error("invalid output");energy+=x*x;}if(energy<.001)throw std::runtime_error("silent");e->noteOff(60);for(int i=0;i<30;++i)e->render(l.data(),r.data(),int(l.size()),p);for(float x:l)if(std::abs(x)>1e-5)throw std::runtime_error("release failed");std::cout<<rate<<" Hz: PASS\n";} }
