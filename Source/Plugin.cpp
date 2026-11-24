#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "GrainEngine.h"
#include "FactoryPresets.h"
#include "SpectralEngine.h"
#include <juce_audio_formats/juce_audio_formats.h>
using namespace juce;
namespace {
const Colour bg(0xff151719),panel(0xff25282b),gold(0xffc5a64a),champ(0xffd4bc76),ink(0xffe8e8e5),muted(0xff989c9f);
struct Spec {const char* id;const char* fr;const char* en;float lo,hi,def,step;};
const Spec layerSpecs[]={{"level","Niveau","Level",0,1,.65,.01},{"pan","Panoramique","Pan",-1,1,0,.01},{"size","Taille","Size",1,2000,180,1},{"density","Densité","Density",1,160,24,1},{"position","Position","Position",0,1,.35,.01},{"scatter","Dispersion","Scatter",0,1,.05,.01},{"pitch","Hauteur","Pitch",-36,36,0,1},{"drift","Dérive","Pitch drift",0,12,0,.01},{"spread","Largeur","Spread",0,1,.4,.01},{"motion","Mouvement","Movement",0,1,.04,.01},{"cutoff","Fréquence","Cutoff",20,20000,9000,1},{"cutoff2","Fréquence","Cutoff",20,20000,18000,1},{"resonance","Résonance","Resonance",.5,12,.707,.001},{"resonance2","Résonance","Resonance",.5,12,.707,.001},{"filterMode","Mode filtre 1","Filter 1 mode",0,7,7,1},{"filterMode2","Mode filtre 2","Filter 2 mode",0,7,0,1},{"filterRouting","Routage","Routing",0,1,0,1},{"source","Source","Source",0,7,0,1},{"direction","Direction","Direction",0,2,0,1},{"shape","Enveloppe","Envelope",0,2,0,1},{"enabled","Active","Enabled",0,1,1,1},{"sampleMode","Sample personnel","User sample",0,1,0,1},{"root","Note fondamentale","Root note",0,127,60,1},{"start","Début","Start",0,1,0,.001f},{"end","Fin","End",0,1,1,.001f},{"fadeIn","Fondu entrée","Fade in",0,.5f,0,.001f},{"fadeOut","Fondu sortie","Fade out",0,.5f,0,.001f},{"normalize","Normaliser","Normalize",0,1,0,1}};
const Spec globalSpecs[]={{"osc1","Niveau OSC 1","OSC 1 level",0,1,.2,.01},{"osc2","Niveau OSC 2","OSC 2 level",0,1,0,.01},{"oscPitch1","Hauteur OSC 1","OSC 1 pitch",-24,24,0,1},{"oscPitch2","Hauteur OSC 2","OSC 2 pitch",-24,24,0,1},{"fm","FM","FM",0,8,0,.01},{"ring","Anneau","Ring",0,1,0,.01},{"attack","Attaque","Attack",.001,3,.02,.001},{"release","Relâchement","Release",.01,5,.9,.01},{"master","Sortie master","Master output",-36,0,-8,.1},{"velocity","Vélocité","Velocity",0,1,.7,.01},{"lfoRate","Vitesse LFO","LFO rate",.01,20,.2,.01},{"lfoDepth","Profondeur LFO","LFO depth",0,1,0,.01},{"xyX","Position X","Position X",0,1,.5,.01},{"xyY","Densité Y","Density Y",0,1,.5,.01},{"xyPos","Quantité position","Position amount",0,1,0,.01},{"xyDensity","Quantité densité","Density amount",0,1,0,.01},{"drive","Saturation","Drive",0,1,0,.01},{"spectralMode","Mode spectral","Spectral mode",0,4,0,1},{"spectralMix","Mix spectral","Spectral mix",0,1,.65f,.01f},{"spectralFreeze","Intensité Freeze","Freeze amount",0,1,1,.01f},{"spectralBlur","Diffusion","Blur",0,1,.45f,.01f},{"spectralStretch","Étirement","Stretch",1,8,2,.01f},{"spectralMorph","Morphing A / B","Morphing A / B",0,1,.5f,.01f}};
static String fxid(int n,const char* param){return "fx"+String(n+1)+"_"+param;}
static String mid(const char* group,int i,const char* key){return String(group)+String(i+1)+"_"+key;}
static String sid(int i,int k){return mid("step",i,"value")+String(k+1);}
const char* macroNames[]={"Position","Cloud","Spread","Motion","Hybrid","Tone","Space","Heat"};
String lid(int layer,const char* id){return String::charToString(juce_wchar('A'+layer))+"_"+id;}
AudioProcessorValueTreeState::ParameterLayout layout(){AudioProcessorValueTreeState::ParameterLayout r;auto add=[&](String id,String name,float lo,float hi,float def,float step){r.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>(lo,hi,step),def));};for(int l=0;l<4;++l)for(auto& s:layerSpecs)add(lid(l,s.id),lid(l,s.en),s.lo,s.hi, String(s.id)=="enabled"?(l==0?1.f:0.f):s.def,s.step);for(auto& s:globalSpecs)add(s.id,s.en,s.lo,s.hi,s.def,s.step);for(int i=0;i<8;++i)add("macro"+String(i),macroNames[i],0,1,.5,.01);for(int i=0;i<12;++i){add(fxid(i,"type"),fxid(i,"type"),0,12,0,1);add(fxid(i,"enabled"),fxid(i,"enabled"),0,1,1,1);add(fxid(i,"amount"),fxid(i,"amount"),0,1,.5,.01);add(fxid(i,"mix"),fxid(i,"mix"),0,1,.5,.01);}for(int i=0;i<64;++i){add(mid("route",i,"source"),mid("route",i,"source"),0,eg::sourceN,0,1);add(mid("route",i,"dest"),mid("route",i,"dest"),0,eg::destN,0,1);add(mid("route",i,"depth"),mid("route",i,"depth"),-1,1,0,.001f);add(mid("route",i,"curve"),mid("route",i,"curve"),.25f,4,1,.01f);}
 for(int i=0;i<8;++i){add(mid("lfo",i,"rate"),mid("lfo",i,"rate"),.01f,20,.2f,.01f);add(mid("lfo",i,"phase"),mid("lfo",i,"phase"),0,1,0,.01f);add(mid("lfo",i,"shape"),mid("lfo",i,"shape"),0,5,0,1);add(mid("lfo",i,"sync"),mid("lfo",i,"sync"),0,1,0,1);for(auto k:{"attack","decay","release"})add(mid("env",i,k),mid("env",i,k),.001f,8,String(k)=="attack"?.05f:String(k)=="decay"?.3f:.6f,.001f);add(mid("env",i,"sustain"),mid("env",i,"sustain"),0,1,.7f,.01f);}
 for(int i=0;i<4;++i){add(mid("step",i,"rate"),mid("step",i,"rate"),.05f,32,4,.05f);add(mid("step",i,"glide"),mid("step",i,"glide"),0,1,0,.01f);add(mid("step",i,"length"),mid("step",i,"length"),1,64,16,1);add(mid("step",i,"sync"),mid("step",i,"sync"),0,1,0,1);for(int k=0;k<64;++k)add(sid(i,k),sid(i,k),-1,1,0,.01f);add(mid("function",i,"rate"),mid("function",i,"rate"),.01f,20,.5f,.01f);add(mid("function",i,"curve"),mid("function",i,"curve"),.25f,4,1,.01f);add(mid("function",i,"shape"),mid("function",i,"shape"),0,3,0,1);add(mid("function",i,"loop"),mid("function",i,"loop"),0,1,1,1);}
 return r;}
