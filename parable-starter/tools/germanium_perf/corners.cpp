#include "perf_bridge.h"
#include "parable_germanium.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace parable::dsp;
std::vector<std::string> fields(const std::string& row){
 std::vector<std::string> result;std::string s;std::istringstream in(row);
 while(std::getline(in,s,','))result.push_back(s);
 return result;
}
int main(){try{
 const std::string directory=PERF_SPICE_FIXTURES;
 std::ifstream config(directory+"/cases.json");if(!config)throw std::runtime_error("Missing corner configurations");
 nlohmann::json cases;config>>cases;if(cases.size()!=4)throw std::runtime_error("Incomplete corner configurations");
 bool pass=true;std::cout<<std::setprecision(12)<<"{\"scope\":\"additional independent ngspice corners; unchanged original thresholds\",\"implementations\":[";bool comma=false;
 for(auto* api:{perf_libm(),perf_h32(),perf_h64(),perf_p8(),perf_p12()}){
  void* model=api->create();
  auto configure=[&](unsigned index){const auto& f=cases.at(index);GermaniumCircuit c;c.supplyVolts=f.at("rail");c.sourceOhms=f.at("source");c.feedbackOhms=f.at("feedback");c.q1.bcLeakageAmps=f.at("leak");c.q1.forwardGain=f.at("beta1");c.q2.forwardGain=f.at("beta2");
   if(!api->configure(model,192000,f.at("fuzz"),.5f,0,&c,sizeof(c)))throw std::runtime_error("Independent corner configuration rejected");};
  double bias=0,nodeSum=0,nodePeak=0,jackSum=0,jackPeak=0,inputSum=0;unsigned biases=0,nodeCount=0,rows=0;std::uint64_t failures=0,contained=0;
  std::string row;std::ifstream dc(directory+"/bias.csv");if(!dc)throw std::runtime_error("Missing corner DC fixture");std::getline(dc,row);
  while(std::getline(dc,row)){const auto f=fields(row);if(f.size()!=7)throw std::runtime_error("Invalid corner DC row");configure(unsigned(std::stoul(f[0])));const auto state=api->state(model);for(unsigned i=0;i<6;++i)bias=std::max(bias,std::abs(state.bias[i]-std::stod(f[i+1])));++biases;}
  std::ifstream tr(directory+"/transient.csv");if(!tr)throw std::runtime_error("Missing corner transient fixture");std::getline(tr,row);unsigned current=999,sample=0;
  while(std::getline(tr,row)){const auto f=fields(row);if(f.size()!=9)throw std::runtime_error("Invalid corner transient row");unsigned id=unsigned(std::stoul(f[0]));
   if(id!=current){if(current!=999){const auto s=api->state(model);failures+=s.failed;contained+=s.contained;}configure(id);current=id;sample=0;}
   const unsigned target=unsigned(std::stoul(f[1]))*2;double level=cases.at(id).at("level");
   for(;sample<=target;++sample){float input=float(level*std::sin(2*3.14159265358979323846*997*sample/192000)),output=0;api->block(model,&input,&output,1);}
   const auto s=api->state(model);for(unsigned i=0;i<5;++i){double error=s.nodes[i]-std::stod(f[i+2]);nodeSum+=error*error;nodePeak=std::max(nodePeak,std::abs(error));++nodeCount;}
   double jack=s.nodes[5]-std::stod(f[7]),input=s.nodes[6]-std::stod(f[8]);jackSum+=jack*jack;inputSum+=input*input;jackPeak=std::max(jackPeak,std::abs(jack));++rows;
  }
  const auto last=api->state(model);failures+=last.failed;contained+=last.contained;api->destroy(model);
  if(biases!=4||rows!=768||nodeCount!=3840)throw std::runtime_error("Incomplete independent corner samples");
  double nodeRms=std::sqrt(nodeSum/nodeCount),jackRms=std::sqrt(jackSum/rows),inputRms=std::sqrt(inputSum/rows);
  bool ok=bias<1e-5&&nodeRms<.02&&nodePeak<.2&&jackRms<.0015&&jackPeak<.01&&inputRms<1e-6&&failures==0&&contained==0;
  pass=pass&&ok;if(comma)std::cout<<',';comma=true;
  std::cout<<"{\"implementation\":\""<<api->name<<"\",\"passed\":"<<(ok?"true":"false")<<",\"bias_peak_volts\":"<<bias<<",\"node_rms_volts\":"<<nodeRms<<",\"node_peak_volts\":"<<nodePeak<<",\"jack_rms_volts\":"<<jackRms<<",\"jack_peak_volts\":"<<jackPeak<<",\"input_rms_amps\":"<<inputRms<<",\"fallbacks\":"<<failures<<",\"containments\":"<<contained<<"}";
 }
 std::cout<<"]}\n";return pass?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
