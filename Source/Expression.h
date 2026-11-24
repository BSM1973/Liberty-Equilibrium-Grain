#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace eg {
struct ExpressionState {
 std::array<float,16> bend{},pressure{},timbre{};
 std::array<bool,16> sustain{};
 std::array<std::array<float,128>,16> poly{};
 void reset(){*this={};}
 static int ch(int channel){return std::clamp(channel-1,0,15);}
 void setBend(int channel,int value){bend[ch(channel)]=std::clamp((value-8192)/float(value>=8192?8191:8192),-1.f,1.f);}
 float pitch(int channel,bool mpe,float normal,float member)const{int c=ch(channel);return mpe&&c>0?bend[0]*normal+bend[c]*member:bend[c]*normal;}
 float touch(int channel,int note)const{int c=ch(channel);return std::max(pressure[c],poly[c][std::clamp(note,0,127)]);}
};
inline float morphValue(float a,float b,float t,bool discrete=false){t=std::clamp(t,0.f,1.f);return discrete?(t<.5f?a:b):a+(b-a)*t;}
inline float macroValue(float base,float macro,float depth){return std::clamp(base+(macro-.5f)*2*depth,0.f,1.f);}
}
