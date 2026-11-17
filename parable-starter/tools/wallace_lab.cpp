#include "wallace_graph.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
using Clock=std::chrono::steady_clock;
struct Run {std::vector<float> out,wet,fuzz,rotary,tape,hall;double us=0,maxBlockUs=0;size_t persistent=0;};
Run run(const std::vector<float>& input,wallace::Mode mode,wallace::RoutingConfig config,const std::filesystem::path& trace={}) {
    auto graph=std::make_unique<wallace::Graph>();lab::require(graph->prepare(mode,config),"graph prepare rejected");
    Run r;for(auto* p:{&r.out,&r.wet,&r.fuzz,&r.rotary,&r.tape,&r.hall})p->resize(input.size());
    std::ofstream csv;if(!trace.empty()){csv.open(trace);csv<<std::setprecision(12)<<"time_s,input_abs,envelope,duck_gain,duck_db,rotary_wet,tape_wet,hall_wet,mixed_wet\n";}
    wallace::Block b;
    for(size_t i=0;i<input.size();i+=64){const auto start=Clock::now();const bool ok=graph->process(input.data()+i,b);const auto us=std::chrono::duration<double,std::micro>(Clock::now()-start).count();r.us+=us;r.maxBlockUs=std::max(r.maxBlockUs,us);lab::require(ok,"invalid input block");
        for(size_t j=0;j<64;++j){size_t n=i+j;r.out[n]=b.out[j];r.wet[n]=b.wet[j];r.fuzz[n]=b.fuzz[j];r.rotary[n]=b.rotary[j];r.tape[n]=b.tape[j];r.hall[n]=b.hall[j];
            if(csv&&n%480==0)csv<<double(n)/48000<<','<<std::abs(input[n])<<','<<b.envelope[j]<<','<<b.gain[j]<<','<<-20*std::log10(b.gain[j])<<','<<b.rotary[j]<<','<<b.tape[j]<<','<<b.hall[j]<<','<<b.wet[j]<<'\n';}
    }
    lab::require(graph->healthy(),"an existing processor reported numerical fallback/containment");
    r.us/=input.size();r.persistent=graph->persistentBytes();return r;
}
int main(int argc,char**argv){try {
    if(argc==5&&std::string(argv[1])=="--scale") {auto w=lab::read(argv[2]);const float gain=std::stof(argv[4]);lab::require(std::isfinite(gain)&&gain>=0&&gain<=100,"bad matching gain");for(auto&x:w.samples)x*=gain;lab::write(argv[3],w);return 0;}
    std::filesystem::path input,output;wallace::RoutingConfig config;
    for(int i=1;i<argc;++i){const std::string k=argv[i];lab::require(i+1<argc,"missing argument");const std::string v=argv[++i];
        if(k=="--input")input=v;else if(k=="--output")output=v;
        else {const float x=std::stof(v);if(k=="--duck-db")config.duckDb=x;else if(k=="--attack-ms")config.attackMs=x;else if(k=="--release-ms")config.releaseMs=x;
            else if(k=="--core")config.core=x;else if(k=="--clean")config.clean=x;else if(k=="--rotary")config.rotary=x;else if(k=="--tape")config.tape=x;else if(k=="--hall")config.hall=x;
            else if(k=="--send-rotary")config.sendRotary=x;else if(k=="--send-tape")config.sendTape=x;else if(k=="--send-hall")config.sendHall=x;
            else if(k=="--tape-hp")config.tapeHz=x;else if(k=="--hall-hp")config.hallHz=x;else throw std::runtime_error("unknown argument");}}
    lab::require(!input.empty()&&!output.empty(),"--input mono48k.wav --output DIR");auto source=lab::read(input);
    lab::require(source.rate==48000&&source.samples.size()>=48000&&source.samples.size()<=20*48000,"require1..20s at48k");
    wallace::Routing check;lab::require(check.prepare(config),"invalid routing settings");
    std::filesystem::create_directories(output);lab::require(std::filesystem::weakly_canonical(input).parent_path()!=std::filesystem::weakly_canonical(output),"input must be outside output folder");
    const auto stats=soundforge::stats(source.samples);lab::require(stats.peak>0,"silent source");const double inputGain=.2/stats.peak;
    for(auto&x:source.samples)x=float(x*inputGain);
    const size_t active=source.samples.size();source.samples.resize(((active+9*48000+63)/64)*64,0);
    //20s source+9s tail is a whole number of blocks. Short CLI tests can round up.
    std::ofstream levels(output/"levels.csv");levels<<std::setprecision(12)<<"name,rms,peak,mean,frames,process_us_per_frame,max_observed_block_us,persistent_bytes\n";
    auto save=[&](const std::string& name,const std::vector<float>& samples,const Run* run=nullptr){const auto s=soundforge::stats(samples);lab::require(s.peak<.98,"headroom exceeded; lower one common gain, do not limit");lab::write(output/(name+".wav"),{48000,samples});levels<<name<<','<<s.rms<<','<<s.peak<<','<<s.mean<<','<<samples.size()<<','<<(run?run->us:0)<<','<<(run?run->maxBlockUs:0)<<','<<(run?run->persistent:0)<<'\n';};
    save("01_dry",source.samples);
    {const auto a=run(source.samples,wallace::Mode::Serial,config);save("02_muff",a.fuzz);save("03_A_serial_raw",a.out,&a);}
    Run b=run(source.samples,wallace::Mode::Static,config);save("04_B_parallel_raw",b.out,&b);save("06_B_wet",b.wet);save("09_C_no_ducking",b.out);
    {const auto c=run(source.samples,wallace::Mode::Dynamic,config,output/"ducking.csv");save("05_C_dynamic_raw",c.out,&c);save("07_C_wet",c.wet);
     lab::require(c.fuzz==b.fuzz&&c.rotary==b.rotary&&c.tape==b.tape&&c.hall==b.hall,"parallel native stems changed");}
    {auto naked=config;naked.carving=false;const auto c=run(source.samples,wallace::Mode::Dynamic,naked);save("08_C_no_carving",c.out,&c);lab::require(c.fuzz==b.fuzz&&c.rotary==b.rotary&&c.tape==b.tape&&c.hall==b.hall,"carving changed native stems");}
    save("stem_rotary_wet",b.rotary);save("stem_tape_wet",b.tape);save("stem_hall_wet",b.hall);
    //Actual routing-only repeat on the same native branch samples, engines absent.
    wallace::Routing routing;lab::require(routing.prepare(config),"routing prepare");float checksum=0;auto start=Clock::now();
    for(size_t i=0;i<source.samples.size();++i)checksum+=routing.process(source.samples[i],b.fuzz[i],b.rotary[i],b.tape[i],b.hall[i]).output;
    const double routingUs=std::chrono::duration<double,std::micro>(Clock::now()-start).count()/source.samples.size();
    auto graph=std::make_unique<wallace::Graph>();lab::require(graph->prepare(wallace::Mode::Static,config),"benchmark prepare");
    std::ofstream costs(output/"costs.json");costs<<std::setprecision(12)<<"{\"routing_us_per_frame\":"<<routingUs<<",\"routing_bytes\":"<<sizeof(wallace::Routing)<<",\"checksum\":"<<checksum<<",\"components\":{";
    bool first=true;float sink=0;std::array<float,64> left{},right{};
    for(const std::string name:{"muff","rotary","tape","hall"}){start=Clock::now();
        if(name=="muff")for(float x:source.samples)sink+=graph->muff_.process(x);
        if(name=="rotary")for(float x:b.fuzz)sink+=graph->rotary_.process(x*config.sendRotary,.7f);
        if(name=="tape")for(float x:b.fuzz)sink+=graph->tape_.process(x*config.sendTape);
        if(name=="hall")for(size_t i=0;i<source.samples.size();i+=64){std::array<float,64> send{};for(size_t j=0;j<64;++j)send[j]=b.fuzz[i+j]*config.sendHall;graph->hall_.process(send.data(),left.data(),right.data());sink+=left[0]+right[0];}
        const double us=std::chrono::duration<double,std::micro>(Clock::now()-start).count()/source.samples.size();if(!first)costs<<',';first=false;costs<<'"'<<name<<"\":{\"us_per_frame\":"<<us<<",\"state_bytes\":"<<(name=="muff"?sizeof(graph->muff_):name=="rotary"?sizeof(graph->rotary_):name=="tape"?sizeof(graph->tape_)+graph->medium_.capacity()*sizeof(float):sizeof(graph->hall_))<<'}';}
    costs<<"},\"sink\":"<<sink<<",\"frames\":"<<source.samples.size()<<",\"full_graph_daisy_admitted\":false}\n";
    std::ofstream settings(output/"settings.json");settings<<std::setprecision(12)<<"{\"rate\":48000,\"block\":64,\"source_frames\":"<<active<<",\"tail_seconds\":9,\"input_gain\":"<<inputGain<<",\"common_output_gain\":1,\"muff\":{\"sustain\":0.65,\"tone\":0.5,\"volume\":0.65,\"fault\":false,\"oversampling\":4},\"rotary_speed_command\":0.7,\"serial_mixes\":[0.35,0.25,0.2],\"parallel\":{\"core\":"<<config.core<<",\"clean\":"<<config.clean<<",\"rotary\":"<<config.rotary<<",\"tape\":"<<config.tape<<",\"hall\":"<<config.hall<<",\"sends\":["<<config.sendRotary<<','<<config.sendTape<<','<<config.sendHall<<"]},\"tape\":{\"loop_seconds\":1.2,\"delay_seconds\":0.3,\"feedback\":0.32,\"drive\":2.4,\"record_hz\":4500,\"replay_hz\":6000,\"erase_retention\":0},\"hall\":{\"decay\":1,\"focusing\":0},\"return_hp_hz\":["<<config.tapeHz<<','<<config.hallHz<<"],\"ducking\":{\"attack_ms\":"<<config.attackMs<<",\"release_ms\":"<<config.releaseMs<<",\"max_db\":"<<config.duckDb<<",\"low\":"<<config.low<<",\"high\":"<<config.high<<"},\"clean_delay_samples\":48,\"extra_queued_blocks\":0,\"09_reuses_B\":true,\"native_parallel_stems_identical\":true}\n";
    lab::require(bool(settings)&&bool(costs)&&bool(levels),"receipt write failed");std::cout<<"Created12 raw WAVs,9 comparisons, actual unchanged stems, controls/CPU/duck receipts\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
