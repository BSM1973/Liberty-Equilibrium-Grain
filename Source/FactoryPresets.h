#pragma once
#include "GrainEngine.h"
namespace eg {
struct FactoryInfo {const char* name;int category;const char* tag;int variant;};
inline constexpr FactoryInfo factory[]={
 {"Init",0,"initialize",0},
 {"Golden Atmosphere",1,"warm evolving pad",0},
 {"Frozen Memories",1,"warm evolving pad",1},
 {"Infinite Horizon",1,"warm evolving pad",2},
 {"Celestial Motion",1,"warm evolving pad",3},
 {"Velvet Clouds",1,"warm evolving pad",4},
 {"Champagne Nebula",1,"warm evolving pad",5},
 {"Satin Aurora",1,"warm evolving pad",6},
 {"Distant Halo",1,"warm evolving pad",7},
 {"Amber Rain",1,"warm evolving pad",8},
 {"Moonlit Veil",1,"warm evolving pad",9},
 {"Quiet Gravity",1,"warm evolving pad",10},
 {"Silver Drift",1,"warm evolving pad",11},
 {"Soft Eclipse",1,"warm evolving pad",12},
 {"Eternal Bloom",1,"warm evolving pad",13},
 {"Luminous Tide",1,"warm evolving pad",14},
 {"Cinematic Universe",2,"cinematic evolving texture",0},
 {"Metallic Dreams",2,"cinematic evolving texture",1},
 {"Broken Dimensions",2,"cinematic evolving texture",2},
 {"Liquid Time",2,"cinematic evolving texture",3},
 {"Equilibrium Motion",2,"cinematic evolving texture",4},
 {"Obsidian Skies",2,"cinematic evolving texture",5},
 {"Crystalline Dusk",2,"cinematic evolving texture",6},
 {"Silent Monument",2,"cinematic evolving texture",7},
 {"Falling Stars",2,"cinematic evolving texture",8},
 {"Ghost Cathedral",2,"cinematic evolving texture",9},
 {"Dark Matter",2,"cinematic evolving texture",10},
 {"Shifting Horizons",2,"cinematic evolving texture",11},
 {"Ancient Signal",2,"cinematic evolving texture",12},
 {"Mirage Engine",2,"cinematic evolving texture",13},
 {"Last Light",2,"cinematic evolving texture",14},
 {"Solar Particles",3,"expressive granular lead",0},
 {"Prism Runner",3,"expressive granular lead",1},
 {"Glass Arrow",3,"expressive granular lead",2},
 {"Neon Thread",3,"expressive granular lead",3},
 {"Particle Solo",3,"expressive granular lead",4},
 {"Silken Pulse",3,"expressive granular lead",5},
 {"Copper Beam",3,"expressive granular lead",6},
 {"Floating Signal",3,"expressive granular lead",7},
 {"Dust Runner",3,"expressive granular lead",8},
 {"Lucid Edge",3,"expressive granular lead",9},
 {"Quantum Dust",4,"deep experimental bass",0},
 {"Sub Fracture",4,"deep experimental bass",1},
 {"Gravity Well",4,"deep experimental bass",2},
 {"Iron Roots",4,"deep experimental bass",3},
 {"Deep Current",4,"deep experimental bass",4},
 {"Broken Sub",4,"deep experimental bass",5},
 {"Carbon Pulse",4,"deep experimental bass",6},
 {"Dark Resonance",4,"deep experimental bass",7},
 {"Abyss Driver",4,"deep experimental bass",8},
 {"Magnetic Core",4,"deep experimental bass",9},
 {"Endless Motion",5,"tempo-shaped rhythmic grain",0},
 {"Fractured Reality",5,"tempo-shaped rhythmic grain",1},
 {"Clockwork Dust",5,"tempo-shaped rhythmic grain",2},
 {"Golden Steps",5,"tempo-shaped rhythmic grain",3},
 {"Rhythmic Orbit",5,"tempo-shaped rhythmic grain",4},
 {"Pulse Garden",5,"tempo-shaped rhythmic grain",5},
 {"Crystal Meter",5,"tempo-shaped rhythmic grain",6},
 {"Time Fragments",5,"tempo-shaped rhythmic grain",7},
 {"Grain Machine",5,"tempo-shaped rhythmic grain",8},
 {"Angular Motion",5,"tempo-shaped rhythmic grain",9},
 {"Ocean of Glass",6,"slow ambient landscape",0},
 {"Quiet Orbit",6,"slow ambient landscape",1},
 {"Morning Haze",6,"slow ambient landscape",2},
 {"Drifting Islands",6,"slow ambient landscape",3},
 {"Polar Night",6,"slow ambient landscape",4},
 {"Slow Breathing",6,"slow ambient landscape",5},
 {"Infinite Mist",6,"slow ambient landscape",6},
 {"Blue Silence",6,"slow ambient landscape",7},
 {"Distant Shore",6,"slow ambient landscape",8},
 {"Still Water",6,"slow ambient landscape",9},
 {"Wooden Resonance",7,"synthetic organic timbre",0},
 {"Silk Strings",7,"synthetic organic timbre",1},
 {"Glass Keys",7,"synthetic organic timbre",2},
 {"Amber Mallets",7,"synthetic organic timbre",3},
 {"Dust Piano",7,"synthetic organic timbre",4},
 {"Metal Harp",7,"synthetic organic timbre",5},
 {"Crystal Bells",7,"synthetic organic timbre",6},
 {"Soft Pluck",7,"synthetic organic timbre",7},
 {"Organic Reed",7,"synthetic organic timbre",8},
 {"Velvet Bow",7,"synthetic organic timbre",9},
 {"Digital Collapse",8,"fragmented experimental sound",0},
 {"Pixel Storm",8,"fragmented experimental sound",1},
 {"Broken Circuit",8,"fragmented experimental sound",2},
 {"Shattered Signal",8,"fragmented experimental sound",3},
 {"Temporal Error",8,"fragmented experimental sound",4},
 {"Scatter Field",8,"fragmented experimental sound",5},
 {"Glitch Bloom",8,"fragmented experimental sound",6},
 {"Particle Fault",8,"fragmented experimental sound",7},
 {"Signal Debris",8,"fragmented experimental sound",8},
 {"Fractal Noise",8,"fragmented experimental sound",9},
 {"Ethereal Voices",9,"synthetic vowel-like texture",0},
 {"Vowel Mist",9,"synthetic vowel-like texture",1},
 {"Choral Echo",9,"synthetic vowel-like texture",2},
 {"Breath of Light",9,"synthetic vowel-like texture",3},
 {"Ghost Vowels",9,"synthetic vowel-like texture",4},
 {"Solar Flare",10,"riser or transition texture",0},
 {"Reverse Horizon",10,"riser or transition texture",1},
 {"Rising Dust",10,"riser or transition texture",2},
 {"Dimension Gate",10,"riser or transition texture",3},
 {"Final Transition",10,"riser or transition texture",4},
};
constexpr int factoryCount=int(sizeof(factory)/sizeof(factory[0]));
static_assert(factoryCount==101,"100 presets plus Init");
inline Settings factorySettings(int index){
 Settings p;for(int j=1;j<4;++j)p.layers[j].enabled=false;
 if(index<=0||index>=factoryCount)return p;
 auto info=factory[index];int c=info.category,v=info.variant;
 const float k=float(v)/float(c<=2?14:c>=9?4:9);
 auto& a=p.layers[0];auto& b=p.layers[1];auto& d=p.layers[2];
 a.source=(v+c*2)%8;a.position=.13f+.69f*k;a.scatter=.03f+.12f*k;a.spread=.3f+.5f*k;
 a.drift=.1f+.6f*k;a.motion=.02f+.14f*k;a.level=.64f;
 b=a;b.enabled=true;b.source=(a.source+3+v%3)%8;b.position=1-a.position;b.level=.25f;b.pan=.2f;b.pitch=12;b.scatter=.16f;
 a.pan=-.1f;p.osc1=.12f;p.osc2=.05f;p.master=-10.f;p.release=.8f;
 auto route=[&](int n,int source,int dest,float depth,float curve=1){p.mod.routes[n]={source,dest,depth,curve};};
 auto fx=[&](int n,int type,float amount,float mix){p.effects[n]={type,true,amount,mix};};
 p.mod.lfos[0].rate=.09f+.17f*k;p.mod.lfos[1].rate=.15f+.5f*k;
 route(0,1,1,.24f+.18f*k);route(1,2,14,-.20f);
 switch(c){
 case 1:a.size=290+530*k;a.density=28+23*k;b.size=a.size*.78f;b.density=a.density*.65f;a.cutoff=3000+6500*k;b.cutoff=4500;p.attack=.25f+1.4f*k;p.release=2.5f+2*k;fx(0,2,.35f,.22f);fx(1,4,.64f+.25f*k,.35f);break;
 case 2:a.size=110+1100*k;a.density=14+45*k;b.size=400+400*k;b.density=22;b.pitch=v%2?-12:12;d=b;d.enabled=true;d.level=.18f;d.pitch=0;d.source=(a.source+5)%8;p.attack=.15f+1.7f*k;p.release=3.2f;route(2,3,27,.32f);p.mod.lfos[2].rate=.04f+.12f*k;fx(0,3,.27f+.3f*k,.28f);fx(1,4,.8f,.42f);break;
 case 3:a.size=40+180*k;a.density=37+45*k;b.size=90;b.density=24;b.pitch=0;a.filterMode=1;a.cutoff=2200+9700*k;a.resonance=.8f+2*k;p.osc1=.32f;p.osc2=.2f;p.fm=.1f+1.6f*k;p.attack=.005f+.025f*k;p.release=.25f+.6f*k;route(2,25,10,.2f);fx(0,3,.18f+.25f*k,.18f);fx(1,2,.25f,.16f);break;
 case 4:a.pitch=-12;a.size=80+240*k;a.density=27+38*k;b.pitch=-24;b.level=.24f;b.size=150;b.density=22;a.filterMode=1;a.cutoff=350+3600*k;a.resonance=1+2*k;p.oscPitch1=-12;p.osc1=.55f;p.oscPitch2=-24;p.osc2=.18f;p.fm=.25f+.8f*k;p.attack=.008f;p.release=.18f+.3f*k;p.drive=.18f+.45f*k;fx(0,1,.32f+.4f*k,.35f);fx(1,6,.45f,.5f);break;
 case 5:a.size=32+110*k;a.density=32+60*k;b.size=60+90*k;b.density=36;b.pitch=v%2?7:0;p.attack=.008f;p.release=.35f;p.mod.steps[0].length=16;p.mod.steps[0].rate=4+4*(v%3);p.mod.steps[0].sync=1;for(int i=0;i<16;++i)p.mod.steps[0].values[i]=(i%(2+v%3)==0)?1.f:-.7f;route(2,17,9,.45f);route(3,17,3,.35f);fx(0,3,.16f+.21f*k,.25f);break;
 case 6:a.size=650+900*k;a.density=10+18*k;b.size=900;b.density=13;b.pitch=v%2?7:12;a.motion=.12f+.2f*k;p.attack=.8f+1.5f*k;p.release=3.2f+1.5f*k;fx(0,4,.82f,.55f);fx(1,11,.7f,.55f);break;
 case 7:a.size=48+270*k;a.density=18+48*k;b.pitch=v%3==0?12:0;b.size=75+160*k;b.density=20;a.filterMode=1;a.cutoff=2200+10500*k;p.osc1=.23f+.12f*k;p.fm=v%3==0?1.7f:.2f;p.attack=.008f+.2f*k;p.release=.3f+1.1f*k;fx(0,4,.35f,.19f);route(2,25,9,.25f);break;
 case 8:a.size=8+130*k;a.density=55+88*k;a.scatter=.22f+.45f*k;a.direction=2;a.drift=2+9*k;b.direction=1;b.pitch=-12+float(v%4)*7;b.size=35+70*k;b.density=75;p.attack=.015f;p.release=.55f;p.mod.lfos[0].shape=4;p.mod.lfos[0].rate=4+8*k;route(2,1,4,.45f);fx(0,9,.35f+.45f*k,.42f);fx(1,8,.5f,.25f);break;
 case 9:a.size=170+500*k;a.density=25+32*k;b.size=220+350*k;b.density=24;b.pitch=7;a.filterMode=3;a.cutoff=650+950*k;a.resonance=2.3f;b.filterMode=3;b.cutoff=1500+950*k;b.resonance=2;p.attack=.15f+.5f*k;p.release=1.5f;route(2,2,10,.28f);fx(0,2,.4f,.2f);fx(1,4,.7f,.38f);break;
 case 10:a.size=120+850*k;a.density=12+50*k;a.direction=v%2?1:0;a.pitch=-12+24*k;b.pitch=12;b.size=200+500*k;b.density=15;p.attack=.2f+2.2f*k;p.release=2.7f;p.mod.functions[0].rate=.25f+.35f*k;p.mod.functions[0].loop=0;route(2,21,4,.65f);route(3,21,10,.42f);fx(0,3,.42f,.28f);fx(1,4,.75f,.38f);break;
 }
 p.macros[0]=.42f+.16f*k;p.macros[1]=.45f+.1f*k;p.macros[2]=.48f+.15f*k;p.macros[3]=.5f+.1f*k;p.macros[4]=.45f+.1f*k;p.macros[5]=.48f+.08f*k;p.macros[6]=.5f;p.macros[7]=.45f+.1f*k;
 return p;
}
}
