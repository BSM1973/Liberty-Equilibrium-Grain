#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "GrainEngine.h"
using namespace juce;
namespace {
const Colour bg(0xff151719),panel(0xff25282b),gold(0xffc5a64a),champ(0xffd4bc76),ink(0xffe8e8e5),muted(0xff989c9f);
struct Spec {const char* id;const char* fr;const char* en;float lo,hi,def,step;};
const Spec layerSpecs[]={{"level","Niveau","Level",0,1,.65,.01},{"pan","Panoramique","Pan",-1,1,0,.01},{"size","Taille","Size",1,2000,180,1},{"density","Densité","Density",1,160,24,1},{"position","Position","Position",0,1,.35,.01},{"scatter","Dispersion","Scatter",0,1,.05,.01},{"pitch","Hauteur","Pitch",-36,36,0,1},{"drift","Dérive","Pitch drift",0,12,0,.01},{"spread","Largeur","Spread",0,1,.4,.01},{"motion","Mouvement","Movement",0,1,.04,.01},{"cutoff","Fréquence","Cutoff",20,20000,9000,1},{"cutoff2","Fréquence","Cutoff",20,20000,18000,1},{"resonance","Résonance","Resonance",.5,12,.707,.001},{"resonance2","Résonance","Resonance",.5,12,.707,.001},{"filterMode","Mode filtre 1","Filter 1 mode",0,7,7,1},{"filterMode2","Mode filtre 2","Filter 2 mode",0,7,0,1},{"filterRouting","Routage","Routing",0,1,0,1},{"source","Source","Source",0,7,0,1},{"direction","Direction","Direction",0,2,0,1},{"shape","Enveloppe","Envelope",0,2,0,1},{"enabled","Active","Enabled",0,1,1,1}};
const Spec globalSpecs[]={{"osc1","Niveau OSC 1","OSC 1 level",0,1,.2,.01},{"osc2","Niveau OSC 2","OSC 2 level",0,1,0,.01},{"oscPitch1","Hauteur OSC 1","OSC 1 pitch",-24,24,0,1},{"oscPitch2","Hauteur OSC 2","OSC 2 pitch",-24,24,0,1},{"fm","FM","FM",0,8,0,.01},{"ring","Anneau","Ring",0,1,0,.01},{"attack","Attaque","Attack",.001,3,.02,.001},{"release","Relâchement","Release",.01,5,.9,.01},{"master","Sortie master","Master output",-36,0,-8,.1},{"velocity","Vélocité","Velocity",0,1,.7,.01},{"lfoRate","Vitesse LFO","LFO rate",.01,20,.2,.01},{"lfoDepth","Profondeur LFO","LFO depth",0,1,0,.01},{"xyX","Position X","Position X",0,1,.5,.01},{"xyY","Densité Y","Density Y",0,1,.5,.01},{"xyPos","Quantité position","Position amount",0,1,0,.01},{"xyDensity","Quantité densité","Density amount",0,1,0,.01},{"drive","Saturation","Drive",0,1,0,.01}};
static String fxid(int n,const char* param){return "fx"+String(n+1)+"_"+param;}
const char* macroNames[]={"Position","Cloud","Spread","Motion","Hybrid","Tone","Space","Heat"};
String lid(int layer,const char* id){return String::charToString(juce_wchar('A'+layer))+"_"+id;}
AudioProcessorValueTreeState::ParameterLayout layout(){AudioProcessorValueTreeState::ParameterLayout r;auto add=[&](String id,String name,float lo,float hi,float def,float step){r.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>(lo,hi,step),def));};for(int l=0;l<4;++l)for(auto& s:layerSpecs)add(lid(l,s.id),lid(l,s.en),s.lo,s.hi, String(s.id)=="enabled"?(l==0?1.f:0.f):s.def,s.step);for(auto& s:globalSpecs)add(s.id,s.en,s.lo,s.hi,s.def,s.step);for(int i=0;i<8;++i)add("macro"+String(i),macroNames[i],0,1,.5,.01);for(int i=0;i<12;++i){add(fxid(i,"type"),fxid(i,"type"),0,12,0,1);add(fxid(i,"enabled"),fxid(i,"enabled"),0,1,1,1);add(fxid(i,"amount"),fxid(i,"amount"),0,1,.5,.01);add(fxid(i,"mix"),fxid(i,"mix"),0,1,.5,.01);}return r;}
class Processor;
class Metal : public LookAndFeel_V4 {public:Metal(){setColour(Slider::textBoxTextColourId,champ);setColour(Slider::textBoxBackgroundColourId,bg);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);setColour(TextButton::buttonColourId,panel);setColour(TextButton::buttonOnColourId,gold.darker(.7f));setColour(TextButton::textColourOffId,ink);setColour(TextButton::textColourOnId,champ);}
void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float start,float end,Slider&)override{float r=std::min(w,h)*.39f,cx=x+w*.5f,cy=y+h*.5f;for(int i=0;i<=28;++i){float a=start+(end-start)*i/28;g.setColour(i<=pos*28?champ:muted.withAlpha(.35f));g.drawLine(cx+std::sin(a)*r*1.12f,cy-std::cos(a)*r*1.12f,cx+std::sin(a)*r*1.24f,cy-std::cos(a)*r*1.24f,1); }Rectangle<float> b(cx-r,cy-r,2*r,2*r);g.setColour(Colours::black.withAlpha(.6f));g.fillEllipse(b.translated(1,3));ColourGradient grad(Colour(0xff78683d),b.getX(),b.getY(),Colour(0xffb6a064),b.getRight(),b.getBottom(),false);grad.addColour(.32,Colour(0xffd8c68a));grad.addColour(.52,Colour(0xff81703e));grad.addColour(.74,Colour(0xffc6b477));g.setGradientFill(grad);g.fillEllipse(b);g.setColour(champ.withAlpha(.65f));g.drawEllipse(b,1);float a=start+(end-start)*pos;g.setColour(ink);g.drawLine(cx+std::sin(a)*r*.55f,cy-std::cos(a)*r*.55f,cx+std::sin(a)*r*.9f,cy-std::cos(a)*r*.9f,2);}};
class Processor: public AudioProcessor {public: Processor():AudioProcessor(BusesProperties().withOutput("Stereo",AudioChannelSet::stereo(),true)),state(*this,nullptr,"EG",layout()),engine(std::make_unique<eg::Engine>()){}
AudioProcessorValueTreeState state;std::unique_ptr<eg::Engine> engine;std::atomic<float> peak{0},peakR{0};std::array<std::atomic<float>,512> analysis{};std::atomic<unsigned> analysisHead{0};float get(String id)const{return state.getRawParameterValue(id)->load();}void set(String id,float x){if(auto* p=state.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(x));p->endChangeGesture();}}
const String getName()const override{return "Equilibrium Grain";}bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}double getTailLengthSeconds()const override{return 12;}int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const String getProgramName(int)override{return "Init";}void changeProgramName(int,const String&)override{}bool hasEditor()const override{return true;}AudioProcessorEditor* createEditor()override;void prepareToPlay(double r,int)override{engine->prepare(r);}void releaseResources()override{}bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainOutputChannelSet()==AudioChannelSet::stereo();}
void processBlock(AudioBuffer<float>& b,MidiBuffer& midi)override{ScopedNoDenormals nd;b.clear();if(b.getNumChannels()<2)return;eg::Settings s;for(int i=0;i<4;++i){auto& l=s.layers[i];auto v=[&](const char* id){return get(lid(i,id));};l.level=v("level");l.pan=v("pan");l.size=v("size");l.density=v("density");l.position=v("position");l.scatter=v("scatter");l.pitch=v("pitch");l.drift=v("drift");l.spread=v("spread");l.motion=v("motion");l.cutoff=v("cutoff");l.cutoff2=v("cutoff2");l.resonance=v("resonance");l.resonance2=v("resonance2");l.filterMode=int(v("filterMode"));l.filterMode2=int(v("filterMode2"));l.filterRouting=int(v("filterRouting"));l.source=int(v("source"));l.direction=int(v("direction"));l.shape=int(v("shape"));l.enabled=v("enabled")>.5f;}
#define READ(x) s.x=get(#x)
READ(osc1);READ(osc2);READ(oscPitch1);READ(oscPitch2);READ(fm);READ(ring);READ(attack);READ(release);READ(master);READ(velocity);READ(lfoRate);READ(lfoDepth);READ(xyX);READ(xyY);READ(xyPos);READ(xyDensity);READ(drive);
#undef READ
for(int i=0;i<8;++i)s.macros[i]=get("macro"+String(i));for(int i=0;i<12;++i){auto& fx=s.effects[i];fx.type=int(get(fxid(i,"type")));fx.enabled=get(fxid(i,"enabled"))>.5f;fx.amount=get(fxid(i,"amount"));fx.mix=get(fxid(i,"mix"));}int cursor=0;for(auto m:midi){int end=jlimit(cursor,b.getNumSamples(),m.samplePosition);if(end>cursor)engine->render(b.getWritePointer(0,cursor),b.getWritePointer(1,cursor),end-cursor,s);auto msg=m.getMessage();if(msg.isNoteOn())engine->noteOn(msg.getNoteNumber(),msg.getFloatVelocity());else if(msg.isNoteOff())engine->noteOff(msg.getNoteNumber());else if(msg.isAllNotesOff()||msg.isAllSoundOff())engine->allOff();cursor=end;}if(cursor<b.getNumSamples())engine->render(b.getWritePointer(0,cursor),b.getWritePointer(1,cursor),b.getNumSamples()-cursor,s);peak.store(jmax(b.getMagnitude(0,0,b.getNumSamples()),peak.load()*.85f));peakR.store(jmax(b.getMagnitude(1,0,b.getNumSamples()),peakR.load()*.85f));auto head=analysisHead.load(std::memory_order_relaxed);for(int k=0;k<b.getNumSamples();++k)analysis[(head++)%512].store(.5f*(b.getSample(0,k)+b.getSample(1,k)),std::memory_order_relaxed);analysisHead.store(head,std::memory_order_release);}
void getStateInformation(MemoryBlock& b)override{if(auto x=state.copyState().createXml())copyXmlToBinary(*x,b);}void setStateInformation(const void* d,int n)override{if(auto x=getXmlFromBinary(d,n))if(x->hasTagName(state.state.getType()))state.replaceState(ValueTree::fromXml(*x));}
};

