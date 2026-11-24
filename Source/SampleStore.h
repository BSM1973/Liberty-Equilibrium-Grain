#pragma once
#include <vector>
#include <array>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <cmath>
#include <algorithm>
namespace eg {
struct SampleData {
 std::vector<float> left,right;double rate=48000;std::string name,path;float peak=1;
 size_t frames()const noexcept{return left.size();}
 size_t bytes()const noexcept{return (left.size()+right.size())*sizeof(float);}
};
class SampleStore {
 std::array<std::atomic<const SampleData*>,4> current{};
 std::vector<std::unique_ptr<const SampleData>> retained;
 std::mutex writer;size_t used=0;
public:
 size_t retainedBytes(){std::lock_guard<std::mutex> lock(writer);return used;}
 const SampleData* get(int layer)const noexcept{return current[size_t(layer)].load(std::memory_order_acquire);}
 bool publish(int layer,std::unique_ptr<SampleData> data){
  if(!data||data->frames()<2||data->right.size()!=data->left.size()||!std::isfinite(data->rate)||data->rate<8000||data->rate>192000)return false;
  std::lock_guard<std::mutex> lock(writer);
  // Reuse immutable audio for the same file and contents, including reloads and multiple layers.
  for(auto& old:retained)if(old->path==data->path&&old->rate==data->rate&&old->left==data->left&&old->right==data->right){current[size_t(layer)].store(old.get(),std::memory_order_release);return true;}
  if(used+data->bytes()>128u*1024u*1024u)return false;
  used+=data->bytes();auto* raw=data.get();retained.push_back(std::move(data));current[size_t(layer)].store(raw,std::memory_order_release);return true;
 }
 void clear(int layer) noexcept{current[size_t(layer)].store(nullptr,std::memory_order_release);}
};
}
