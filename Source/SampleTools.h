#pragma once
#include "SampleStore.h"
#include <numeric>
#include <limits>
namespace eg {
struct SampleAnalysis {int root=-1;float confidence=0;std::vector<size_t> slices;};
inline SampleAnalysis analyseSample(const SampleData& s){
 SampleAnalysis a;a.slices.push_back(0);size_t n=s.frames();if(n<64)return a;
 // Downsample a high-energy 120 ms region to roughly 8 kHz for autocorrelation.
 size_t window=std::min(n,size_t(s.rate*.12)),best=0;double bestEnergy=0;
 for(size_t i=0;i+window<=n;i+=std::max<size_t>(1,window/2)){double e=0;for(size_t j=i;j<i+window;j+=8)e+=double(s.left[j])*s.left[j]+double(s.right[j])*s.right[j];if(e>bestEnergy){bestEnergy=e;best=i;}}
 int dec=std::max(1,int(s.rate/8000));double sr=s.rate/dec;std::vector<float> x;
 for(size_t i=best;i+dec<=best+window;i+=dec){float v=0;for(int j=0;j<dec;++j)v+=.5f*(s.left[i+j]+s.right[i+j]);x.push_back(v/dec);}
 if(x.size()>100){float mean=std::accumulate(x.begin(),x.end(),0.f)/x.size();for(auto& v:x)v-=mean;int lo=std::max(2,int(sr/1600)),hi=std::min(int(sr/40),int(x.size()/2));std::vector<float> corr(size_t(hi+2),0);
 for(int lag=lo;lag<=hi;++lag){double ab=0,aa=0,bb=0;for(size_t j=0;j+lag<x.size();++j){float u=x[j],v=x[j+lag];ab+=u*v;aa+=u*u;bb+=v*v;}corr[lag]=float(ab/std::sqrt(std::max(1e-20,aa*bb)));}
 float max=0;for(int k=lo;k<=hi;++k)max=std::max(max,corr[k]);for(int k=lo+1;k<hi;++k)if(corr[k]>=.75f&&corr[k]>=max*.90f&&corr[k]>=corr[k-1]&&corr[k]>corr[k+1]){float denom=corr[k-1]-2*corr[k]+corr[k+1];float off=std::abs(denom)>1e-6f?.5f*(corr[k-1]-corr[k+1])/denom:0;double hz=sr/(k+std::clamp(off,-.5f,.5f));a.root=std::clamp(int(std::lround(69+12*std::log2(hz/440))),0,127);a.confidence=corr[k];break;}}
 // Energy-rise transient detection with a refractory period.
 size_t hop=std::max<size_t>(1,size_t(s.rate*.005)),gap=size_t(s.rate*.045);float avg=0,prev=0;
 for(size_t i=0;i<n;i+=hop){float e=0;size_t end=std::min(n,i+hop);for(size_t j=i;j<end;++j)e+=s.left[j]*s.left[j]+s.right[j]*s.right[j];e=std::sqrt(e/(2*(end-i)));if(i>a.slices.back()+gap&&e>std::max(.008f,avg*2.5f)&&e>prev*1.7f&&a.slices.size()<64)a.slices.push_back(i);avg=.9f*avg+.1f*e;prev=e;}
 return a;
}
inline size_t nearestZero(const SampleData& s,size_t pos){
 if(s.frames()<2)return 0;pos=std::min(pos,s.frames()-1);size_t span=size_t(s.rate*.01),lo=pos>span?pos-span:0,hi=std::min(s.frames()-1,pos+span),best=pos;float cost=std::numeric_limits<float>::max();
 for(size_t k=std::max<size_t>(1,lo);k<=hi;++k){float x=s.left[k-1]+s.right[k-1],y=s.left[k]+s.right[k];if((x<=0&&y>=0)||(x>=0&&y<=0)){float c=std::abs(s.left[k])+std::abs(s.right[k])+.05f*float(k>pos?k-pos:pos-k)/std::max<size_t>(1,span);if(c<cost){cost=c;best=k;}}}return best;
}
inline float linearSample(const std::vector<float>& x,double p){p=std::clamp(p,0.,double(x.size()-1));size_t i=size_t(p),j=std::min(x.size()-1,i+1);return x[i]+float(p-i)*(x[j]-x[i]);}
// Offline WSOLA with stereo-coherent correlation search and overlap-add.
// Resample first for pitch, then compensate duration with WSOLA.
inline std::unique_ptr<SampleData> transformSample(const SampleData& in,size_t begin,size_t end,float stretch,float semitones){
 if(begin>=end||end>in.frames()||end-begin<64)return {};
 stretch=std::clamp(stretch,.5f,2.f);semitones=std::clamp(semitones,-12.f,12.f);double pitch=std::exp2(semitones/12.0);
 size_t count=end-begin,outN=size_t(std::round(count*stretch));if(outN<64||outN>size_t(in.rate*30))return {};
 size_t n=std::max<size_t>(64,size_t(std::round(count/pitch)));std::vector<float> l(n),r(n);
 // Windowed-sinc resampling; low-pass cutoff tracks downsampling ratio.
 double cutoff=.92/std::max(1.,pitch);
 for(size_t i=0;i<n;++i){double p=begin+i*pitch;long center=long(p);double sum=0,a=0,b=0;for(int k=-8;k<=8;++k){long j=center+k;double d=j-p,u=d*cutoff,win=.5+.5*std::cos(3.141592653589793*d/9.);double w=cutoff*(std::abs(u)<1e-9?1:std::sin(3.141592653589793*u)/(3.141592653589793*u))*win;size_t z=size_t(std::clamp(j,long(begin),long(end-1)));a+=w*in.left[z];b+=w*in.right[z];sum+=w;}l[i]=float(a/std::max(1e-8,sum));r[i]=float(b/std::max(1e-8,sum));}
 auto out=std::make_unique<SampleData>();out->rate=in.rate;out->left.resize(outN);out->right.resize(outN);
 size_t win=std::min({size_t(in.rate*.04),n,outN});size_t hop=std::max<size_t>(1,win/2),search=std::max<size_t>(1,size_t(in.rate*.008));double ratio=double(outN)/n;
 for(size_t j=0;j<win;++j){out->left[j]=l[j];out->right[j]=r[j];}
 for(size_t dst=hop;dst<outN;dst+=hop){size_t expected=std::min(n-win,size_t(dst/ratio)),lo=expected>search?expected-search:0,hi=std::min(n-win,expected+search),best=expected;double maxScore=-2;size_t overlap=std::min(hop,outN-dst);
 for(size_t pos=lo;pos<=hi;pos+=std::max<size_t>(1,search/48)){double ab=0,aa=0,bb=0;for(size_t j=0;j<overlap;j+=4){float x=out->left[dst+j]+out->right[dst+j],y=l[pos+j]+r[pos+j];ab+=x*y;aa+=x*x;bb+=y*y;}double score=ab/std::sqrt(std::max(1e-18,aa*bb));if(score>maxScore){maxScore=score;best=pos;}}
 for(size_t j=0;j<win&&dst+j<outN;++j){float f=j<hop?float(j)/hop:1;out->left[dst+j]=out->left[dst+j]*(1-f)+l[best+j]*f;out->right[dst+j]=out->right[dst+j]*(1-f)+r[best+j]*f;}}
 out->peak=0;for(size_t i=0;i<outN;++i)out->peak=std::max({out->peak,std::abs(out->left[i]),std::abs(out->right[i])});return out;
}
inline float loopSample(const std::vector<float>& x,float pos,float begin,float end,float crossfade,bool reverse){
 float v=linearSample(x,pos);float cf=std::clamp(crossfade,0.f,(end-begin)*.49f);if(cf<1)return v;
 if(!reverse&&pos>end-cf){float t=(pos-(end-cf))/cf;return v*(1-t)+linearSample(x,begin+(pos-(end-cf)))*t;}
 if(reverse&&pos<begin+cf){float t=(begin+cf-pos)/cf;return v*(1-t)+linearSample(x,end-(begin+cf-pos))*t;}return v;
}
}
