#include "parable_tiled_room.h"
#include "parable_coupled_rooms.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <iomanip>
#include <iostream>
namespace {
void stereo(const std::filesystem::path& path,const std::vector<float>& l,const std::vector<float>& r){
    lab::require(l.size()==r.size()&&!l.empty(),"stereo size");auto b=lab::encode({48000,{0}});b.resize(44+4*l.size());
    const auto n=[&](size_t pos,uint32_t v,unsigned count){for(unsigned j=0;j<count;++j)b[pos+j]=uint8_t(v>>(j*8));};
    n(4,uint32_t(b.size()-8),4);n(22,2,2);n(28,48000*4,4);n(32,4,2);n(40,uint32_t(l.size()*4),4);
    for(size_t i=0;i<l.size();++i)for(unsigned ch=0;ch<2;++ch){const float x=ch?r[i]:l[i];lab::require(std::isfinite(x)&&std::abs(x)<.98f,"stereo headroom");n(44+4*i+2*ch,uint16_t(int(std::lround(x*32768.))),2);}
    std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());lab::require(bool(f),"stereo write");
}
}
int main(int argc,char**argv){try{
    lab::require(argc==5&&std::string(argv[1])=="--input"&&std::string(argv[3])=="--output","--input mono48k.wav --output DIR");const std::filesystem::path output=argv[4];auto src=lab::read(argv[2]);lab::require(src.rate==48000&&src.samples.size()<=960000&&src.samples.size()>=48000,"1..20s48k required");const double peak=soundforge::stats(src.samples).peak;lab::require(peak>0,"silent source");const double inputGain=.2/peak;for(auto&x:src.samples)x=float(x*inputGain);const size_t active=src.samples.size();src.samples.resize(active+3*48000,0);
    std::filesystem::create_directories(output);lab::require(std::filesystem::weakly_canonical(argv[2]).parent_path()!=std::filesystem::weakly_canonical(output),"source outside output");
    parable::soundforge::TiledRoom e;std::vector<float> buffer(e.requiredSamples(48000));
    std::ofstream costs(output/"costs.csv");costs<<std::setprecision(12)<<"mode,us_per_frame,state_bytes,buffer_bytes,peak,rms,diffuser_gain\n";
    for(bool fault:{false,true}){
        parable::soundforge::TiledSettings settings;settings.ceramic=fault;
        std::vector<float> mix(src.samples.size()),wet(mix.size()),left(mix.size()),right(mix.size());lab::require(e.prepare(48000,buffer.data(),buffer.size(),settings),"prepare");
        const auto start=std::chrono::steady_clock::now();
        for(size_t i=0;i<mix.size();++i){const auto y=e.process(src.samples[i]);mix[i]=y.mono;wet[i]=(y.wetLeft+y.wetRight)*.5f;left[i]=y.left;right[i]=y.right;}
        const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/mix.size();const std::string name=fault?"fault":"healthy";
        for(const auto* v:{&mix,&wet,&left,&right})lab::require(soundforge::stats(*v).peak<.98,"native render headroom");
        lab::write(output/(name+"_raw.wav"),{48000,mix});lab::write(output/(name+"_wet_raw.wav"),{48000,wet});stereo(output/(name+"_stereo_raw.wav"),left,right);
        std::vector<float> ir(6*48000),probe(ir.size());e.reset();
        for(size_t i=0;i<ir.size();++i){auto y=e.process(i==0?.2f:0);ir[i]=(y.wetLeft+y.wetRight)*.5f;}
        lab::write(output/("impulse_"+name+".wav"),{48000,ir});
        std::ofstream f(output/("impulse_"+name+".f32"),std::ios::binary);f.write(reinterpret_cast<const char*>(ir.data()),ir.size()*sizeof(float));lab::require(bool(f),"float impulse write");e.reset();
        for(size_t i=0;i<probe.size();++i){const float x=(i==0||i==4800||i==15360)?.2f:0;auto y=e.process(x);probe[i]=(y.wetLeft+y.wetRight)*.5f;}
        lab::write(output/("transient_"+name+".wav"),{48000,probe});
        costs<<name<<','<<us<<','<<sizeof(e)<<','<<e.bufferBytes()<<','<<soundforge::stats(mix).peak<<','<<soundforge::stats(mix).rms<<','<<e.diffuserGain()<<'\n';
    }
    lab::write(output/"dry_raw.wav",src);
    // Exact existing SF020 core. Aperture0 isolates its nominal.65s Room A.
    parable::soundforge::CoupledRooms room;std::vector<float> oldBuffer(room.requiredSamples(48000)),oldWet(src.samples.size()),oldIR(6*48000);
    lab::require(room.prepare(48000,oldBuffer.data(),oldBuffer.size(),{0,1,false}),"old room prepare");
    for(size_t i=0;i<oldWet.size();++i)oldWet[i]=room.process(src.samples[i]).mono;
    lab::write(output/"comparison_roomA_wet_raw.wav",{48000,oldWet});room.reset();
    for(size_t i=0;i<oldIR.size();++i)oldIR[i]=room.process(i==0?.2f:0).mono;
    lab::write(output/"impulse_roomA.wav",{48000,oldIR});
    std::ofstream json(output/"settings.json");json<<std::setprecision(12)<<"{\"rate\":48000,\"source_frames\":"<<active<<",\"tail_seconds\":3,\"input_gain\":"<<inputGain<<",\"size_ratio\":0.3,\"predelay_ms\":4,\"decay_seconds_control\":0.62,\"loop_gain\":"<<e.loopGain()<<",\"wet\":0.35,\"dry\":0.65,\"wet_calibration\":0.5,\"input_lp_hz\":10000,\"tank_lp_hz\":6500,\"modulation\":false,\"state_bytes\":"<<sizeof(e)<<",\"buffer_bytes\":"<<e.bufferBytes()<<",\"fault_implemented\":true,\"healthy_right_ap2_feedback\":0.5,\"fault_right_ap2_feedback\":0.94,\"comparison_roomA\":{\"aperture\":0,\"native_decay_seconds\":0.65,\"readout\":\"original mono return\"}}\n";
    lab::require(bool(json)&&bool(costs),"receipt write");std::cout<<"Healthy/fault same-network WAVs and original roomA comparison rendered\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
