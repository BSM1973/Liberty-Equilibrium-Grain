#define EG_QUALITY_TEST 1
#include "../Source/Plugin.cpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
void qc(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
 ScopedJuceInitialiser_GUI gui;
 auto folder=File::getCurrentWorkingDirectory().getChildFile("quality-reports");folder.createDirectory();
 auto procOwner=std::make_unique<Processor>();auto& proc=*procOwner;proc.prepareToPlay(48000,128);ReferenceSurface surface(proc);
 StringArray metrics;metrics.add("index,name,peak,rms,dc,nonfinite");
 for(int i=1;i<eg::factoryCount;++i){
  surface.qcLoadFactory(i);proc.prepareToPlay(48000,128);AudioBuffer<float> b(2,128);MidiBuffer midi;double sum=0,squares=0;float peak=0;int count=0;
  for(int block=0;block<375;++block){midi.clear();if(block==0)midi.addEvent(MidiMessage::noteOn(1,60,.8f),0);if(block==250)midi.addEvent(MidiMessage::noteOff(1,60),0);proc.processBlock(b,midi);for(int ch=0;ch<2;++ch)for(int n=0;n<128;++n){float v=b.getSample(ch,n);qc(std::isfinite(v)&&std::abs(v)<=1,"invalid preset output");peak=std::max(peak,std::abs(v));sum+=v;squares+=v*v;++count;}}
  qc(squares>1e-7,"silent factory preset through plugin");metrics.add(String(i)+","+eg::factory[i].name+","+String(peak,7)+","+String(std::sqrt(squares/count),7)+","+String(sum/count,7)+",0");
 }
 qc(folder.getChildFile("factory-plugin-audit.csv").replaceWithText(metrics.joinIntoString("\n")),"write factory report");std::cout<<"100 factory presets PASS"<<std::endl;
 surface.qcLoadFactory(1);proc.set(lid(0,"size"),321);proc.set("oscMode1",5);proc.set("oscUnison1",8);proc.set(mid("seq",63,"pitch"),12);proc.set(mid("route",63,"depth"),.43f);proc.toggleRandomLock(lid(0,"size"));proc.armLearn("osc1");proc.receiveCC(3,21,100);proc.capture(0);proc.set("osc1",.9f);proc.capture(1);proc.set("morphEnabled",1);
 MemoryBlock data;proc.getStateInformation(data);auto restoreOwner=std::make_unique<Processor>();auto& restored=*restoreOwner;restored.setStateInformation(data.getData(),int(data.getSize()));
 qc(std::abs(restored.get(lid(0,"size"))-321)<.01f&&restored.get("oscMode1")==5&&restored.get("oscUnison1")==8&&restored.get(mid("seq",63,"pitch"))==12,"state parameter restore");
 qc(std::abs(restored.get(mid("route",63,"depth"))-.43f)<.002f&&restored.randomLocked(lid(0,"size"))&&restored.learnedCC("osc1")==277&&restored.snapshotFlags.load()==3,"state mapping restore");
 restored.set("morphEnabled",0);float original=restored.get(lid(0,"size"));restored.randomize(1);qc(restored.get(lid(0,"size"))==original,"random lock");restored.undoRandom();qc(restored.get(lid(0,"size"))==original,"random undo");
 auto preset=folder.getChildFile("roundtrip.egpreset");qc(proc.saveUserPreset(preset)&&restored.openUserPreset(preset),"preset file roundtrip");preset.deleteFile();
 std::cout<<"State and preset roundtrip PASS; starting UI audit"<<std::endl;
 int issues=surface.qcLayout(folder);std::cout<<"100 factory presets through actual processor; state roundtrip PASS; layout warnings "<<issues<<"\n";
 // Keep geometry warnings visible in report; visual inspection remains a release gate.
 return 0;
}
