#include "parable_capricorn.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <iomanip>
#include <iostream>
using parable::dsp::Capricorn;using parable::dsp::CapricornControls;
int main(int argc,char** argv){try{
    lab::require(argc==5&&std::string(argv[1])=="--input"&&std::string(argv[3])=="--output","Use --input monoPCM16.wav --output NEW_DIR");
    const std::filesystem::path source=argv[2],out=argv[4];lab::require(!std::filesystem::exists(out),"Output must be a new directory");auto input=lab::read(source);lab::require(input.samples.size()>=input.rate,"At least1s input required");
    input.samples.resize(std::min(input.samples.size(),std::size_t(8)*input.rate));const auto original=soundforge::stats(input.samples);lab::require(original.peak>0,"Silent input rejected");const double inputGain=.04/original.peak;for(auto& x:input.samples)x=float(x*inputGain);input.samples.resize(input.samples.size()+input.rate,0);
    std::filesystem::create_directories(out);std::ofstream csv(out/"renders.csv"),bias(out/"bias.csv"),trace(out/"state.csv");for(auto* f:{&csv,&bias,&trace}){lab::require(bool(*f),"Receipt creation failed");*f<<std::setprecision(15);}
    csv<<"file,distortion,tone,level,C4_open,frames,offline_gain,raw_rms,raw_peak,final_rms,final_peak,dc,failures,input_clamps,table_clamps,host_us_per_frame\n";
    bias<<"distortion,C4_open,base_V,collector_V,emitter_V,diode_companion_ohms\n";trace<<"distortion,C4_open,frame,Q2_V,amplifier_V,diode_V,jack_V\n";
    auto write=[&](std::string name,const std::vector<float>& x,double gain,CapricornControls c,double us){auto y=x;for(auto& v:y)v=float(v*gain);const auto a=soundforge::stats(x),b=soundforge::stats(y);lab::require(b.peak<.999,"Refuse clipped WAV");lab::write(out/name,{input.rate,y});csv<<name<<','<<c.distortion<<','<<c.tone<<','<<c.level<<','<<c.openC4<<','<<y.size()<<','<<gain<<','<<a.rms<<','<<a.peak<<','<<b.rms<<','<<b.peak<<','<<b.mean<<",0,0,0,"<<us<<'\n';};
    auto dry=input.samples;for(auto& v:dry)v/=2;write("00_dry_common.wav",dry,1,{},0);
    for(unsigned group=0;group<2;++group){CapricornControls c;c.distortion=group?.9f:.15f;std::array<std::vector<float>,2> pair;double cost[2]{};
        for(unsigned fault=0;fault<2;++fault){c.openC4=fault!=0;Capricorn model;lab::require(model.prepare(float(input.rate),c),"Prepare failed");bias<<c.distortion<<','<<fault;for(auto v:model.bias())bias<<','<<v;bias<<','<<model.diodeResistance()<<'\n';
            auto& y=pair[fault];y.resize(input.samples.size());for(std::size_t n=0;n<y.size();++n){y[n]=model.process(input.samples[n]);if(n%8192==0){const auto p=model.probe();trace<<c.distortion<<','<<fault<<','<<n<<','<<p.q2Volts<<','<<p.amplifierVolts<<','<<p.diodeVolts<<','<<p.jackVolts<<'\n';}}
            lab::require(!model.failures()&&!model.invalidInputs()&&!model.inputClamps()&&!model.tableClamps(),"Containment in ordinary audio");model.reset();std::vector<float> repeat(y.size());const auto start=std::chrono::steady_clock::now();for(std::size_t n=0;n<y.size();++n)repeat[n]=model.process(input.samples[n]);cost[fault]=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/y.size();lab::require(repeat==y&&!model.failures(),"Reset/repeat mismatch");
            const std::string name=std::string(group?"high":"low")+(fault?"_C4_open_raw.wav":"_healthy_raw.wav");write(name,y,1,c,cost[fault]);std::cout<<"Wrote "<<name<<" RMS="<<soundforge::stats(y).rms<<" host_us/frame="<<cost[fault]<<'\n'<<std::flush;
        }
        const double target=std::min({soundforge::rmsTarget(pair,.07,.89),16*soundforge::stats(pair[0]).rms,16*soundforge::stats(pair[1]).rms});for(unsigned fault=0;fault<2;++fault){c.openC4=fault!=0;write(std::string(group?"high":"low")+(fault?"_C4_open_matched.wav":"_healthy_matched.wav"),pair[fault],soundforge::rmsGain(soundforge::stats(pair[fault]),target),c,cost[fault]);}
    }
    std::ofstream settings(out/"settings.json"),listen(out/"LISTEN.txt");settings<<std::setprecision(15)<<"{\"source\":\"published recipe Guitar-TECHS Player3 take03; caller-supplied PCM16\",\"excerpt_seconds_max\":8,\"tail_seconds\":1,\"input_peak_model_V\":0.04,\"input_gain\":"<<inputGain<<",\"low_distortion\":0.15,\"high_distortion\":0.9,\"tone\":0.5,\"level\":0.8,\"healthy_C4_F\":2.5e-10,\"fault_C4_F\":0,\"output_volts_per_unit\":2,\"oversampling\":4,\"FIR_taps\":97,\"state_bytes\":"<<sizeof(Capricorn)<<"}\n";
    listen<<"SF018 original1978-family reduced DS-1. One shared dry, low/high Distortion healthy/C4-open raw and offline RMS matched. Same input/Level. Q2 capacitor alone250pF->open; no EQ/gain fault decoration. Raw jackV/2; matching gains in renders.csv, not LUFS. First8s plus1s tail. Native renderer verifies bit-exact reset/repeat; host timings include full processor but not IO. Founder listening/physical pedal calibration/Daisy timing remain unproven.\n";
    for(auto* f:{&csv,&bias,&trace,&settings,&listen}){f->flush();lab::require(bool(*f),"Receipt write failed");}return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
