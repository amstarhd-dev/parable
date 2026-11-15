#include "parable_scorpio.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include <chrono>
#include <iomanip>
#include <iostream>
using parable::dsp::Scorpio;
using parable::dsp::ScorpioControls;
int main(int argc,char** argv){try {
    lab::require(argc==5&&std::string(argv[1])=="--input"&&std::string(argv[3])=="--output","Use --input monoPCM16.wav --output NEW_DIR");
    const std::filesystem::path input=argv[2],out=argv[4];lab::require(!std::filesystem::exists(out),"Output directory must be new");
    auto wave=lab::read(input);lab::require(wave.samples.size()>=wave.rate,"Input must have at least1second");
    wave.samples.resize(std::min(wave.samples.size(),std::size_t(8)*wave.rate));const auto original=soundforge::stats(wave.samples);
    lab::require(original.peak>0,"Input must not be silent");const double inputGain=.12/original.peak;
    for(auto& x:wave.samples)x=float(x*inputGain);
    wave.samples.resize(wave.samples.size()+wave.rate,0);
    std::filesystem::create_directories(out);std::ofstream csv(out/"renders.csv"),trace(out/"state.csv"),bias(out/"bias.csv");
    for(auto* f:{&csv,&trace,&bias}){lab::require(bool(*f),"Cannot write receipts");*f<<std::setprecision(15);}
    csv<<"file,gain,low_cut,level,fault,frames,offline_gain,raw_rms,raw_peak,final_rms,final_peak,dc,clipped_samples,failures,table_clamps,host_us_per_frame\n";
    trace<<"gain,fault,frame,preamp_V,flux_Vs,magnetizing_A,core_V,C10_relative_V,jack_V\n";
    bias<<"gain,fault,B1_V,C1_B2_V,E1_V,E2_V,C2_V,output_thevenin_ohms\n";
    auto write=[&](const std::string& name,const std::vector<float>& samples,double gain,ScorpioControls c,double us,uint32_t fails,uint32_t clamps){
        auto y=samples;for(auto& x:y)x=float(x*gain);const auto a=soundforge::stats(samples),b=soundforge::stats(y);lab::require(b.peak<.999,"Refuse clipped WAV");lab::write(out/name,{wave.rate,y});
        csv<<name<<','<<c.gain<<','<<c.lowCut<<','<<c.level<<','<<c.resistorDrift<<','<<y.size()<<','<<gain<<','<<a.rms<<','<<a.peak<<','<<b.rms<<','<<b.peak<<','<<b.mean<<",0,"<<fails<<','<<clamps<<','<<us<<'\n';};
    auto dry=wave.samples;for(auto& x:dry)x/=8;write("00_external_dry_common.wav",dry,1,{},0,0,0);
    write("01_external_dry_matched.wav",dry,std::min(16.,soundforge::rmsGain(soundforge::stats(dry),.07)),{},0,0,0);
    for(unsigned group=0;group<2;++group){std::array<std::vector<float>,2> signals;double costs[2]{};ScorpioControls c;c.gain=group?.9f:.1f;
        for(unsigned fault=0;fault<2;++fault){c.resistorDrift=fault!=0;Scorpio model;lab::require(model.prepare(float(wave.rate),c),"Scorpio prepare failed");
            bias<<c.gain<<','<<fault;for(double v:model.bias())bias<<','<<v;bias<<','<<model.sourceOhms()<<'\n';
            auto& y=signals[fault];y.resize(wave.samples.size());
            for(std::size_t n=0;n<y.size();++n){y[n]=model.process(wave.samples[n]);if(n%512==0){const auto p=model.probe();trace<<c.gain<<','<<fault<<','<<n<<','<<p.preampVolts<<','<<p.flux<<','<<p.magnetizingAmps<<','<<p.coreVolts<<','<<p.capacitorVolts<<','<<p.outputVolts<<'\n';}}
            lab::require(!model.failures()&&!model.invalidInputs()&&!model.inputClamps()&&!model.tableClamps(),"Numerical containment in normal audition");
            model.reset();std::vector<float> repeat(y.size());const auto start=std::chrono::steady_clock::now();for(std::size_t n=0;n<y.size();++n)repeat[n]=model.process(wave.samples[n]);
            costs[fault]=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/y.size();lab::require(repeat==y&&!model.failures(),"Reset/repeat changed native output");
            const std::string name=std::string(group?"high":"low")+(fault?"_R3_10k_raw.wav":"_healthy_raw.wav");write(name,y,1,c,costs[fault],model.failures(),model.tableClamps());
            std::cout<<"Wrote "<<name<<" host_us/frame="<<costs[fault]<<" RMS="<<soundforge::stats(y).rms<<'\n'<<std::flush;
        }
        const double target=std::min({soundforge::rmsTarget(signals,.07,.89),16*soundforge::stats(signals[0]).rms,16*soundforge::stats(signals[1]).rms});
        for(unsigned fault=0;fault<2;++fault){c.resistorDrift=fault!=0;const auto gain=std::min(16.,soundforge::rmsGain(soundforge::stats(signals[fault]),target));
            write(std::string(group?"high":"low")+(fault?"_R3_10k_matched.wav":"_healthy_matched.wav"),signals[fault],gain,c,costs[fault],0,0);}
    }
    std::ofstream settings(out/"settings.json"),notes(out/"LISTEN.txt");settings<<std::setprecision(15)<<"{\"source\":\"caller-supplied monoPCM16; published recipe verifies take03\",\"excerpt_seconds_max\":8,\"tail_seconds\":1,\"input_peak_model_V\":0.12,\"input_gain\":"<<inputGain<<",\"output_volts_per_unit\":8,\"low_gain\":0.1,\"high_gain\":0.9,\"low_cut\":0.35,\"level\":0.8,\"R3_healthy_ohms\":5600,\"R3_fault_ohms\":10000,\"L0_H\":20,\"flux_scale_Vs\":0.008,\"factor\":4,\"ADAA\":true,\"state_bytes\":"<<sizeof(Scorpio)<<"}\n";
    notes<<"SF017 reduced Skywave-family9V experiment. First8s of supplied input plus1s silence. Same input/calibration/Level for both voices; R3 alone5.6k->10k. Raw/common is actual jackVolts/8; separate matching gains in renders.csv capped16, not LUFS. External dry is shared between presets. Start low/high healthy versusR3_10k matched, then raw. Prepared quasi-static two-BJT nonlinearity plus linearized capacitor/feedback dynamics and passive loaded nonlinear magnetic flux/C10 states. Assumed uncalibrated20H/8mVs core, no hysteresis/remanence, no measured Hudson/TY141P fidelity or Seed3 deadline. Founder listening pending.\n";
    for(auto* f:{&csv,&trace,&bias,&settings,&notes}){f->flush();lab::require(bool(*f),"Cannot finish evidence");}
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