class Processor;
class Metal : public LookAndFeel_V4 {public:Metal(){setColour(Slider::textBoxTextColourId,champ);setColour(Slider::textBoxBackgroundColourId,bg);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);setColour(TextButton::buttonColourId,panel);setColour(TextButton::buttonOnColourId,gold.darker(.7f));setColour(TextButton::textColourOffId,ink);setColour(TextButton::textColourOnId,champ);}
void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float start,float end,Slider&)override{float r=std::min(w,h)*.39f,cx=x+w*.5f,cy=y+h*.5f;for(int i=0;i<=28;++i){float a=start+(end-start)*i/28;g.setColour(i<=pos*28?champ:muted.withAlpha(.35f));g.drawLine(cx+std::sin(a)*r*1.12f,cy-std::cos(a)*r*1.12f,cx+std::sin(a)*r*1.24f,cy-std::cos(a)*r*1.24f,1); }Rectangle<float> b(cx-r,cy-r,2*r,2*r);g.setColour(Colours::black.withAlpha(.6f));g.fillEllipse(b.translated(1,3));ColourGradient grad(Colour(0xff78683d),b.getX(),b.getY(),Colour(0xffb6a064),b.getRight(),b.getBottom(),false);grad.addColour(.32,Colour(0xffd8c68a));grad.addColour(.52,Colour(0xff81703e));grad.addColour(.74,Colour(0xffc6b477));g.setGradientFill(grad);g.fillEllipse(b);g.setColour(champ.withAlpha(.65f));g.drawEllipse(b,1);float a=start+(end-start)*pos;g.setColour(ink);g.drawLine(cx+std::sin(a)*r*.55f,cy-std::cos(a)*r*.55f,cx+std::sin(a)*r*.9f,cy-std::cos(a)*r*.9f,2);}};
class Processor: public AudioProcessor {public: Processor():AudioProcessor(BusesProperties().withOutput("Stereo",AudioChannelSet::stereo(),true)),state(*this,nullptr,"EG",layout()),engine(std::make_unique<eg::Engine>()){}
AudioProcessorValueTreeState state;std::unique_ptr<eg::Engine> engine;eg::SpectralEngine spectral;std::atomic<float> peak{0},peakR{0};std::array<std::atomic<float>,512> analysis{};std::atomic<unsigned> analysisHead{0};float get(String id)const{return state.getRawParameterValue(id)->load();}void set(String id,float x){if(auto* p=state.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(x));p->endChangeGesture();}}
const String getName()const override{return "Equilibrium Grain";}bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}double getTailLengthSeconds()const override{return 12;}int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const String getProgramName(int)override{return "Init";}void changeProgramName(int,const String&)override{}bool hasEditor()const override{return true;}AudioProcessorEditor* createEditor()override;void prepareToPlay(double r,int)override{engine->prepare(r);spectral.prepare(r);}void releaseResources()override{}bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainOutputChannelSet()==AudioChannelSet::stereo();}
void processBlock(AudioBuffer<float>& b,MidiBuffer& midi)override{ScopedNoDenormals nd;b.clear();if(b.getNumChannels()<2)return;eg::Settings s;for(int i=0;i<4;++i){auto& l=s.layers[i];auto v=[&](const char* id){return get(lid(i,id));};l.level=v("level");l.pan=v("pan");l.size=v("size");l.density=v("density");l.position=v("position");l.scatter=v("scatter");l.pitch=v("pitch");l.drift=v("drift");l.spread=v("spread");l.motion=v("motion");l.cutoff=v("cutoff");l.cutoff2=v("cutoff2");l.resonance=v("resonance");l.resonance2=v("resonance2");l.filterMode=int(v("filterMode"));l.filterMode2=int(v("filterMode2"));l.filterRouting=int(v("filterRouting"));l.source=int(v("source"));l.direction=int(v("direction"));l.shape=int(v("shape"));l.enabled=v("enabled")>.5f;l.sampleMode=v("sampleMode")>.5f;l.root=int(v("root"));l.start=v("start");l.end=v("end");l.fadeIn=v("fadeIn");l.fadeOut=v("fadeOut");l.normalize=v("normalize")>.5f;}
#define READ(x) s.x=get(#x)
READ(osc1);READ(osc2);READ(oscPitch1);READ(oscPitch2);READ(fm);READ(ring);READ(attack);READ(release);READ(master);READ(velocity);READ(lfoRate);READ(lfoDepth);READ(xyX);READ(xyY);READ(xyPos);READ(xyDensity);READ(drive);
#undef READ
for(int i=0;i<8;++i)s.macros[i]=get("macro"+String(i));for(int i=0;i<12;++i){auto& fx=s.effects[i];fx.type=int(get(fxid(i,"type")));fx.enabled=get(fxid(i,"enabled"))>.5f;fx.amount=get(fxid(i,"amount"));fx.mix=get(fxid(i,"mix"));}readMod(s.mod);if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())if(auto bpm=pos->getBpm())s.mod.bpm=float(*bpm);
 int cursor=0;for(auto m:midi){int end=jlimit(cursor,b.getNumSamples(),m.samplePosition);if(end>cursor)engine->render(b.getWritePointer(0,cursor),b.getWritePointer(1,cursor),end-cursor,s);auto msg=m.getMessage();if(msg.isNoteOn())engine->noteOn(msg.getNoteNumber(),msg.getFloatVelocity());else if(msg.isNoteOff())engine->noteOff(msg.getNoteNumber());else if(msg.isAllNotesOff()||msg.isAllSoundOff())engine->allOff();else if(msg.isController()&&msg.getControllerNumber()==1)engine->setWheel(msg.getControllerValue()/127.f);else if(msg.isChannelPressure())engine->setPressure(msg.getChannelPressureValue()/127.f);cursor=end;}if(cursor<b.getNumSamples())engine->render(b.getWritePointer(0,cursor),b.getWritePointer(1,cursor),b.getNumSamples()-cursor,s);eg::SpectralEngine::Params sp;sp.mode=int(get("spectralMode"));sp.mix=get("spectralMix");sp.freeze=get("spectralFreeze");sp.blur=get("spectralBlur");sp.stretch=get("spectralStretch");sp.morph=get("spectralMorph");spectral.process(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples(),sp);peak.store(jmax(b.getMagnitude(0,0,b.getNumSamples()),peak.load()*.85f));peakR.store(jmax(b.getMagnitude(1,0,b.getNumSamples()),peakR.load()*.85f));auto head=analysisHead.load(std::memory_order_relaxed);for(int k=0;k<b.getNumSamples();++k)analysis[(head++)%512].store(.5f*(b.getSample(0,k)+b.getSample(1,k)),std::memory_order_relaxed);analysisHead.store(head,std::memory_order_release);}
