#include "../Source/SpectralEngine.h"
#include <memory>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
double energy(const std::vector<float>& a){double e=0;for(float x:a){require(std::isfinite(x)&&std::abs(x)<=1,"invalid output");e+=x*x;}return e;}
double tone(const std::vector<float>& a,double sr,double hz){double re=0,im=0;for(size_t i=0;i<a.size();++i){double ph=6.283185307179586*hz*i/sr;re+=a[i]*std::cos(ph);im+=a[i]*std::sin(ph);}return re*re+im*im;}
int main(){
 using E=eg::SpectralEngine;auto e=std::make_unique<E>();E::Params p;constexpr int n=8192;std::vector<float> l(n),r(n),orig(n);
 for(double sr:{44100.,48000.,96000.,192000.}){
  auto fill=[&](double hz,double amp=.35){for(int i=0;i<n;++i)l[i]=r[i]=float(amp*std::sin(6.283185307179586*hz*i/sr));};
  auto silence=[&]{std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);};
  double low=sr*10/1024,high=sr*31/1024;
  e->prepare(sr);p={};fill(low);orig=l;e->process(l.data(),r.data(),n,p);require(l==orig&&r==orig,"off is not transparent");
  p.mode=E::Freeze;p.mix=1;fill(low);e->process(l.data(),r.data(),n,p);require(e->captured.load()&4,"freeze capture");silence();e->process(l.data(),r.data(),n,p);require(energy(l)>.01,"freeze not sustained");require(tone(l,sr,low)>tone(l,sr,high)*5,"freeze pitch");
  e->prepare(sr);p={};p.mode=E::Blur;p.mix=1;p.blur=.9f;fill(low);orig=l;e->process(l.data(),r.data(),n,p);double diff=0;for(int i=2048;i<n;++i)diff+=std::abs(l[i]-orig[i]);require(diff>1&&energy(l)>.0001,"blur must affect audio");
  e->prepare(sr);p={};p.mode=E::Stretch;p.mix=1;p.stretch=4;fill(low);e->process(l.data(),r.data(),n,p);fill(high);e->process(l.data(),r.data(),n,p);require(energy(l)>.001,"stretch output");
  e->prepare(sr);p={};p.mode=E::Morph;p.mix=1;fill(low);e->process(l.data(),r.data(),n,p);e->request(E::CaptureA);fill(low);e->process(l.data(),r.data(),n,p);fill(high);e->process(l.data(),r.data(),n,p);e->request(E::CaptureB);fill(high);e->process(l.data(),r.data(),n,p);require((e->captured.load()&3)==3,"morph captures");p.morph=0;for(int i=0;i<3;++i){silence();e->process(l.data(),r.data(),n,p);}require(energy(l)>.001,"morph A");double a1=tone(l,sr,low),a2=tone(l,sr,high);p.morph=1;for(int i=0;i<3;++i){silence();e->process(l.data(),r.data(),n,p);}require(energy(l)>.001,"morph B");double b1=tone(l,sr,low),b2=tone(l,sr,high);require(a1>a2*2&&b2>b1*2,"morph endpoints");
  std::cout<<sr<<" Hz: bypass, freeze, blur, stretch, morph PASS\n";
 }
}