static String tr(bool fr,const char* a,const char* b){return String::fromUTF8(fr?a:b);}
class ReferenceLook : public Metal {
public:
 ReferenceLook(){
  setColour(ComboBox::backgroundColourId,Colour(0xff1c211e));setColour(ComboBox::outlineColourId,Colour(0xff41483c));setColour(ComboBox::textColourId,ink);setColour(ComboBox::arrowColourId,muted);
  setColour(PopupMenu::backgroundColourId,Colour(0xff202622));setColour(PopupMenu::textColourId,ink);setColour(PopupMenu::highlightedBackgroundColourId,Colour(0xff57513a));setColour(PopupMenu::highlightedTextColourId,champ);
  setColour(TextEditor::backgroundColourId,Colour(0xff171d19));setColour(TextEditor::outlineColourId,Colour(0xff41483c));setColour(TextEditor::focusedOutlineColourId,gold);setColour(TextEditor::textColourId,ink);
 }
 void drawButtonBackground(Graphics& g,Button& b,const Colour&,bool over,bool down)override{
  auto r=b.getLocalBounds().toFloat().reduced(.8f);
  bool on=b.getToggleState();auto tab=b.getProperties().contains("tab");
  Colour top=on?Colour(0xff514c36):Colour(0xff282e28);
  Colour bot=on?Colour(0xff393b2d):Colour(0xff1b211e);
  if(over)top=top.brighter(.10f);if(down)bot=bot.darker(.16f);
  g.setGradientFill(ColourGradient(top,0,r.getY(),bot,0,r.getBottom(),false));
  if(tab)g.fillRect(r);else g.fillRoundedRectangle(r,3);
  g.setColour(on?gold.withAlpha(.7f):Colour(0xff41493e));if(tab)g.drawRect(r,.8f);else g.drawRoundedRectangle(r,3,1);
  if(tab&&on){g.setColour(champ);g.fillRect(r.getX()+17,r.getBottom()-3,r.getWidth()-34,2.5f);}
 }
 void drawButtonText(Graphics& g,TextButton& b,bool,bool)override{
  g.setColour((b.getToggleState()?champ:ink).withMultipliedAlpha(b.isEnabled()?1.f:.45f));
  g.setFont(FontOptions(b.getProperties().contains("tab")?12.f:14.f,Font::bold));
  g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(9,2),b.getProperties().contains("left")?Justification::centredLeft:Justification::centred,1,.75f);
 }
 Font getComboBoxFont(ComboBox&)override{return Font(FontOptions(14.f));}
 Font getPopupMenuFont()override{return Font(FontOptions(15.f));}
};
class Knob:public Component {
public:
 Slider slider;Label label;std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> attachment;
 Knob(Processor& p,String id,String name){
  label.setText(name,dontSendNotification);label.setJustificationType(Justification::centred);label.setColour(Label::textColourId,Colour(0xffb4b7a9));label.setFont(FontOptions(13.f,Font::bold));addAndMakeVisible(label);
  slider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(Slider::TextBoxBelow,false,92,19);slider.setRotaryParameters(MathConstants<float>::pi*1.25f,MathConstants<float>::pi*2.75f,true);addAndMakeVisible(slider);
  attachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,id,slider);
  if(auto* param=p.state.getParameter(id))slider.setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
  slider.setColour(Slider::textBoxTextColourId,champ);slider.setColour(Slider::textBoxBackgroundColourId,Colours::transparentBlack);
  if(id.endsWith("_size"))slider.setTextValueSuffix(" ms");
  if(id.endsWith("_density"))slider.setTextValueSuffix(" Hz");
  if(id.endsWith("_pitch")||id.endsWith("_drift")||id.startsWith("oscPitch"))slider.setTextValueSuffix(" st");
  if(id.endsWith("_cutoff")||id.endsWith("_cutoff2"))slider.setTextValueSuffix(" Hz");
  if(id=="master")slider.setTextValueSuffix(" dB");
 }
 void resized()override{auto r=getLocalBounds();label.setBounds(r.removeFromTop(20));slider.setBounds(r);}
};
class ReferenceSurface:public Component,private Timer {
public:
 explicit ReferenceSurface(Processor& p):proc(p){
  setLookAndFeel(&look);setSize(1500,1000);setOpaque(true);
  for(int i=0;i<8;++i){auto& b=tabs[i];b.getProperties().set("tab",true);b.onClick=[this,i]{page=i;rebuild();};addAndMakeVisible(b);}
  for(int i=0;i<4;++i){auto& b=layers[i];b.onClick=[this,i]{layer=i;rebuild();};addAndMakeVisible(b);}
  language.setButtonText("FR / EN");language.onClick=[this]{fr=!fr;rebuild();};addAndMakeVisible(language);
  sound.addItem(tr(fr,"Son personnalisé","Custom sound"),1);for(int i=0;i<8;++i)sound.addItem(presetNames()[i],i+2);sound.setSelectedId(1,dontSendNotification);sound.onChange=[this]{int n=sound.getSelectedId()-2;if(n>=0)loadPreset(n);};addAndMakeVisible(sound);
  previous.setButtonText("<");next.setButtonText(">");previous.onClick=[this]{loadPreset((current+7)%8);};next.onClick=[this]{loadPreset((current+1)%8);};addAndMakeVisible(previous);addAndMakeVisible(next);
  for(int i=0;i<8;++i){macros[i]=std::make_unique<Knob>(proc,"macro"+String(i),macroNames[i]);addAndMakeVisible(*macros[i]);if(i==6)macros[i]->slider.setTooltip("Space: global effects wet/dry");}
  master=std::make_unique<Knob>(proc,"master","Master");addAndMakeVisible(*master);
  for(int i=0;i<8;++i)favorites[i]=bool(proc.state.state.getProperty("favorite"+String(i),false));
  rebuild();startTimerHz(20);
 }
 ~ReferenceSurface()override{stopTimer();clearPage();setLookAndFeel(nullptr);}
 void resized()override{
  for(int i=0;i<8;++i)tabs[i].setBounds(26+i*180,118,180,60);
  for(int i=0;i<4;++i)layers[i].setBounds(26+i*91,page==4?263:198,84,41);
  sound.setBounds(950,53,245,38);previous.setBounds(1202,53,29,38);next.setBounds(1236,53,29,38);language.setBounds(1394,14,78,26);
  for(int i=0;i<8;++i)if(macros[i])macros[i]->setBounds(155+i*142,833,130,125);
  if(master)master->setBounds(1328,833,133,125);
 }
 void paint(Graphics& g)override{
  g.fillAll(Colour(0xff171a19));
  g.setGradientFill(ColourGradient(Colour(0xff292b28),0,0,Colour(0xff161918),1500,120,false));g.fillRect(0,0,1500,118);
  g.setColour(champ);Path logo;logo.startNewSubPath(28,44);logo.lineTo(28,84);logo.lineTo(62,77);logo.startNewSubPath(39,48);logo.lineTo(39,67);logo.lineTo(62,61);g.strokePath(logo,PathStrokeType(3.2f));
  text(g,"L I B E R T Y",76,38,145,33,25,champ,true);
  text(g,"SOUND WITHOUT LIMITS",77,76,150,15,10,muted,true);
  g.setColour(Colour(0xff5c5a4b));g.drawVerticalLine(236,34,90);g.drawHorizontalLine(117,0,1500);
  text(g,"EQUILIBRIUM",266,29,232,45,33,ink,false);text(g,"GRAIN",503,29,205,45,33,champ,true);
  text(g,"Advanced Granular Synthesis Instrument",267,76,560,20,14,muted,true);
  text(g,tr(fr,"DERNIER SON","SOUND STATE"),950,31,240,16,11,muted,true);
  text(g,tr(fr,"SORTIE","OUTPUT"),1291,43,86,13,10,muted,true);meter(g,{1290,65,174,17});
  g.setColour(Colour(0xff171c19));g.fillRect(0,118,1500,61);
  switch(page){case 0:paintGranular(g);break;case 1:paintLab(g);break;case 2:paintSynth(g);break;case 3:paintMod(g);break;case 4:paintFilters(g);break;case 5:paintMotion(g);break;case 6:paintPerformance(g);break;case 7:paintBrowser(g);break;}
  g.setGradientFill(ColourGradient(Colour(0xff292d27),0,818,Colour(0xff1b211d),0,970,false));g.fillRect(0,818,1500,153);
  g.setColour(gold.withAlpha(.65f));g.drawHorizontalLine(818,0,1500);g.setColour(Colour(0xff545b48));g.drawVerticalLine(1296,835,957);
  text(g,"PERFORMANCE",26,862,120,17,11,muted,true);text(g,"M A C R O S",26,880,130,28,19,champ,true);text(g,tr(fr,"Contrôles directs","Direct controls"),26,918,125,15,10,muted,false);
  g.setColour(Colour(0xff141a16));g.fillRect(0,971,1500,29);g.setColour(Colour(0xff4e5647));g.drawHorizontalLine(971,0,1500);
  text(g,tr(fr,"SOURCES INTERNES  /  MIDI POLYPHONIQUE  /  V0.2.1","BUILT-IN SOURCES  /  MIDI POLYPHONIC  /  V0.2.1"),26,978,700,15,10,muted,true);
  text(g,"SHAPE SOUND BEYOND REALITY.",1110,978,365,15,10,muted,true,Justification::centredRight);
  for(auto pt:{Point<float>(9,9),Point<float>(1490,9),Point<float>(9,990),Point<float>(1490,990)}){g.setColour(Colour(0xff62675b));g.drawEllipse(pt.x-3,pt.y-3,6,6,1);g.drawLine(pt.x-1.5f,pt.y+1.5f,pt.x+1.5f,pt.y-1.5f,1);}
 }
 void mouseDown(const MouseEvent& e)override{mouseDrag(e);}
 void mouseDrag(const MouseEvent& e)override{
  auto p=e.position;
  if((page==0||page==1)&&waveRect().contains(p)){auto r=waveRect();auto range=waveRange();proc.set(lid(layer,"position"),jlimit(0.f,1.f,range.first+(p.x-r.getX())/r.getWidth()*range.second));}
  if((page==5||page==6)&&xyRect().contains(p)){auto r=xyRect();proc.set("xyX",jlimit(0.f,1.f,(p.x-r.getX())/r.getWidth()));proc.set("xyY",jlimit(0.f,1.f,1-(p.y-r.getY())/r.getHeight()));}
  if(page==4&&!rackPage)for(int i=0;i<2;++i){auto r=filterRect(i);if(r.contains(p))proc.set(lid(layer,i?"cutoff2":"cutoff"),20.f*std::pow(1000.f,jlimit(0.f,1.f,(p.x-r.getX())/r.getWidth())));}
 }
private:
 Processor& proc;ReferenceLook look;bool fr=true;int page=0,layer=0,current=-1,zoom=1,category=0;bool rackPage=false;std::array<bool,8> favorites{};
 std::array<TextButton,8> tabs;std::array<TextButton,4> layers;TextButton language,previous,next;ComboBox sound;
 std::array<std::unique_ptr<Knob>,8> macros;std::unique_ptr<Knob> master;
 std::vector<std::unique_ptr<Component>> controls;std::vector<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>> combos;std::vector<std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>> switches;
 std::array<TextButton*,8> rows{},stars{};std::array<int,8> rowIds{};TextEditor* search=nullptr;TextButton* active=nullptr;std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> driveAttachment;std::array<Knob*,12> fxAmountControls{},fxMixControls{};std::array<TextButton*,12> fxOnControls{};std::array<Knob*,2> filterQ{};
 std::array<float,64> spectrum{};
 static const char* const* presetNames(){static const char* n[]={"Init","Golden Atmosphere","Frozen Memories","Infinite Horizon","Quantum Dust","Velvet Clouds","Liquid Time","Metallic Dreams"};return n;}
 static const char* const* sources(){static const char* n[]={"Velvet","Silk","Glass","Amber","Mist","Crystal","Dust","Metal"};return n;}
 int presetCategory(int n)const{return n==0?0:n<4?1:n==5?1:n==4||n==7?2:3;}
 String catName(int n)const{switch(n){case 1:return tr(fr,"Pads atmosphériques","Atmospheric Pads");case 2:return tr(fr,"Textures expérimentales","Experimental Textures");case 3:return tr(fr,"Paysages sonores","Soundscapes");default:return "Init";}}
 static void text(Graphics& g,String s,int x,int y,int w,int h,float font,Colour c,bool bold=false,Justification j=Justification::centredLeft){g.setColour(c);g.setFont(FontOptions(font,bold?Font::bold:Font::plain));g.drawFittedText(s,x,y,w,h,j,1,.75f);}
 void panelBox(Graphics& g,Rectangle<float> r){g.setGradientFill(ColourGradient(Colour(0xff282d28),r.getX(),r.getY(),Colour(0xff202622),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,5);g.setColour(Colour(0xff424b3e));g.drawRoundedRectangle(r,5,1.5f);}
 void panelTitle(Graphics& g,int x,int y,int w,String kicker,String name){text(g,kicker,x,y,w,16,11,muted,true);text(g,name,x,y+20,w,26,19,ink,true);}
 void note(Graphics& g,String s,int x,int y,int w){text(g,s,x,y,w,18,13,muted);}
 void unavailable(Graphics& g,Rectangle<float> r,String title,String desc){panelBox(g,r);panelTitle(g,int(r.getX())+18,int(r.getY())+17,int(r.getWidth())-36,tr(fr,"À IMPLÉMENTER","NOT IMPLEMENTED"),title);note(g,desc,int(r.getX())+18,int(r.getY())+68,int(r.getWidth())-36);}
 void meter(Graphics& g,Rectangle<float> r){
  float levels[]={proc.peak.load(),proc.peakR.load()};
  for(int ch=0;ch<2;++ch){float db=Decibels::gainToDecibels(levels[ch],-60.f);float n=jlimit(0.f,1.f,(db+60)/60);for(int i=0;i<30;++i){g.setColour(i<30*n?(i<25?gold:Colour(0xffd0a059)):Colour(0xff434b34));g.fillRect(r.getX()+i*r.getWidth()/30,r.getY()+ch*r.getHeight()/2,r.getWidth()/30-1.5f,r.getHeight()/2-2);}}
 }
 Rectangle<float> waveRect()const{return page==0?Rectangle<float>(296,299,1170,201):Rectangle<float>(26,299,1440,223);}
 Rectangle<float> xyRect()const{return page==5?Rectangle<float>(48,322,663,278):Rectangle<float>(48,265,660,281);}
 Rectangle<float> filterRect(int i)const{return {float(47+i*720),453,661,146};}
 std::pair<float,float> waveRange()const{float span=1.f/zoom;float pos=proc.get(lid(layer,"position"));return {jlimit(0.f,1.f-span,pos-span*.5f),span};}
 void wave(Graphics& g){
  auto r=waveRect();auto top=r.withY(r.getY()-43).withHeight(43);auto base=r.withY(r.getBottom()).withHeight(29);
  panelBox(g,top.getUnion(r).getUnion(base));int source=jlimit(0,7,int(proc.get(lid(layer,"source"))));
  text(g,sources()[source],int(top.getX())+16,int(top.getY())+7,140,28,17,champ,true);text(g,tr(fr,"SOURCE INTERNE / COUCHE ","BUILT-IN SOURCE / LAYER ")+String::charToString(juce_wchar('A'+layer)),int(top.getX())+155,int(top.getY())+10,500,24,11,muted,true);
  g.setColour(Colour(0xff1b211d));g.fillRect(r);
  auto range=waveRange();for(int i=0;i<=16;++i){float x=r.getX()+i*r.getWidth()/16;g.setColour(Colour(0xff333d32));g.drawVerticalLine(int(x),r.getY(),r.getBottom());if(i%4==0&&i<16)text(g,String(range.first+range.second*i/16,3)+" s",int(x)+6,int(r.getY())+5,90,16,12,muted);}
  g.setColour(Colour(0xff343b32));g.drawHorizontalLine(int(r.getCentreY()),r.getX(),r.getRight());
  auto& src=proc.engine->sources[source];Path path;int count=int(r.getWidth());for(int i=0;i<count;++i){float t=range.first+range.second*float(i)/float(count-1);int idx=jlimit(0,int(src.size())-1,int(t*float(src.size()-1)));float y=r.getCentreY()-src[size_t(idx)]*r.getHeight()*.42f;if(i==0)path.startNewSubPath(r.getX(),y);else path.lineTo(r.getX()+i,y);}
  g.setColour(champ.withAlpha(.86f));g.strokePath(path,PathStrokeType(1.15f));
  float px=r.getX()+r.getWidth()*(proc.get(lid(layer,"position"))-range.first)/range.second;if(px>=r.getX()&&px<=r.getRight()){g.setColour(champ);g.drawVerticalLine(int(px),r.getY(),r.getBottom());Path tri;tri.addTriangle(px-6,r.getY(),px+6,r.getY(),px,r.getY()+8);g.fillPath(tri);}
  g.setColour(Colour(0xff273023));g.fillRect(base);text(g,tr(fr,"POSITION ","POSITION ")+String(proc.get(lid(layer,"position"))*100,1)+" %",int(base.getX())+16,int(base.getY())+4,320,20,11,muted,true);text(g,tr(fr,"GLISSER POUR DÉPLACER","DRAG TO SCAN"),int(base.getRight())-255,int(base.getY())+4,240,20,11,muted,true,Justification::centredRight);
 }
 void drawSpectrum(Graphics& g,Rectangle<float> r){g.setColour(Colour(0xff1a201c));g.fillRect(r);for(int i=1;i<4;++i){g.setColour(Colour(0xff303a31));g.drawHorizontalLine(int(r.getY()+i*r.getHeight()/4),r.getX(),r.getRight());}Path p;for(int i=0;i<64;++i){float x=r.getX()+r.getWidth()*i/63;float y=r.getBottom()-jlimit(0.f,1.f,spectrum[i])*r.getHeight();if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);}g.setColour(gold);g.strokePath(p,PathStrokeType(1.7f));}
 void drawXY(Graphics& g){auto r=xyRect();g.setColour(Colour(0xff19211e));g.fillRect(r);g.setColour(Colour(0xff43503d));g.drawRect(r,1);for(int i=1;i<8;++i){g.setColour(Colour(0xff344036));g.drawVerticalLine(int(r.getX()+i*r.getWidth()/8),r.getY(),r.getBottom());g.drawHorizontalLine(int(r.getY()+i*r.getHeight()/8),r.getX(),r.getRight());}g.setColour(gold.withAlpha(.4f));g.drawVerticalLine(int(r.getCentreX()),r.getY(),r.getBottom());g.drawHorizontalLine(int(r.getCentreY()),r.getX(),r.getRight());g.setColour(muted.withAlpha(.4f));g.drawEllipse(r.reduced(r.getWidth()*.23f,r.getHeight()*.14f),1.1f);float x=r.getX()+proc.get("xyX")*r.getWidth(),y=r.getBottom()-proc.get("xyY")*r.getHeight();g.setColour(gold);g.drawEllipse(x-35,y-19,70,38,2);g.setColour(champ);g.fillEllipse(x-11,y-6,22,12);text(g,tr(fr,"Y / DENSITÉ","Y / DENSITY"),int(r.getX())+24,int(r.getY())+12,200,20,15,muted,true);text(g,"X / POSITION",int(r.getRight())-200,int(r.getBottom())-30,175,20,15,muted,true,Justification::centredRight);}
 void paintGranular(Graphics& g){
  panelBox(g,{26,256,252,354});panelTitle(g,43,269,220,tr(fr,"COUCHE GRANULAIRE","GRANULAR LAYER")+" "+String::charToString(juce_wchar('A'+layer)),tr(fr,"Matériau source","Source material"));
  note(g,tr(fr,"Banque interne","Built-in source bank"),43,321,220);note(g,tr(fr,"Direction des grains","Grain direction"),43,552,220);
  wave(g);text(g,tr(fr,"SPECTRE DE SORTIE","OUTPUT SPECTRUM"),296,539,400,18,11,muted,true);drawSpectrum(g,{296,563,880,44});note(g,tr(fr,"Forme d'enveloppe","Envelope shape"),1200,540,265);
  panelBox(g,{26,623,1441,163});panelTitle(g,46,632,470,tr(fr,"TEMPS / HAUTEUR / ESPACE","TIME / PITCH / SPACE"),tr(fr,"Architecture des grains","Grain architecture"));
 }
 void paintLab(Graphics& g){
  wave(g);panelBox(g,{26,568,700,228});panelTitle(g,47,583,660,"SOURCE LAB",tr(fr,"Région et préparation","Region & preparation"));note(g,tr(fr,"Sources internes uniquement dans cette version","Built-in sources only in this version"),48,634,650);
  panelBox(g,{747,568,720,228});panelTitle(g,768,583,670,tr(fr,"VARIATION PAR GRAIN","PER-GRAIN VARIATION"),tr(fr,"Variation des grains","Grain variation"));
 }
 void paintSynth(Graphics& g){
  for(int i=0;i<2;++i){int x=26+i*720;panelBox(g,{float(x),198,702,352});panelTitle(g,x+21,214,660,tr(fr,"SYNTHÈSE HYBRIDE","HYBRID SYNTHESIS"),"Oscillator 0"+String(i+1));note(g,tr(fr,"Sinusoïde / accord indépendant","Sine / independent tuning"),x+21,278,660);g.setColour(gold.withAlpha(.8f));Path p;for(int k=0;k<630;++k){float yy=338.f+13.f*std::sin(k*.028f);if(k==0)p.startNewSubPath(float(x+35),yy);else p.lineTo(float(x+35+k),yy);}g.strokePath(p,PathStrokeType(1.2f));note(g,tr(fr,"Autres formes et unisson : non implémentés","Other waveforms and unison: not implemented"),x+21,522,660);}
  panelBox(g,{26,568,702,228});panelTitle(g,47,583,660,tr(fr,"MODULATION ET MIXAGE","MODULATION & MIX"),tr(fr,"Interaction hybride","Hybrid interaction"));
  panelBox(g,{746,568,721,228});panelTitle(g,767,583,680,tr(fr,"SIGNAL DE SORTIE","OUTPUT AUDIO"),tr(fr,"Analyse de sortie","Output analysis"));drawSpectrum(g,{768,655,677,121});
 }
 void paintMod(Graphics& g){
  panelBox(g,{26,198,1441,73});panelTitle(g,46,209,1370,tr(fr,"MODULATION","MODULATION"),tr(fr,"LFO de position","Position LFO"));
  panelBox(g,{26,291,700,505});panelTitle(g,47,311,650,tr(fr,"SOURCE ACTIVE","ACTIVE SOURCE"),"LFO 01");auto r=Rectangle<float>(48,380,656,190);g.setColour(Colour(0xff18211c));g.fillRect(r);g.setColour(Colour(0xff394638));g.drawHorizontalLine(int(r.getCentreY()),r.getX(),r.getRight());Path p;for(int i=0;i<655;++i){float y=r.getCentreY()-std::sin(float(i)*2*eg::pi/655)*70;if(i==0)p.startNewSubPath(r.getX(),y);else p.lineTo(r.getX()+i,y);}g.setColour(champ);g.strokePath(p,PathStrokeType(2));
  panelBox(g,{747,291,720,244});panelTitle(g,768,311,678,tr(fr,"ROUTAGE ACTIF","ACTIVE ROUTING"),tr(fr,"LFO 1 vers position des grains","LFO 1 to grain position"));note(g,tr(fr,"Profondeur et vitesse reliées au moteur audio.","Depth and rate connected to audio engine."),768,383,678);
  unavailable(g,{747,553,720,243},tr(fr,"Matrice 64 routes et autres sources","64-slot matrix and additional sources"),tr(fr,"Disponibles après implémentation du moteur de modulation.","Available after modulation engine implementation."));
 }
 StringArray filterModes()const{return {tr(fr,"Bypass","Bypass"),tr(fr,"Passe-bas 12 dB","Low-pass 12 dB"),tr(fr,"Passe-haut 12 dB","High-pass 12 dB"),tr(fr,"Passe-bande","Band-pass"),tr(fr,"Coupe-bande","Notch"),tr(fr,"Passe-bas 24 dB","Low-pass 24 dB"),tr(fr,"Passe-haut 24 dB","High-pass 24 dB"),tr(fr,"Passe-bas 6 dB (original)","Low-pass 6 dB (original)")};}
 StringArray fxNames()const{return {tr(fr,"Vide","Empty"),"Saturation","Chorus","Delay",tr(fr,"Réverbération","Reverb"),"EQ",tr(fr,"Compresseur","Compressor"),"Phaser","Flanger","Bitcrusher",tr(fr,"Décalage fréquentiel","Frequency Shifter"),tr(fr,"Image stéréo","Stereo Imager"),tr(fr,"Limiteur","Limiter")};}
 String fxAmount(int t)const{switch(t){case 1:return "Drive";case 2:case 7:case 8:return tr(fr,"Mouvement","Motion");case 3:return tr(fr,"Temps","Time");case 4:return tr(fr,"Durée","Decay");case 5:return tr(fr,"Tonalité","Tone");case 6:return tr(fr,"Seuil","Threshold");case 9:return tr(fr,"Réduction","Reduction");case 10:return tr(fr,"Décalage","Shift");case 11:return tr(fr,"Largeur","Width");case 12:return tr(fr,"Plafond","Ceiling");default:return tr(fr,"Quantité","Amount");}}
 void paintFilters(Graphics& g){
  g.setColour(Colour(0xff485044));g.drawHorizontalLine(247,26,1467);
  if(rackPage){
   text(g,tr(fr,"TRAITEMENT STÉRÉO / 12 SLOTS EN SÉRIE","STEREO PROCESSING / 12 SERIAL SLOTS"),830,207,637,26,12,muted,true,Justification::centredRight);
   for(int i=0;i<12;++i){int x=26+(i%4)*365,y=260+(i/4)*178;panelBox(g,{float(x),float(y),346,170});text(g,"SLOT "+String(i+1).paddedLeft('0',2),x+12,y+8,105,19,12,champ,true);}
   return;
  }
  text(g,tr(fr,"FILTRES INDÉPENDANTS PAR COUCHE","INDEPENDENT LAYER FILTERS"),850,207,617,24,12,muted,true,Justification::centredRight);
  text(g,tr(fr,"Routage","Routing"),1195,269,105,28,13,muted,true);
  for(int i=0;i<2;++i){int x=26+i*720;panelBox(g,{float(x),313,702,435});panelTitle(g,x+21,330,660,tr(fr,"COUCHE ","LAYER ")+String::charToString(juce_wchar('A'+layer)),"Filter 0"+String(i+1));note(g,tr(fr,"Mode","Mode"),x+21,390,660);
   auto r=filterRect(i);g.setColour(Colour(0xff19221a));g.fillRect(r);g.setColour(Colour(0xff43503d));g.drawRect(r,1.f);
   for(int k=1;k<4;++k){g.setColour(Colour(0xff344235));g.drawHorizontalLine(int(r.getY()+k*r.getHeight()/4),r.getX(),r.getRight());}
   int mode=int(proc.get(lid(layer,i?"filterMode2":"filterMode")));float fc=proc.get(lid(layer,i?"cutoff2":"cutoff"))*std::pow(4.f,proc.get("macro5")-.5f);float q=proc.get(lid(layer,i?"resonance2":"resonance"));double sr=proc.getSampleRate();if(sr<=0)sr=48000;
   auto c=eg::FilterCoeffs::make(mode,fc,q,sr);Path p;for(int k=0;k<int(r.getWidth());++k){float hz=20.f*std::pow(1000.f,float(k)/(r.getWidth()-1));float db=Decibels::gainToDecibels(c.magnitude(hz,sr),-60.f);float y=r.getY()+jlimit(0.f,1.f,(18.f-db)/78.f)*r.getHeight();if(k==0)p.startNewSubPath(r.getX(),y);else p.lineTo(r.getX()+k,y);}
   g.setColour(gold);g.strokePath(p,PathStrokeType(2));float pos=jlimit(0.f,1.f,std::log(fc/20.f)/std::log(1000.f));g.setColour(champ.withAlpha(.5f));g.drawVerticalLine(int(r.getX()+pos*r.getWidth()),r.getY(),r.getBottom());
   if(mode==0||mode==7)note(g,mode==0?tr(fr,"Filtre contourné","Filter bypassed"):tr(fr,"Mode original sans résonance","Original non-resonant mode"),x+380,711,300);
  }
  panelBox(g,{26,758,1441,48});text(g,tr(fr,"TRAITEMENT DE SORTIE / SATURATION","OUTPUT PROCESSING / SATURATION"),45,769,450,25,13,muted,true);
 }
 void paintMotion(Graphics& g){
  panelBox(g,{26,254,709,542});panelTitle(g,47,270,670,"PERFORMANCE XY",tr(fr,"Champ de mouvement","Motion field"));drawXY(g);note(g,tr(fr,"Déplace le vecteur XY pour modifier le son.","Drag the XY vector to change the sound."),48,626,665);note(g,tr(fr,"Mouvements programmés et séquenceur : non implémentés.","Programmed motion and sequencer: not implemented."),48,755,665);
  panelBox(g,{755,254,712,542});panelTitle(g,777,270,668,tr(fr,"UN MOUVEMENT / PLUSIEURS DIMENSIONS","ONE MOVEMENT / MULTIPLE DIMENSIONS"),tr(fr,"Destinations du vecteur","Vector destinations"));note(g,tr(fr,"Position et densité répondent simultanément.","Position and density respond simultaneously."),777,573,668);
 }
 void paintPerformance(Graphics& g){
  panelBox(g,{26,198,702,443});panelTitle(g,47,213,660,tr(fr,"CONTRÔLE EXPRESSIF","EXPRESSIVE CONTROL"),tr(fr,"Vecteur de performance","Live performance vector"));drawXY(g);note(g,tr(fr,"Les quantités se règlent sur la page Mouvement.","Amounts are set on the Motion page."),47,584,660);
  panelBox(g,{746,198,721,443});panelTitle(g,768,213,677,tr(fr,"MIDI POLYPHONIQUE","POLYPHONIC MIDI"),tr(fr,"Réponse de l'instrument","Instrument response"));text(g,tr(fr,"SORTIE","OUTPUT"),768,404,300,18,12,muted,true);meter(g,{768,435,676,34});note(g,"-60                         -36                         -18                         -6                    0 dB",768,475,677);
  text(g,"4",768,520,190,49,34,champ);text(g,"2",1002,520,190,49,34,champ);text(g,"16",1230,520,190,49,34,champ);note(g,tr(fr,"Couches granulaires","Granular layers"),768,571,200);note(g,tr(fr,"Oscillateurs hybrides","Hybrid oscillators"),1002,571,220);note(g,tr(fr,"Voix","Voices"),1230,571,190);
  panelBox(g,{26,660,1441,136});panelTitle(g,47,675,1370,tr(fr,"CONTRÔLES FIXES","FIXED ASSIGNMENTS"),tr(fr,"Macros de performance","Performance macros"));note(g,tr(fr,"Les huit macros sont actives. Space règle la proportion globale des effets.","All eight macros are active. Space controls global effects wet/dry."),47,735,1370);
 }
 void paintBrowser(Graphics& g){
  panelBox(g,{26,198,295,600});panelTitle(g,47,214,253,"LIBERTY SOUND LIBRARY",tr(fr,"Trouvez votre équilibre.","Find your equilibrium."));note(g,tr(fr,"8 sons de cette version","8 sounds in this version"),47,741,252);
  g.setColour(Colour(0xff303b2c));g.fillRect(342,251,1125,37);text(g,tr(fr,"NOM DU SON","SOUND NAME"),360,257,500,24,11,muted,true);text(g,tr(fr,"CATÉGORIE","CATEGORY"),1003,257,260,24,11,muted,true);text(g,tr(fr,"FAVORI","FAVORITE"),1380,257,72,24,11,muted,true);
  for(int i=0;i<8;++i){int n=rowIds[i];if(n<0)continue;auto y=289+i*56;g.setColour(i%2?Colour(0xff222b22):Colour(0xff1c251e));g.fillRect(342,y,1125,54);text(g,catName(presetCategory(n)),1003,y+10,310,32,14,muted);}
  note(g,tr(fr,"Banque d'usine protégée. Les presets utilisateur ne sont pas encore disponibles.","Factory sounds protected. User presets are not implemented yet."),344,773,1120);
 }
 template<class T> T* addControl(std::unique_ptr<T> p,Rectangle<int> bounds){auto* raw=p.get();addAndMakeVisible(*raw);raw->setBounds(bounds);controls.push_back(std::move(p));return raw;}
 TextButton* button(String name,Rectangle<int> r,std::function<void()> fn){auto p=std::make_unique<TextButton>(name);p->onClick=std::move(fn);return addControl(std::move(p),r);}
 Knob* knob(String id,String title,int x,int y,int w,int h){return addControl(std::make_unique<Knob>(proc,id,title),{x,y,w,h});}
 Knob* local(const char* id,int x,int y,int w,int h){for(const auto& s:layerSpecs)if(String(s.id)==id)return knob(lid(layer,id),tr(fr,s.fr,s.en),x,y,w,h);return nullptr;}
 Knob* global(const char* id,int x,int y,int w,int h){for(const auto& s:globalSpecs)if(String(s.id)==id)return knob(id,tr(fr,s.fr,s.en),x,y,w,h);return nullptr;}
 void combo(const char* id,StringArray values,Rectangle<int> r){auto b=std::make_unique<ComboBox>();for(int i=0;i<values.size();++i)b->addItem(values[i],i+1);auto* raw=addControl(std::move(b),r);combos.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,lid(layer,id),*raw));}
 void sourceCombo(Rectangle<int> r){StringArray v;for(int i=0;i<8;++i)v.add(sources()[i]);combo("source",v,r);}
 void zoomButtons(){auto r=waveRect();int x=int(r.getRight());button("-",{x-112,267,27,24},[this]{zoom=jmax(1,zoom/2);repaint();});button("+",{x-39,267,27,24},[this]{zoom=jmin(8,zoom*2);repaint();});}
 void clearPage(){driveAttachment.reset();fxAmountControls.fill(nullptr);fxMixControls.fill(nullptr);fxOnControls.fill(nullptr);filterQ.fill(nullptr);switches.clear();combos.clear();controls.clear();rows.fill(nullptr);stars.fill(nullptr);rowIds.fill(-1);active=nullptr;search=nullptr;}
 void rebuild(){
  clearPage();const char* f[]={"MOTEUR GRANULAIRE","SAMPLE LAB","SYNTHÈSE","MODULATION","FILTRES & EFFETS","MOUVEMENT & SÉQUENCEUR","PERFORMANCE","PRESETS"};const char* e[]={"GRANULAR ENGINE","SAMPLE LAB","SYNTHESIS","MODULATION","FILTERS & EFFECTS","MOTION & SEQUENCER","PERFORMANCE","PRESET BROWSER"};
  sound.changeItemText(1,tr(fr,"Son personnalisé","Custom sound"));
  for(int i=0;i<8;++i){tabs[i].setButtonText(String(i+1).paddedLeft('0',2)+"  "+tr(fr,f[i],e[i]));tabs[i].setToggleState(i==page,dontSendNotification);}
  for(int i=0;i<4;++i){layers[i].setVisible(page==0||page==1||page==4&&!rackPage);layers[i].setBounds(26+i*91,page==4?263:198,84,41);layers[i].setButtonText(String::charToString(juce_wchar('A'+i))+"  "+tr(fr,"COUCHE","LAYER"));layers[i].setToggleState(i==layer,dontSendNotification);}
  if(page==0){
   sourceCombo({43,342,217,35});local("level",50,386,100,111);local("pan",156,386,100,111);
   combo("direction",{tr(fr,"Avant","Forward"),tr(fr,"Arrière","Reverse"),tr(fr,"Aléatoire","Random")},{43,574,217,31});
   combo("shape",{"Hann","Triangle",tr(fr,"Gaussienne","Gaussian")},{1200,562,265,34});
   active=button(tr(fr,"COUCHE ACTIVE","LAYER ACTIVE"),{1290,197,177,44},[this]{proc.set(lid(layer,"enabled"),proc.get(lid(layer,"enabled"))>.5f?0:1);updateActive();});updateActive();
   zoomButtons();const char* ids[]={"size","density","position","scatter","pitch","drift","spread","motion"};for(int i=0;i<8;++i)local(ids[i],67+i*176,671,124,111);
  }
  if(page==1){zoomButtons();sourceCombo({48,656,655,33});local("position",73,697,145,96);local("size",261,697,145,96);local("level",450,697,145,96);local("scatter",768,642,152,145);local("drift",940,642,152,145);local("spread",1112,642,152,145);local("density",1284,642,152,145);}
  if(page==2){global("osc1",90,372,160,139);global("oscPitch1",390,372,160,139);global("osc2",810,372,160,139);global("oscPitch2",1110,372,160,139);const char* ids[]={"fm","ring","attack","release"};for(int i=0;i<4;++i)global(ids[i],65+i*170,645,140,138);}
  if(page==3){global("lfoRate",95,602,210,175);global("lfoDepth",400,602,210,175);}
  if(page==4){
   auto* filters=button(tr(fr,"FILTRES DES COUCHES","LAYER FILTERS"),{26,198,210,41},[this]{rackPage=false;rebuild();});filters->setToggleState(!rackPage,dontSendNotification);
   auto* rack=button(tr(fr,"RACK 12 EFFETS","12-SLOT EFFECTS RACK"),{246,198,237,41},[this]{rackPage=true;rebuild();});rack->setToggleState(rackPage,dontSendNotification);
   if(!rackPage){
    combo("filterRouting",{tr(fr,"Série","Serial"),tr(fr,"Parallèle","Parallel")},{1300,267,167,34});
    for(int i=0;i<2;++i){int x=26+i*720;combo(i?"filterMode2":"filterMode",filterModes(),{x+21,411,660,33});local(i?"cutoff2":"cutoff",x+120,612,175,130);auto* q=local(i?"resonance2":"resonance",x+400,612,175,130);filterQ[i]=q;q->setEnabled(int(proc.get(lid(layer,i?"filterMode2":"filterMode")))%7!=0);}
    auto drive=std::make_unique<Slider>();drive->setSliderStyle(Slider::LinearHorizontal);drive->setTextBoxStyle(Slider::TextBoxRight,false,76,24);auto* raw=addControl(std::move(drive),{510,766,925,32});
    driveAttachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(proc.state,"drive",*raw);
   }else{
    for(int i=0;i<12;++i){int x=26+(i%4)*365,y=260+(i/4)*178;auto c=std::make_unique<ComboBox>();auto names=fxNames();for(int t=0;t<names.size();++t)c->addItem(names[t],t+1);auto* raw=addControl(std::move(c),{x+12,y+34,322,32});combos.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,fxid(i,"type"),*raw));
     auto b=std::make_unique<TextButton>("ON");b->setClickingTogglesState(true);auto* braw=addControl(std::move(b),{x+269,y+7,65,22});switches.push_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(proc.state,fxid(i,"enabled"),*braw));
     auto* amount=knob(fxid(i,"amount"),fxAmount(int(proc.get(fxid(i,"type")))),x+22,y+68,138,98);
     auto* mix=knob(fxid(i,"mix"),"Mix",x+180,y+68,138,98);
     fxAmountControls[i]=amount;fxMixControls[i]=mix;fxOnControls[i]=braw;
    }
   }
  }
  if(page==5){global("xyPos",830,348,200,175);global("xyDensity",1150,348,200,175);}
  if(page==6){global("velocity",785,278,140,118);global("attack",952,278,140,118);global("release",1119,278,140,118);global("master",1286,278,140,118);}
  if(page==7){
   const String names[]={tr(fr,"Tous les sons","All sounds"),tr(fr,"Favoris","Favorites"),catName(1),catName(2),catName(3),"Init"};
   for(int i=0;i<6;++i){auto* b=button(names[i],{47,277+i*47,253,38},[this,i]{category=i;refreshRows();});b->getProperties().set("left",true);}
   auto e=std::make_unique<TextEditor>();e->setTextToShowWhenEmpty(tr(fr,"Rechercher un nom ou une catégorie...","Search name or category..."),muted);e->onTextChange=[this]{refreshRows();};search=addControl(std::move(e),{342,198,980,42});
   button(tr(fr,"SON ALÉATOIRE","RANDOM SOUND"),{1334,198,133,42},[this]{loadPreset(Random::getSystemRandom().nextInt(8));});
   for(int i=0;i<8;++i){rows[i]=button("",{350,290+i*56,620,52},[this,i]{if(rowIds[i]>=0)loadPreset(rowIds[i]);});rows[i]->getProperties().set("left",true);stars[i]=button("",{1384,298+i*56,64,36},[this,i]{int n=rowIds[i];if(n<0)return;favorites[n]=!favorites[n];proc.state.state.setProperty("favorite"+String(n),favorites[n],nullptr);refreshRows();});}
   refreshRows();
  }
  resized();repaint();
 }
 void refreshRows(){if(page!=7||!search)return;auto q=search->getText().trim();rowIds.fill(-1);int r=0;for(int n=0;n<8;++n){bool keep=category==0||(category==1&&favorites[n])||(category>=2&&category<=4&&presetCategory(n)==category-1)||(category==5&&n==0);if(!keep||(!q.isEmpty()&&!String(presetNames()[n]).containsIgnoreCase(q)&&!catName(presetCategory(n)).containsIgnoreCase(q)))continue;rowIds[r++]=n;}for(int i=0;i<8;++i){int n=rowIds[i];rows[i]->setVisible(n>=0);stars[i]->setVisible(n>=0);if(n>=0){rows[i]->setButtonText(presetNames()[n]);rows[i]->setToggleState(n==current,dontSendNotification);stars[i]->setButtonText(favorites[n]?String::fromUTF8("★"):String::fromUTF8("☆"));stars[i]->setToggleState(favorites[n],dontSendNotification);}}repaint();}
 void loadPreset(int i){if(i<0||i>7)return;current=i;for(auto* param:proc.getParameters())param->setValueNotifyingHost(param->getDefaultValue());for(int j=0;j<4;++j){proc.set(lid(j,"enabled"),j==0||(i>4&&j==1)?1:0);proc.set(lid(j,"source"),float((i+j)%8));proc.set(lid(j,"size"),i==0?180.f:45.f+i*95.f);proc.set(lid(j,"density"),i==0?24.f:8.f+i*7.f);proc.set(lid(j,"pitch"),j==0?0.f:12.f);proc.set(lid(j,"cutoff"),9000.f-i*750.f);proc.set(lid(j,"position"),.15f+i*.09f);proc.set(lid(j,"level"),j==0?.65f:.32f);}proc.set("attack",.02f+i*.075f);proc.set("release",.9f+i*.2f);proc.set("lfoDepth",i*.085f);proc.set("osc1",.2f);proc.set("osc2",i*.035f);sound.setSelectedId(i+2,dontSendNotification);updateActive();refreshRows();repaint();}
 void updateActive(){if(active){bool on=proc.get(lid(layer,"enabled"))>.5f;active->setToggleState(on,dontSendNotification);active->setButtonText(on?tr(fr,"COUCHE ACTIVE","LAYER ACTIVE"):tr(fr,"COUCHE INACTIVE","LAYER INACTIVE"));}}
 void timerCallback()override{updateActive();if(page==4){if(rackPage){for(int i=0;i<12;++i)if(fxAmountControls[i]){int t=int(proc.get(fxid(i,"type")));fxAmountControls[i]->label.setText(fxAmount(t),dontSendNotification);fxAmountControls[i]->setEnabled(t!=0);fxMixControls[i]->setEnabled(t!=0);fxOnControls[i]->setEnabled(t!=0);}}else for(int i=0;i<2;++i)if(filterQ[i]){int m=int(proc.get(lid(layer,i?"filterMode2":"filterMode")));filterQ[i]->setEnabled(m!=0&&m!=7);}}if(page==0||page==2){std::array<float,512> a{};auto head=proc.analysisHead.load(std::memory_order_acquire);for(unsigned n=0;n<512;++n)a[n]=proc.analysis[(head+n)%512].load(std::memory_order_relaxed)*(.5f-.5f*std::cos(2*eg::pi*n/511.f));for(int k=0;k<64;++k){float re=0,im=0;int bin=1+k*4;for(int n=0;n<512;++n){float t=2*eg::pi*bin*n/512.f;re+=a[n]*std::cos(t);im+=a[n]*std::sin(t);}float db=Decibels::gainToDecibels(std::sqrt(re*re+im*im)/128.f,-80.f);spectrum[k]=jmax(jlimit(0.f,1.f,(db+80)/80),spectrum[k]*.8f);}}repaint();}
};
class Editor:public AudioProcessorEditor {
public:
 explicit Editor(Processor& p):AudioProcessorEditor(p),surface(p){addAndMakeVisible(surface);setResizable(true,true);setResizeLimits(930,620,1800,1200);if(auto* c=getConstrainer())c->setFixedAspectRatio(1.5);setSize(1050,700);}
 void resized()override{float scale=jmin(getWidth()/1500.f,getHeight()/1000.f);surface.setTransform(AffineTransform::scale(scale));surface.setTopLeftPosition(0,0);}
 void paint(Graphics& g)override{g.fillAll(bg);}
private:ReferenceSurface surface;
};
AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