void readMod(eg::ModSettings& m){
 auto getp=[&](const char* group,int i,const char* k){return get(mid(group,i,k));};
 for(int i=0;i<64;++i)m.routes[i]={int(getp("route",i,"source")),int(getp("route",i,"dest")),getp("route",i,"depth"),getp("route",i,"curve")};
 for(int i=0;i<8;++i){m.lfos[i]={getp("lfo",i,"rate"),getp("lfo",i,"phase"),int(getp("lfo",i,"shape")),int(getp("lfo",i,"sync"))};m.envs[i]={getp("env",i,"attack"),getp("env",i,"decay"),getp("env",i,"sustain"),getp("env",i,"release")};}
 for(int i=0;i<4;++i){auto& s=m.steps[i];s.rate=getp("step",i,"rate");s.glide=getp("step",i,"glide");s.length=int(getp("step",i,"length"));s.sync=int(getp("step",i,"sync"));for(int k=0;k<64;++k)s.values[k]=get(sid(i,k));m.functions[i]={getp("function",i,"rate"),getp("function",i,"curve"),int(getp("function",i,"shape")),int(getp("function",i,"loop"))};}
 }
 String loadSample(int layer,const File& f,bool select=true){
  if(layer<0||layer>=4)return "Invalid layer";
  if(!f.existsAsFile()||!f.hasFileExtension("wav;wave;aif;aiff;aifc;flac"))return "WAV, AIFF or FLAC file required";
  AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<AudioFormatReader> reader(formats.createReaderFor(f));
  if(!reader)return "Unreadable audio file";
  if(reader->numChannels<1||reader->numChannels>2||reader->sampleRate<8000||reader->sampleRate>192000||reader->lengthInSamples<2||reader->lengthInSamples>reader->sampleRate*30.0)return "Mono/stereo, 8-192 kHz, 30 seconds maximum";
  int n=int(reader->lengthInSamples);AudioBuffer<float> buf(int(reader->numChannels),n);
  if(!reader->read(&buf,0,n,0,true,true))return "Audio decoding failed";
  auto s=std::make_unique<eg::SampleData>();s->rate=reader->sampleRate;s->name=f.getFileName().toStdString();s->path=f.getFullPathName().toStdString();s->left.resize(size_t(n));s->right.resize(size_t(n));s->peak=0;
  const float* l=buf.getReadPointer(0);const float* r=buf.getReadPointer(reader->numChannels==2?1:0);
  for(int i=0;i<n;++i){s->left[size_t(i)]=std::isfinite(l[i])?jlimit(-4.f,4.f,l[i]):0.f;s->right[size_t(i)]=std::isfinite(r[i])?jlimit(-4.f,4.f,r[i]):0.f;s->peak=jmax(s->peak,std::abs(s->left[size_t(i)]),std::abs(s->right[size_t(i)]));}
  if(!engine->samples.publish(layer,std::move(s)))return "Sample memory limit reached (128 MiB per instance)";
  state.state.setProperty(lid(layer,"samplePath"),f.getFullPathName(),nullptr);
  if(select){set(lid(layer,"sampleMode"),1);set(lid(layer,"enabled"),1);set(lid(layer,"start"),0);set(lid(layer,"end"),1);set(lid(layer,"root"),60);set(lid(layer,"fadeIn"),0);set(lid(layer,"fadeOut"),0);set(lid(layer,"normalize"),0);}
  return {};
 }
 void getStateInformation(MemoryBlock& b)override{if(auto x=state.copyState().createXml())copyXmlToBinary(*x,b);}
 void setStateInformation(const void* d,int n)override{
  if(auto x=getXmlFromBinary(d,n))if(x->hasTagName(state.state.getType())){
   state.replaceState(ValueTree::fromXml(*x));
   for(int j=0;j<4;++j){auto path=state.state.getProperty(lid(j,"samplePath")).toString();auto* old=engine->samples.get(j);if(old&&String::fromUTF8(old->path.c_str())==path)continue;engine->samples.clear(j);if(path.isNotEmpty())loadSample(j,File(path),false);}
  }
 }
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
class ReferenceSurface:public Component,public FileDragAndDropTarget,private Timer {
public:
 explicit ReferenceSurface(Processor& p):proc(p){
  setLookAndFeel(&look);setSize(1500,1000);setOpaque(true);
  for(int i=0;i<8;++i){auto& b=tabs[i];b.getProperties().set("tab",true);b.onClick=[this,i]{page=i;rebuild();};addAndMakeVisible(b);}
  for(int i=0;i<4;++i){auto& b=layers[i];b.onClick=[this,i]{layer=i;rebuild();};addAndMakeVisible(b);}
  language.setButtonText("FR / EN");language.onClick=[this]{fr=!fr;rebuild();};addAndMakeVisible(language);
  sound.addItem(tr(fr,"Son personnalisé","Custom sound"),1);for(int i=0;i<eg::factoryCount;++i)sound.addItem(eg::factory[i].name,i+2);sound.setSelectedId(1,dontSendNotification);sound.onChange=[this]{int n=sound.getSelectedId()-2;if(n>=0)loadPreset(n);};addAndMakeVisible(sound);
  previous.setButtonText("<");next.setButtonText(">");previous.onClick=[this]{loadPreset(current<=0?eg::factoryCount-1:current-1);};next.onClick=[this]{loadPreset((current+1)%eg::factoryCount);};addAndMakeVisible(previous);addAndMakeVisible(next);
  for(int i=0;i<8;++i){macros[i]=std::make_unique<Knob>(proc,"macro"+String(i),macroNames[i]);addAndMakeVisible(*macros[i]);if(i==6)macros[i]->slider.setTooltip("Space: global effects wet/dry");}
  master=std::make_unique<Knob>(proc,"master","Master");addAndMakeVisible(*master);
  for(int i=0;i<eg::factoryCount;++i)favorites[i]=bool(proc.state.state.getProperty("factoryFav_"+String(i),false));
  rebuild();startTimerHz(20);
 }
 ~ReferenceSurface()override{stopTimer();chooser.reset();clearPage();setLookAndFeel(nullptr);}
 bool isInterestedInFileDrag(const StringArray& files)override{return (page==0||page==1)&&files.size()==1&&File(files[0]).hasFileExtension("wav;wave;aif;aiff;aifc;flac");}
 void filesDropped(const StringArray& files,int,int)override{if(isInterestedInFileDrag(files))importSample(File(files[0]),layer);}

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
  text(g,tr(fr,"SOURCES INTERNES ET SAMPLES  /  MIDI POLYPHONIQUE  /  V0.4","BUILT-IN AND USER SAMPLES  /  MIDI POLYPHONIC  /  V0.4"),26,978,700,15,10,muted,true);
  text(g,"SHAPE SOUND BEYOND REALITY.",1110,978,365,15,10,muted,true,Justification::centredRight);
  for(auto pt:{Point<float>(9,9),Point<float>(1490,9),Point<float>(9,990),Point<float>(1490,990)}){g.setColour(Colour(0xff62675b));g.drawEllipse(pt.x-3,pt.y-3,6,6,1);g.drawLine(pt.x-1.5f,pt.y+1.5f,pt.x+1.5f,pt.y-1.5f,1);}
 }
 void mouseDown(const MouseEvent& e)override{mouseDrag(e);}
 void mouseDrag(const MouseEvent& e)override{
  auto p=e.position;
  if((page==0||page==1)&&waveRect().contains(p)){auto r=waveRect();auto range=waveRange();proc.set(lid(layer,"position"),jlimit(0.f,1.f,range.first+(p.x-r.getX())/r.getWidth()*range.second));}
  if((page==5||page==6)&&xyRect().contains(p)){auto r=xyRect();proc.set("xyX",jlimit(0.f,1.f,(p.x-r.getX())/r.getWidth()));proc.set("xyY",jlimit(0.f,1.f,1-(p.y-r.getY())/r.getHeight()));}
  if(page==4&&!rackPage&&!spectralPage)for(int i=0;i<2;++i){auto r=filterRect(i);if(r.contains(p))proc.set(lid(layer,i?"cutoff2":"cutoff"),20.f*std::pow(1000.f,jlimit(0.f,1.f,(p.x-r.getX())/r.getWidth())));}
 }
