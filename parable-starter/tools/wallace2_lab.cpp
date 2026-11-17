#include "wallace2_graph.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
using Clock=std::chrono::steady_clock;
namespace {
std::vector<float> kernel(const std::filesystem::path& p){
    std::ifstream f(p,std::ios::binary|std::ios::ate);lab::require(bool(f),"cannot open kernel");const auto bytes=f.tellg();
    lab::require(bytes>0&&bytes<=32768*4&&bytes%4==0,"kernel must be1..32768 float32le samples");
    std::vector<float> ir(size_t(bytes)/4);f.seekg(0);
    for(auto&x:ir){unsigned char b[4];f.read(reinterpret_cast<char*>(b),4);uint32_t u=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);std::memcpy(&x,&u,4);}
    lab::require(bool(f),"kernel read failed");return ir;
}
void floats(const std::filesystem::path& p,const std::vector<float>& x){
    std::ofstream f(p,std::ios::binary);for(float y:x){uint32_t v;std::memcpy(&v,&y,4);unsigned char b[4];for(int j=0;j<4;++j)b[j]=static_cast<unsigned char>(v>>(8*j));f.write(reinterpret_cast<const char*>(b),4);}lab::require(bool(f),"float write failed");
}
std::vector<float> convolve(const std::vector<float>& x,const std::vector<float>& h,double* us=nullptr){
    wallace2::Cabinet c;lab::require(c.prepare(48000,h.data(),h.size()),"cab prepare");std::vector<float> y(x.size());double elapsed=0;
    for(size_t i=0;i<x.size();i+=64){const auto t=Clock::now();lab::require(c.process(x.data()+i,y.data()+i),"cab block");elapsed+=std::chrono::duration<double,std::micro>(Clock::now()-t).count();}
    if(us)*us=elapsed/x.size();
    return y;
}
double maxError(const std::vector<float>& a,const std::vector<float>& b){double e=0;for(size_t i=0;i<a.size();++i)e=std::max(e,double(std::abs(a[i]-b[i])));return e;}
}
int main(int argc,char**argv){try{
    std::filesystem::path input,irDir,output,verify;wallace2::Config settings;
    for(int i=1;i<argc;++i){lab::require(i+1<argc,"missing argument");const std::string k=argv[i],v=argv[++i];
        if(k=="--input")input=v;else if(k=="--irs")irDir=v;else if(k=="--output")output=v;else if(k=="--verify-against")verify=v;
        else if(k=="--clean")settings.clean=std::stof(v);else if(k=="--muff")settings.muff=std::stof(v);
        else if(k=="--rotary")settings.rotary=std::stof(v);else if(k=="--tape")settings.tape=std::stof(v);else if(k=="--hall")settings.hall=std::stof(v);else throw std::runtime_error("unknown argument");}
    lab::require(!input.empty()&&!irDir.empty()&&!output.empty(),"--input mono48k.wav --irs DIR --output DIR");
    lab::require(settings.clean>=.05f&&settings.clean<=2,"principal clean must be meaningful and nonzero");
    const auto h=kernel(irDir/"jensen-48k.f32"),h2=kernel(irDir/"v30-48k.f32");
    auto check=std::make_unique<wallace2::Graph>();lab::require(check->prepare(h.data(),h.size(),settings),"invalid levels/kernel");check.reset();
    auto source=lab::read(input);lab::require(source.rate==48000&&source.samples.size()>=48000&&source.samples.size()<=960000,"1..20s mono48k required");
    const auto peak=soundforge::stats(source.samples).peak;lab::require(peak>0,"silent input");const double inputGain=.2/peak;
    for(float&x:source.samples)x=float(x*inputGain);
    const size_t active=source.samples.size();source.samples.resize(((active+9*48000+63)/64)*64,0);
    std::filesystem::create_directories(output);lab::require(std::filesystem::weakly_canonical(input).parent_path()!=std::filesystem::weakly_canonical(output),"source must be outside output");
    std::ofstream csv(output/"levels.csv");csv<<std::setprecision(12)<<"name,rms,peak,mean,process_us_per_frame,max_observed_block_us,fixed_bytes_excluding_cab_heap,stem_sum_max_error\n";
    auto save=[&](const std::string& name,const std::vector<float>& x,double us=0,double maxUs=0,size_t bytes=0,double error=0){
        const auto s=soundforge::stats(x);lab::require(s.peak<.98,"raw gain exceeds headroom; do not limit");
        if(verify.empty()){lab::write(output/(name+"_raw.wav"),{48000,x});floats(output/(name+".f32"),x);}
        else {const auto expected=lab::encode({48000,x});std::ifstream f(verify/(name+"_raw.wav"),std::ios::binary);const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});lab::require(bytes==expected,"deterministic WAV verification failed");}
        csv<<name<<','<<s.rms<<','<<s.peak<<','<<s.mean<<','<<us<<','<<maxUs<<','<<bytes<<','<<error<<'\n';};
    // Independently execute exact original graph for required SF023 serial control.
    std::vector<float> fuzz(source.samples.size()),serial(fuzz.size());
    {auto g=std::make_unique<wallace::Graph>();lab::require(g->prepare(wallace::Mode::Serial),"serial prepare");wallace::Block b;
     for(size_t i=0;i<fuzz.size();i+=64){lab::require(g->process(source.samples.data()+i,b),"serial block");std::copy(b.fuzz.begin(),b.fuzz.end(),fuzz.begin()+i);std::copy(b.out.begin(),b.out.end(),serial.begin()+i);}lab::require(g->healthy(),"serial containment");}
    save("01_muff_direct",fuzz);save("04_original_serial",serial);
    double cab1us=0,cab2us=0;save("02_muff_jensen",convolve(fuzz,h,&cab1us),0);save("03_muff_v30",convolve(fuzz,h2,&cab2us),0);
    std::vector<float> baseline,baselineClean;
    for(int recipe=5;recipe<=10;++recipe){auto c=settings;if(recipe==6){c.clean=0;c.cleanDisabledControl=true;}
        if(recipe==7){c.clean=settings.clean*1.5f;c.muff=settings.muff*.6f;}
        if(recipe>=8)c.placement=wallace2::Placement::PostCab;
        if(recipe==9)c.ducking=true;
        if(recipe==10){c.rotary=.25f;c.tape=.32f;c.hall=.40f;}
        const std::string name=recipe==5?"05_pre_cab_clean":recipe==6?"06_CONTROL_clean_disabled":recipe==7?"07_more_clean_less_muff":recipe==8?"08_post_cab_studio":recipe==9?"09_post_cab_duck":"10_expansive";
        auto g=std::make_unique<wallace2::Graph>();lab::require(g->prepare(h.data(),h.size(),c),"new graph prepare");wallace2::Block b;
        std::vector<float> full(fuzz.size()),clean(fuzz.size()),fuzzStem(fuzz.size()),wet(fuzz.size());double elapsed=0,maxUs=0,minDuck=1;
        for(size_t i=0;i<full.size();i+=64){const auto t=Clock::now();const bool ok=g->process(source.samples.data()+i,b);const double us=std::chrono::duration<double,std::micro>(Clock::now()-t).count();elapsed+=us;maxUs=std::max(maxUs,us);lab::require(ok,"new graph process rejected");
            for(size_t j=0;j<64;++j){full[i+j]=b.out[j];clean[i+j]=b.clean[j];fuzzStem[i+j]=b.fuzz[j];wet[i+j]=b.wet[j];minDuck=std::min(minDuck,double(b.duck[j]));}}
        lab::require(g->healthy(),"native containment/fallback");const size_t bytes=g->fixedBytes();g.reset();
        // Diagnostic extra passes are outside measured processing, not live DSP states.
        auto cleanCab=convolve(clean,h),fuzzCab=convolve(fuzzStem,h);
        if(c.placement==wallace2::Placement::PreCab)wet=convolve(wet,h);
        std::vector<float> sum(full.size());for(size_t i=0;i<sum.size();++i)sum[i]=(cleanCab[i]+fuzzCab[i])+wet[i];
        const double error=maxError(full,sum);lab::require(error<2e-6,"final stems do not reconstruct mix");
        save(name,full,elapsed/full.size(),maxUs,bytes,error);
        if(recipe!=6){save(name+"_stem_clean_before_cab",clean);save(name+"_stem_clean_after_cab",cleanCab);save(name+"_stem_fuzz_after_cab",fuzzCab);save(name+"_stem_ambience_final",wet);}
        if(recipe==5){baseline=full;baselineClean=cleanCab;}
        if(recipe==6){for(size_t i=0;i<sum.size();++i)sum[i]=baseline[i]-full[i];lab::require(maxError(sum,baselineClean)<2e-6,"clean-disabled counterfactual failed");}
        std::cout<<name<<" rendered; stem residual "<<error<<", min duck "<<minDuck<<'\n';
    }
    std::ofstream receipt(output/"settings.json");receipt<<std::setprecision(12)<<"{\"rate\":48000,\"block\":64,\"active_frames\":"<<active<<",\"tail_seconds\":9,\"input_gain\":"<<inputGain<<",\"clean\":"<<settings.clean<<",\"muff\":"<<settings.muff<<",\"rotary\":"<<settings.rotary<<",\"tape\":"<<settings.tape<<",\"hall\":"<<settings.hall<<",\"duck_db\":2.5,\"clean_alignment_frames\":48,\"cab_extra_buffering_frames\":0,\"cab1_us_per_frame\":"<<cab1us<<",\"cab2_us_per_frame\":"<<cab2us<<",\"graph_fixed_bytes_excludes_cab_heap\":true,\"extra_stem_passes_outside_graph_timing\":true}\n";
    lab::require(bool(receipt)&&bool(csv),"receipt write");
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