private:
 Processor& proc;ReferenceLook look;bool fr=true;int page=0,layer=0,current=-1,zoom=1,category=0;bool rackPage=false,spectralPage=false;int modPage=0,modBank=0,modUnit=0,stepBank=0;std::array<bool,eg::factoryCount> favorites{};int browserPage=0,matchCount=0;std::unique_ptr<FileChooser> chooser;String sampleStatus;TextButton* pageLabel=nullptr;std::array<TextButton*,13> categoryButtons{};
 std::array<TextButton,8> tabs;std::array<TextButton,4> layers;TextButton language,previous,next;ComboBox sound;
 std::array<std::unique_ptr<Knob>,8> macros;std::unique_ptr<Knob> master;
 std::vector<std::unique_ptr<Component>> controls;std::vector<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>> combos;std::vector<std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>> switches;
 std::array<TextButton*,8> rows{},stars{};std::array<int,8> rowIds{};TextEditor* search=nullptr;TextButton* active=nullptr;std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> driveAttachment;std::array<Knob*,12> fxAmountControls{},fxMixControls{};std::array<TextButton*,12> fxOnControls{};std::array<Knob*,2> filterQ{};
 std::array<float,64> spectrum{};
 static const char* const* sources(){static const char* n[]={"Velvet","Silk","Glass","Amber","Mist","Crystal","Dust","Metal"};return n;}
 int presetCategory(int n)const{return eg::factory[n].category;}
 String catName(int n)const{const char* f[]={"Init","Pads atmosphériques","Textures cinématiques","Leads granulaires","Basses expérimentales","Grains rythmiques","Paysages ambiants","Instruments organiques","Glitch et expérimental","Textures vocales","FX et transitions"};const char* e[]={"Init","Atmospheric Pads","Cinematic Textures","Granular Leads","Experimental Basses","Rhythmic Grains","Ambient Soundscapes","Organic Instruments","Glitch & Experimental","Vocal Textures","Sound FX & Transitions"};return tr(fr,f[jlimit(0,10,n)],e[jlimit(0,10,n)]);}
 bool personal()const{return proc.get(lid(layer,"sampleMode"))>.5f;}
 void importSample(const File& f,int target){auto error=proc.loadSample(target,f);sampleStatus=error.isEmpty()?tr(fr,"Sample chargé","Sample loaded"):tr(fr,"Import impossible : ","Import failed: ")+error;if(target==layer)rebuild();else repaint();}
 void chooseSample(){if(chooser)return;int target=layer;chooser=std::make_unique<FileChooser>(tr(fr,"Importer un sample","Import sample"),File{},"*.wav;*.wave;*.aif;*.aiff;*.aifc;*.flac");Component::SafePointer<ReferenceSurface> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe,target](const FileChooser& c){if(safe!=nullptr){auto f=c.getResult();MessageManager::callAsync([safe,target,f]{if(safe!=nullptr){safe->chooser.reset();if(f.existsAsFile())safe->importSample(f,target);}});}});}
 void sampleToggle(String id,String label,Rectangle<int> r){auto p=std::make_unique<TextButton>(label);p->setClickingTogglesState(true);auto* raw=addControl(std::move(p),r);switches.push_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(proc.state,lid(layer,id.toRawUTF8()),*raw));}
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
  panelBox(g,top.getUnion(r).getUnion(base));int source=jlimit(0,7,int(proc.get(lid(layer,"source"))));bool user=personal();const auto* imported=user?proc.engine->samples.get(layer):nullptr;double duration=imported?double(imported->frames())/imported->rate:1.0;
  text(g,user?(imported?String::fromUTF8(imported->name.c_str()):tr(fr,"Sample manquant","Missing sample")):String(sources()[source]),int(top.getX())+16,int(top.getY())+7,490,28,17,champ,true);text(g,(user?tr(fr,"SAMPLE PERSONNEL / COUCHE ","USER SAMPLE / LAYER "):tr(fr,"SOURCE INTERNE / COUCHE ","BUILT-IN SOURCE / LAYER "))+String::charToString(juce_wchar('A'+layer)),int(top.getX())+520,int(top.getY())+10,300,24,11,muted,true);
  g.setColour(Colour(0xff1b211d));g.fillRect(r);
  auto range=waveRange();for(int i=0;i<=16;++i){float x=r.getX()+i*r.getWidth()/16;g.setColour(Colour(0xff333d32));g.drawVerticalLine(int(x),r.getY(),r.getBottom());if(i%4==0&&i<16)text(g,String((range.first+range.second*i/16)*duration,3)+" s",int(x)+6,int(r.getY())+5,90,16,12,muted);}
  g.setColour(Colour(0xff343b32));g.drawHorizontalLine(int(r.getCentreY()),r.getX(),r.getRight());
  auto& src=proc.engine->sources[source];int count=int(r.getWidth());g.setColour(champ.withAlpha(.86f));
  if(!user||imported){size_t frames=imported?imported->frames():src.size();auto val=[&](size_t k){return imported?(std::abs(imported->left[k])>=std::abs(imported->right[k])?imported->left[k]:imported->right[k]):src[k];};
   for(int i=0;i<count;++i){float t0=range.first+range.second*float(i)/count,t1=range.first+range.second*float(i+1)/count;size_t begin=std::min(frames-1,size_t(t0*frames)),end=std::min(frames,std::max(begin+1,size_t(t1*frames)));float lo=0,hi=0;size_t step=std::max<size_t>(1,(end-begin)/64);for(size_t k=begin;k<end;k+=step){float x=val(k);lo=std::min(lo,x);hi=std::max(hi,x);}g.drawVerticalLine(int(r.getX())+i,r.getCentreY()-jlimit(-1.f,1.f,hi)*r.getHeight()*.42f,r.getCentreY()-jlimit(-1.f,1.f,lo)*r.getHeight()*.42f);}
  }
  if(imported){float st=proc.get(lid(layer,"start")),en=proc.get(lid(layer,"end"));g.setColour(champ.withAlpha(.6f));for(float v:{st,en}){float x=r.getX()+r.getWidth()*(v-range.first)/range.second;if(x>=r.getX()&&x<=r.getRight())g.drawVerticalLine(int(x),r.getY(),r.getBottom());}}
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
  wave(g);panelBox(g,{26,568,700,228});panelTitle(g,47,583,660,"SOURCE LAB",tr(fr,"Région et préparation","Region & preparation"));note(g,sampleStatus.isNotEmpty()?sampleStatus:tr(fr,"WAV / AIFF / FLAC : glisser ou importer (30 s max.)","WAV / AIFF / FLAC: drag or import (30 s max.)"),48,634,650);
  panelBox(g,{747,568,720,228});panelTitle(g,768,583,670,tr(fr,"VARIATION PAR GRAIN","PER-GRAIN VARIATION"),tr(fr,"Variation des grains","Grain variation"));
 }
 void paintSynth(Graphics& g){
  for(int i=0;i<2;++i){int x=26+i*720;panelBox(g,{float(x),198,702,352});panelTitle(g,x+21,214,660,tr(fr,"SYNTHÈSE HYBRIDE","HYBRID SYNTHESIS"),"Oscillator 0"+String(i+1));note(g,tr(fr,"Sinusoïde / accord indépendant","Sine / independent tuning"),x+21,278,660);g.setColour(gold.withAlpha(.8f));Path p;for(int k=0;k<630;++k){float yy=338.f+13.f*std::sin(k*.028f);if(k==0)p.startNewSubPath(float(x+35),yy);else p.lineTo(float(x+35+k),yy);}g.strokePath(p,PathStrokeType(1.2f));note(g,tr(fr,"Autres formes et unisson : non implémentés","Other waveforms and unison: not implemented"),x+21,522,660);}
  panelBox(g,{26,568,702,228});panelTitle(g,47,583,660,tr(fr,"MODULATION ET MIXAGE","MODULATION & MIX"),tr(fr,"Interaction hybride","Hybrid interaction"));
  panelBox(g,{746,568,721,228});panelTitle(g,767,583,680,tr(fr,"SIGNAL DE SORTIE","OUTPUT AUDIO"),tr(fr,"Analyse de sortie","Output analysis"));drawSpectrum(g,{768,655,677,121});
 }
 StringArray sourceNames()const{
  StringArray s{tr(fr,"Aucune","None")};for(int i=1;i<=8;++i)s.add("LFO "+String(i));for(int i=1;i<=8;++i)s.add(tr(fr,"Enveloppe ","Envelope ")+String(i));for(int i=1;i<=4;++i)s.add("Step "+String(i));for(int i=1;i<=4;++i)s.add(tr(fr,"Fonction ","Function ")+String(i));s.add(tr(fr,"Vélocité","Velocity"));s.add(tr(fr,"Note MIDI","MIDI note"));s.add("Mod Wheel");s.add("Aftertouch");s.add("XY X");s.add("XY Y");return s;
 }
 StringArray destNames()const{
  StringArray s{tr(fr,"Aucune","None")};const char* f[]={"Position","Taille","Densité","Hauteur","Dispersion","Dérive","Largeur","Panoramique","Niveau","Fréquence F1","Fréquence F2","Résonance F1","Résonance F2"};const char* e[]={"Position","Size","Density","Pitch","Scatter","Drift","Spread","Pan","Level","Cutoff F1","Cutoff F2","Resonance F1","Resonance F2"};for(int j=0;j<4;++j)for(int k=0;k<13;++k)s.add(String::charToString(juce_wchar('A'+j))+" / "+tr(fr,f[k],e[k]));for(auto n:{"OSC 1","OSC 2","FM","Ring"})s.add(n);s.add(tr(fr,"Attaque","Attack"));s.add(tr(fr,"Relâchement","Release"));s.add("Drive");for(int i=1;i<=12;++i){s.add("FX "+String(i)+" / Amount");s.add("FX "+String(i)+" / Mix");}return s;
 }
 void paintMod(Graphics& g){
  g.setColour(Colour(0xff485044));g.drawHorizontalLine(245,26,1467);
  if(modPage==0){g.setColour(Colour(0xff303b2c));g.fillRect(26,301,1441,35);text(g,"SLOT",40,305,65,25,11,muted,true);text(g,tr(fr,"SOURCE DE MODULATION","MODULATION SOURCE"),110,305,330,25,12,muted,true);text(g,"DESTINATION",510,305,420,25,12,muted,true);text(g,tr(fr,"PROFONDEUR BIPOLAIRE","BIPOLAR DEPTH"),940,305,350,25,12,muted,true);text(g,tr(fr,"COURBE","CURVE"),1330,305,125,25,12,muted,true);
   for(int i=0;i<8;++i){int y=336+i*57;g.setColour(i%2?Colour(0xff222b22):Colour(0xff1c251e));g.fillRect(26,y,1441,56);text(g,String(modBank*8+i+1).paddedLeft('0',2),40,y+12,56,32,15,champ,true);text(g,String::fromUTF8("→"),461,y+12,42,32,20,champ);}
   return;
  }
  panelBox(g,{26,307,1441,488});String heading=modPage==1?"LFO ":modPage==2?tr(fr,"Enveloppe ","Envelope "):modPage==3?"Step ":tr(fr,"Fonction ","Function ");
  panelTitle(g,47,319,1400,tr(fr,"SOURCE DE MODULATION","MODULATION SOURCE"),heading+String(modUnit+1));
  if(modPage==3){text(g,tr(fr,"Valeurs bipolaires / 64 pas maximum","Bipolar values / up to 64 steps"),50,384,750,22,15,muted);return;}
  auto r=Rectangle<float>(48,390,1397,221);g.setColour(Colour(0xff19211e));g.fillRect(r);for(int k=0;k<=8;++k){g.setColour(Colour(0xff344036));g.drawVerticalLine(int(r.getX()+r.getWidth()*k/8),r.getY(),r.getBottom());}g.drawHorizontalLine(int(r.getCentreY()),r.getX(),r.getRight());Path p;
  for(int k=0;k<1397;++k){float t=k/1396.f,v=0;if(modPage==1)v=eg::waveMod(int(proc.get(mid("lfo",modUnit,"shape"))),t+proc.get(mid("lfo",modUnit,"phase")));else if(modPage==4)v=2*eg::functionWave(int(proc.get(mid("function",modUnit,"shape"))),t,proc.get(mid("function",modUnit,"curve")))-1;else{float a=proc.get(mid("env",modUnit,"attack")),d=proc.get(mid("env",modUnit,"decay")),s=proc.get(mid("env",modUnit,"sustain")),rel=proc.get(mid("env",modUnit,"release"));float hold=.5f,total=a+d+hold+rel,x=t*total;float env=x<a?x/a:x<a+d?1-(1-s)*(x-a)/d:x<a+d+hold?s:s*std::max(0.f,1-(x-a-d-hold)/rel);v=2*env-1;}float yy=r.getCentreY()-v*r.getHeight()*.42f;if(k==0)p.startNewSubPath(r.getX(),yy);else p.lineTo(r.getX()+k,yy);}g.setColour(champ);g.strokePath(p,PathStrokeType(2));
 }
 void modCombo(String id,const StringArray& names,Rectangle<int> bounds){auto c=std::make_unique<ComboBox>();for(int i=0;i<names.size();++i)c->addItem(names[i],i+1);auto* raw=addControl(std::move(c),bounds);combos.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,id,*raw));}
 void modSlider(String id,Rectangle<int> r,bool vert=false){auto p=std::make_unique<Slider>();p->setSliderStyle(vert?Slider::LinearVertical:Slider::LinearHorizontal);p->setTextBoxStyle(vert?Slider::TextBoxBelow:Slider::TextBoxRight,false,vert?57:63,21);p->setColour(Slider::trackColourId,gold);p->setColour(Slider::thumbColourId,champ);auto* raw=addControl(std::move(p),r);modSliders.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(proc.state,id,*raw));}
 void buildMod(){
  String labels[]={tr(fr,"MATRICE 64 ROUTES","64-SLOT MATRIX"),"8 LFO",tr(fr,"8 ENVELOPPES","8 ENVELOPES"),"4 STEP",tr(fr,"4 FONCTIONS","4 FUNCTIONS")};
  for(int i=0;i<5;++i){auto* b=button(labels[i],{26+i*199,198,190,39},[this,i]{modPage=i;modUnit=0;stepBank=0;rebuild();});b->setToggleState(i==modPage,dontSendNotification);}
  if(modPage==0){for(int i=0;i<8;++i){auto* b=button(String(i*8+1)+" - "+String(i*8+8),{26+i*180,253,170,39},[this,i]{modBank=i;rebuild();});b->setToggleState(i==modBank,dontSendNotification);}
   auto sn=sourceNames(),dn=destNames();for(int i=0;i<8;++i){int n=modBank*8+i,y=336+i*57;modCombo(mid("route",n,"source"),sn,{110,y+9,340,37});modCombo(mid("route",n,"dest"),dn,{510,y+9,420,37});modSlider(mid("route",n,"depth"),{940,y+8,355,40});modSlider(mid("route",n,"curve"),{1301,y+8,151,40});}return;}
  int num=modPage<=2?8:4;for(int i=0;i<num;++i){auto* b=button(String(i+1),{26+i*112,253,103,39},[this,i]{modUnit=i;rebuild();});b->setToggleState(i==modUnit,dontSendNotification);}
  if(modPage==1){modCombo(mid("lfo",modUnit,"shape"),{tr(fr,"Sinusoïde","Sine"),"Triangle",tr(fr,"Dent de scie","Saw"),tr(fr,"Carré","Square"),"Sample & Hold",tr(fr,"Harmonique","Harmonic")},{48,640,335,36});modCombo(mid("lfo",modUnit,"sync"),{tr(fr,"Libre (Hz)","Free (Hz)"),tr(fr,"Tempo (cycles / temps)","Tempo (cycles / beat)")},{48,692,335,36});knob(mid("lfo",modUnit,"rate"),tr(fr,"Vitesse","Rate"),450,630,230,150);knob(mid("lfo",modUnit,"phase"),"Phase",730,630,230,150);if(modUnit==0){global("lfoRate",1000,630,200,150);global("lfoDepth",1220,630,200,150);}}
  if(modPage==2){const char* keys[]={"attack","decay","sustain","release"};const char* f[]={"Attaque","Déclin","Maintien","Relâchement"};for(int i=0;i<4;++i)knob(mid("env",modUnit,keys[i]),tr(fr,f[i],keys[i]),140+i*330,628,210,155);}
  if(modPage==3){for(int i=0;i<4;++i){auto* b=button(String(i*16+1)+" - "+String(i*16+16),{965+i*122,253,115,39},[this,i]{stepBank=i;rebuild();});b->setToggleState(i==stepBank,dontSendNotification);}for(int i=0;i<16;++i){int k=stepBank*16+i;modSlider(sid(modUnit,k),{48+i*87,415,77,228},true);}
   knob(mid("step",modUnit,"length"),tr(fr,"Nombre de pas","Length"),110,651,170,135);knob(mid("step",modUnit,"rate"),tr(fr,"Vitesse","Rate"),330,651,170,135);knob(mid("step",modUnit,"glide"),"Glide",550,651,170,135);modCombo(mid("step",modUnit,"sync"),{tr(fr,"Libre","Free"),"Tempo"},{775,684,260,38});button(tr(fr,"ALÉATOIRE","RANDOM"),{1090,684,170,38},[this]{for(int k=0;k<64;++k)proc.set(sid(modUnit,k),Random::getSystemRandom().nextFloat()*2-1);});button("RESET",{1270,684,175,38},[this]{for(int k=0;k<64;++k)proc.set(sid(modUnit,k),0);});}
  if(modPage==4){modCombo(mid("function",modUnit,"shape"),{tr(fr,"Montée","Rise"),tr(fr,"Descente","Fall"),"Triangle",tr(fr,"Courbe douce","Smooth curve")},{48,650,340,38});modCombo(mid("function",modUnit,"loop"),{"One-shot",tr(fr,"Boucle","Loop")},{48,704,340,38});knob(mid("function",modUnit,"rate"),tr(fr,"Vitesse","Rate"),500,630,240,150);knob(mid("function",modUnit,"curve"),tr(fr,"Courbe","Curve"),850,630,240,150);}
 }
 StringArray filterModes()const{return {tr(fr,"Bypass","Bypass"),tr(fr,"Passe-bas 12 dB","Low-pass 12 dB"),tr(fr,"Passe-haut 12 dB","High-pass 12 dB"),tr(fr,"Passe-bande","Band-pass"),tr(fr,"Coupe-bande","Notch"),tr(fr,"Passe-bas 24 dB","Low-pass 24 dB"),tr(fr,"Passe-haut 24 dB","High-pass 24 dB"),tr(fr,"Passe-bas 6 dB (original)","Low-pass 6 dB (original)")};}
 StringArray fxNames()const{return {tr(fr,"Vide","Empty"),"Saturation","Chorus","Delay",tr(fr,"Réverbération","Reverb"),"EQ",tr(fr,"Compresseur","Compressor"),"Phaser","Flanger","Bitcrusher",tr(fr,"Décalage fréquentiel","Frequency Shifter"),tr(fr,"Image stéréo","Stereo Imager"),tr(fr,"Limiteur","Limiter")};}
 String fxAmount(int t)const{switch(t){case 1:return "Drive";case 2:case 7:case 8:return tr(fr,"Mouvement","Motion");case 3:return tr(fr,"Temps","Time");case 4:return tr(fr,"Durée","Decay");case 5:return tr(fr,"Tonalité","Tone");case 6:return tr(fr,"Seuil","Threshold");case 9:return tr(fr,"Réduction","Reduction");case 10:return tr(fr,"Décalage","Shift");case 11:return tr(fr,"Largeur","Width");case 12:return tr(fr,"Plafond","Ceiling");default:return tr(fr,"Quantité","Amount");}}
 void paintSpectral(Graphics& g){
  text(g,"FFT 1024 / HOP 256 / STEREO",845,207,622,26,12,muted,true,Justification::centredRight);
  panelBox(g,{26,260,1441,278});panelTitle(g,47,274,900,tr(fr,"ANALYSE ET RESYNTHÈSE","ANALYSIS AND RESYNTHESIS"),tr(fr,"Spectre avant / après traitement","Input / processed spectrum"));
  auto r=Rectangle<float>(48,329,1397,184);g.setColour(Colour(0xff19211e));g.fillRect(r);g.setColour(Colour(0xff43503d));g.drawRect(r,1.f);
  for(int i=1;i<6;++i){g.setColour(Colour(0xff344036));g.drawHorizontalLine(int(r.getY()+i*r.getHeight()/6),r.getX(),r.getRight());}
  for(int i=1;i<16;++i){g.setColour(Colour(0xff344036));g.drawVerticalLine(int(r.getX()+i*r.getWidth()/16),r.getY(),r.getBottom());}
  auto draw=[&](const auto& data,Colour colour){Path p;for(int i=0;i<eg::SpectralEngine::Display;++i){float x=r.getX()+r.getWidth()*i/(eg::SpectralEngine::Display-1);float db=Decibels::gainToDecibels(data[i].load(std::memory_order_relaxed),-80.f);float y=r.getBottom()-jlimit(0.f,1.f,(db+80.f)/80.f)*r.getHeight();if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);}g.setColour(colour);g.strokePath(p,PathStrokeType(1.8f));};
  draw(proc.spectral.inputView,muted.withAlpha(.8f));draw(proc.spectral.outputView,champ);
  text(g,tr(fr,"ENTRÉE","INPUT"),1030,282,155,25,12,muted,true);text(g,tr(fr,"TRAITÉ","PROCESSED"),1225,282,185,25,12,champ,true);
  panelBox(g,{26,552,1441,244});panelTitle(g,47,568,800,tr(fr,"TRAITEMENT DE SORTIE","MASTER PROCESSING"),tr(fr,"Architecture spectrale","Spectral architecture"));
  const int flags=proc.spectral.captured.load();text(g,(flags&1)?tr(fr,"A : capturé","A: captured"):tr(fr,"A : vide","A: empty"),990,573,125,21,13,(flags&1)?champ:muted,true);text(g,(flags&2)?tr(fr,"B : capturé","B: captured"):tr(fr,"B : vide","B: empty"),1120,573,125,21,13,(flags&2)?champ:muted,true);text(g,(flags&4)?tr(fr,"Freeze : actif","Freeze: captured"):tr(fr,"Freeze : vide","Freeze: empty"),1250,573,198,21,13,(flags&4)?champ:muted,true);
  note(g,tr(fr,"Captures en mémoire de session. Stretch : historique spectral roulant.","Session captures. Stretch: rolling spectral history."),48,773,1380);
 }
 void buildSpectral(){
  modCombo("spectralMode",{tr(fr,"Bypass","Bypass"),"Spectral Freeze","Spectral Blur","Spectral Stretch","Spectral Morphing"},{48,622,310,36});
  button(tr(fr,"FIGER / RECAPTURER","FREEZE / RECAPTURE"),{48,670,310,34},[this]{proc.set("spectralMode",1);proc.spectral.request(eg::SpectralEngine::RecaptureFreeze);});
  button(tr(fr,"CAPTURER A","CAPTURE A"),{48,715,100,35},[this]{proc.spectral.request(eg::SpectralEngine::CaptureA);});
  button(tr(fr,"CAPTURER B","CAPTURE B"),{155,715,100,35},[this]{proc.spectral.request(eg::SpectralEngine::CaptureB);});
  button(tr(fr,"EFFACER","CLEAR"),{262,715,96,35},[this]{proc.spectral.request(eg::SpectralEngine::Clear);});
  const char* ids[]={"spectralMix","spectralFreeze","spectralBlur","spectralStretch","spectralMorph"};
  for(int i=0;i<5;++i){auto* k=global(ids[i],385+i*210,615,195,145);if(i==3&&k)k->slider.setTextValueSuffix(" x");}
 }
 void paintFilters(Graphics& g){
  g.setColour(Colour(0xff485044));g.drawHorizontalLine(247,26,1467);
  if(spectralPage){paintSpectral(g);return;}
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
  panelBox(g,{26,198,295,600});panelTitle(g,47,214,253,"LIBERTY SOUND LIBRARY",tr(fr,"Trouvez votre équilibre.","Find your equilibrium."));note(g,tr(fr,"100 sons + Init","100 sounds + Init"),47,761,252);
  g.setColour(Colour(0xff303b2c));g.fillRect(342,251,1125,37);text(g,tr(fr,"NOM DU SON","SOUND NAME"),360,257,500,24,11,muted,true);text(g,tr(fr,"CATÉGORIE","CATEGORY"),1003,257,260,24,11,muted,true);text(g,tr(fr,"FAVORI","FAVORITE"),1380,257,72,24,11,muted,true);
  for(int i=0;i<8;++i){int n=rowIds[i];if(n<0)continue;auto y=289+i*56;g.setColour(i%2?Colour(0xff222b22):Colour(0xff1c251e));g.fillRect(342,y,1125,54);text(g,catName(presetCategory(n)),1003,y+3,355,25,14,muted);text(g,eg::factory[n].tag,1003,y+28,355,21,11,muted);}
  note(g,tr(fr,"Banque d’usine protégée / 100 sons originaux","Protected factory library / 100 original sounds"),344,773,650);
 }
 template<class T> T* addControl(std::unique_ptr<T> p,Rectangle<int> bounds){auto* raw=p.get();addAndMakeVisible(*raw);raw->setBounds(bounds);controls.push_back(std::move(p));return raw;}
 TextButton* button(String name,Rectangle<int> r,std::function<void()> fn){auto p=std::make_unique<TextButton>(name);p->onClick=std::move(fn);return addControl(std::move(p),r);}
 Knob* knob(String id,String title,int x,int y,int w,int h){return addControl(std::make_unique<Knob>(proc,id,title),{x,y,w,h});}
 Knob* local(const char* id,int x,int y,int w,int h){for(const auto& s:layerSpecs)if(String(s.id)==id)return knob(lid(layer,id),tr(fr,s.fr,s.en),x,y,w,h);return nullptr;}
 Knob* global(const char* id,int x,int y,int w,int h){for(const auto& s:globalSpecs)if(String(s.id)==id)return knob(id,tr(fr,s.fr,s.en),x,y,w,h);return nullptr;}
 void combo(const char* id,StringArray values,Rectangle<int> r){auto b=std::make_unique<ComboBox>();for(int i=0;i<values.size();++i)b->addItem(values[i],i+1);auto* raw=addControl(std::move(b),r);combos.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,lid(layer,id),*raw));}
 void sourceCombo(Rectangle<int> r){StringArray v;for(int i=0;i<8;++i)v.add(sources()[i]);combo("source",v,r);}
 void zoomButtons(){auto r=waveRect();int x=int(r.getRight());button("-",{x-112,267,27,24},[this]{zoom=jmax(1,zoom/2);repaint();});button("+",{x-39,267,27,24},[this]{zoom=jmin(8,zoom*2);repaint();});}
 std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> modSliders;
 void clearPage(){pageLabel=nullptr;categoryButtons.fill(nullptr);modSliders.clear();driveAttachment.reset();fxAmountControls.fill(nullptr);fxMixControls.fill(nullptr);fxOnControls.fill(nullptr);filterQ.fill(nullptr);switches.clear();combos.clear();controls.clear();rows.fill(nullptr);stars.fill(nullptr);rowIds.fill(-1);active=nullptr;search=nullptr;}
 void rebuild(){
  clearPage();const char* f[]={"MOTEUR GRANULAIRE","SAMPLE LAB","SYNTHÈSE","MODULATION","FILTRES & EFFETS","MOUVEMENT & SÉQUENCEUR","PERFORMANCE","PRESETS"};const char* e[]={"GRANULAR ENGINE","SAMPLE LAB","SYNTHESIS","MODULATION","FILTERS & EFFECTS","MOTION & SEQUENCER","PERFORMANCE","PRESET BROWSER"};
  sound.changeItemText(1,tr(fr,"Son personnalisé","Custom sound"));
  for(int i=0;i<8;++i){tabs[i].setButtonText(String(i+1).paddedLeft('0',2)+"  "+tr(fr,f[i],e[i]));tabs[i].setToggleState(i==page,dontSendNotification);}
  for(int i=0;i<4;++i){layers[i].setVisible(page==0||page==1||page==4&&!rackPage&&!spectralPage);layers[i].setBounds(26+i*91,page==4?263:198,84,41);layers[i].setButtonText(String::charToString(juce_wchar('A'+i))+"  "+tr(fr,"COUCHE","LAYER"));layers[i].setToggleState(i==layer,dontSendNotification);}
  if(page==0){
   sourceCombo({43,342,217,35});sampleToggle("sampleMode",tr(fr,"SAMPLE PERSO","USER SAMPLE"),{43,503,217,34});local("level",50,386,100,111);local("pan",156,386,100,111);
   combo("direction",{tr(fr,"Avant","Forward"),tr(fr,"Arrière","Reverse"),tr(fr,"Aléatoire","Random")},{43,574,217,31});
   combo("shape",{"Hann","Triangle",tr(fr,"Gaussienne","Gaussian")},{1200,562,265,34});
   active=button(tr(fr,"COUCHE ACTIVE","LAYER ACTIVE"),{1290,197,177,44},[this]{proc.set(lid(layer,"enabled"),proc.get(lid(layer,"enabled"))>.5f?0:1);updateActive();});updateActive();
   zoomButtons();const char* ids[]={"size","density","position","scatter","pitch","drift","spread","motion"};for(int i=0;i<8;++i)local(ids[i],67+i*176,671,124,111);
  }
  if(page==1){zoomButtons();button(tr(fr,"IMPORTER","IMPORT"),{1060,198,170,41},[this]{chooseSample();});sampleToggle("sampleMode",tr(fr,"SAMPLE PERSONNEL","USER SAMPLE"),{1240,198,225,41});sourceCombo({48,656,415,33});sampleToggle("normalize",tr(fr,"NORMALISER","NORMALIZE"),{473,656,230,33});const char* ids[]={"start","end","fadeIn","fadeOut","root"};for(int i=0;i<5;++i)local(ids[i],49+i*131,695,124,98);local("scatter",768,642,152,145);local("drift",940,642,152,145);local("spread",1112,642,152,145);local("density",1284,642,152,145);}
  if(page==2){global("osc1",90,372,160,139);global("oscPitch1",390,372,160,139);global("osc2",810,372,160,139);global("oscPitch2",1110,372,160,139);const char* ids[]={"fm","ring","attack","release"};for(int i=0;i<4;++i)global(ids[i],65+i*170,645,140,138);}
  if(page==3)buildMod();
  if(page==4){
   auto* filters=button(tr(fr,"FILTRES DES COUCHES","LAYER FILTERS"),{26,198,210,41},[this]{rackPage=false;spectralPage=false;rebuild();});filters->setToggleState(!rackPage&&!spectralPage,dontSendNotification);
   auto* rack=button(tr(fr,"RACK 12 EFFETS","12-SLOT EFFECTS RACK"),{246,198,237,41},[this]{rackPage=true;spectralPage=false;rebuild();});rack->setToggleState(rackPage,dontSendNotification);
   auto* spec=button(tr(fr,"MOTEUR SPECTRAL","SPECTRAL ENGINE"),{493,198,237,41},[this]{rackPage=false;spectralPage=true;rebuild();});spec->setToggleState(spectralPage,dontSendNotification);
   if(spectralPage){buildSpectral();}else if(!rackPage){
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
   for(int i=0;i<13;++i){String name=i==0?tr(fr,"Tous les sons","All sounds"):i==1?tr(fr,"Favoris","Favorites"):catName(i-2);if(i>=2){int count=0;for(int j=0;j<eg::factoryCount;++j)if(presetCategory(j)==i-2)++count;name+="  "+String(count);}auto* b=button(name,{47,266+i*37,253,33},[this,i]{category=i;browserPage=0;refreshRows();});b->getProperties().set("left",true);categoryButtons[i]=b;}
   auto e=std::make_unique<TextEditor>();e->setTextToShowWhenEmpty(tr(fr,"Rechercher un nom, une catégorie ou un tag...","Search name, category or tag..."),muted);e->onTextChange=[this]{browserPage=0;refreshRows();};search=addControl(std::move(e),{342,198,980,42});
   button(tr(fr,"SON ALÉATOIRE","RANDOM SOUND"),{1334,198,133,42},[this]{loadPreset(1+Random::getSystemRandom().nextInt(100));});
   for(int i=0;i<8;++i){rows[i]=button("",{350,290+i*56,620,52},[this,i]{if(rowIds[i]>=0)loadPreset(rowIds[i]);});rows[i]->getProperties().set("left",true);stars[i]=button("",{1384,298+i*56,64,36},[this,i]{int n=rowIds[i];if(n<0)return;favorites[n]=!favorites[n];proc.state.state.setProperty("factoryFav_"+String(n),favorites[n],nullptr);refreshRows();});}
   button("<",{1080,748,80,37},[this]{browserPage=jmax(0,browserPage-1);refreshRows();});pageLabel=button("",{1170,748,180,37},[]{});pageLabel->setInterceptsMouseClicks(false,false);button(">",{1360,748,95,37},[this]{browserPage=jmin(jmax(0,(matchCount-1)/8),browserPage+1);refreshRows();});
   refreshRows();
  }
  resized();repaint();
 }
 void refreshRows(){if(page!=7||!search)return;auto q=search->getText().trim();std::array<int,eg::factoryCount> matches{};matchCount=0;
  for(int n=0;n<eg::factoryCount;++n){bool keep=category==0||(category==1&&favorites[n])||(category>=2&&presetCategory(n)==category-2);if(!keep||(!q.isEmpty()&&!String(eg::factory[n].name).containsIgnoreCase(q)&&!catName(presetCategory(n)).containsIgnoreCase(q)&&!String(eg::factory[n].tag).containsIgnoreCase(q)))continue;matches[matchCount++]=n;}
  browserPage=jlimit(0,jmax(0,(matchCount-1)/8),browserPage);rowIds.fill(-1);
  for(int i=0;i<8;++i){int idx=browserPage*8+i;int n=idx<matchCount?matches[idx]:-1;rowIds[i]=n;rows[i]->setVisible(n>=0);stars[i]->setVisible(n>=0);if(n>=0){rows[i]->setButtonText(eg::factory[n].name);rows[i]->setToggleState(n==current,dontSendNotification);stars[i]->setButtonText(favorites[n]?String::fromUTF8("★"):String::fromUTF8("☆"));stars[i]->setToggleState(favorites[n],dontSendNotification);}}
  for(int i=0;i<13;++i)if(categoryButtons[i])categoryButtons[i]->setToggleState(i==category,dontSendNotification);if(pageLabel)pageLabel->setButtonText(String(browserPage+1)+" / "+String(jmax(1,(matchCount+7)/8))+"  ("+String(matchCount)+")");repaint();
 }
 void loadPreset(int i){if(i<0||i>=eg::factoryCount)return;current=i;proc.spectral.request(eg::SpectralEngine::Clear);auto p=eg::factorySettings(i);for(auto* param:proc.getParameters())param->setValueNotifyingHost(param->getDefaultValue());for(int j=0;j<4;++j){const auto& l=p.layers[j];auto set=[&](const char* id,float v){proc.set(lid(j,id),v);};
 #define LP(k) set(#k,float(l.k))
 LP(level);LP(pan);LP(size);LP(density);LP(position);LP(scatter);LP(pitch);LP(drift);LP(spread);LP(motion);LP(cutoff);LP(cutoff2);LP(resonance);LP(resonance2);LP(filterMode);LP(filterMode2);LP(filterRouting);LP(source);LP(direction);LP(shape);LP(enabled);LP(sampleMode);
 #undef LP
 }
 #define GP(k) proc.set(#k,p.k)
 GP(osc1);GP(osc2);GP(oscPitch1);GP(oscPitch2);GP(fm);GP(ring);GP(attack);GP(release);GP(master);GP(velocity);GP(lfoRate);GP(lfoDepth);GP(xyX);GP(xyY);GP(xyPos);GP(xyDensity);GP(drive);
 #undef GP
 for(int k=0;k<8;++k){proc.set("macro"+String(k),p.macros[k]);auto& l=p.mod.lfos[k];proc.set(mid("lfo",k,"rate"),l.rate);proc.set(mid("lfo",k,"phase"),l.phase);proc.set(mid("lfo",k,"shape"),float(l.shape));proc.set(mid("lfo",k,"sync"),float(l.sync));}
 for(int k=0;k<12;++k){auto& f=p.effects[k];proc.set(fxid(k,"type"),float(f.type));proc.set(fxid(k,"enabled"),float(f.enabled));proc.set(fxid(k,"amount"),f.amount);proc.set(fxid(k,"mix"),f.mix);}
 for(int k=0;k<64;++k){auto& r=p.mod.routes[k];proc.set(mid("route",k,"source"),float(r.source));proc.set(mid("route",k,"dest"),float(r.dest));proc.set(mid("route",k,"depth"),r.depth);proc.set(mid("route",k,"curve"),r.curve);}
 for(int k=0;k<4;++k){auto& s=p.mod.steps[k];proc.set(mid("step",k,"length"),float(s.length));proc.set(mid("step",k,"rate"),s.rate);proc.set(mid("step",k,"glide"),s.glide);proc.set(mid("step",k,"sync"),float(s.sync));for(int n=0;n<64;++n)proc.set(sid(k,n),s.values[n]);auto& f=p.mod.functions[k];proc.set(mid("function",k,"rate"),f.rate);proc.set(mid("function",k,"curve"),f.curve);proc.set(mid("function",k,"shape"),float(f.shape));proc.set(mid("function",k,"loop"),float(f.loop));}
 sound.setSelectedId(i+2,dontSendNotification);updateActive();refreshRows();repaint();
 }
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
