#include "cmos_measure.h"
#include "soundforge/excitation.h"
#include <array>
#include <iomanip>
#include <iostream>

using namespace parable::dsp;
namespace {
struct Measurements {
    cmoslab::Errors references;
    double aliasOne=0,aliasFour=0;
    struct Row {std::string name;float drive;unsigned bin;double input;cmoslab::Spectrum value;};
    std::vector<Row> rows;
};
Measurements measure(){
    Measurements m;m.references=cmoslab::references();
    m.aliasOne=cmoslab::spectrum({}, {.9f,.5f},751,.1,CmosRate::Reference1x).aliasDb;
    m.aliasFour=cmoslab::spectrum({}, {.9f,.5f},751,.1).aliasDb;
    for(const auto* name:{"healthy","coupling_small","feedback_low"})for(unsigned bin:{17u,85u,511u})
        m.rows.push_back({name,.5f,bin,.0001,cmoslab::spectrum(cmoslab::variation(name),{.5f,.5f},bin,.0001)});
    for(float drive:{0.f,.5f,1.f})for(double input:{.001,.01,.1,.2})
        m.rows.push_back({"healthy",drive,85,input,cmoslab::spectrum({}, {drive,.5f},85,input)});
    lab::require(m.rows[0].value.fundamental>m.rows[3].value.fundamental*10,"Coupling-cap change must affect bass response");
    lab::require(m.rows[0].value.fundamental>m.rows[6].value.fundamental*2,"Feedback change must affect bass/input sensitivity");
    lab::require(m.aliasOne-m.aliasFour>6,"CMOS folded-third alias fixture did not improve");
    return m;
}
void reports(const std::filesystem::path& output,const Measurements& m){
    std::ofstream f(output/"measurements.json");lab::require(bool(f),"Cannot create CMOS measurements");f<<std::setprecision(12);
    f<<"{\n\"independent_dc_peak_error_volts\":"<<m.references.dcPeak<<",\n\"independent_transient_rms_error_volts\":"<<m.references.transientRms
     <<",\n\"independent_transient_peak_error_volts\":"<<m.references.transientPeak<<",\n\"alias_folded_third_1x_dbc\":"<<m.aliasOne
     <<",\n\"alias_folded_third_4x_dbc\":"<<m.aliasFour<<",\n\"alias_reduction_db\":"<<m.aliasOne-m.aliasFour<<",\n\"host_instance_bytes\":"<<sizeof(Cmos)<<"\n}\n";
    f.flush();lab::require(bool(f),"Cannot finish CMOS measurements");
    std::ofstream csv(output/"frequency-and-level.csv");lab::require(bool(csv),"Cannot create CMOS spectrum report");csv<<std::setprecision(12);
    csv<<"case,drive,input_volts,frequency_hz,fundamental,thdn_dbc,second_harmonic_dbc,mean,peak,max_last_phase_loop_evaluations\n";
    for(const auto& r:m.rows){const auto s=r.value;csv<<r.name<<','<<r.drive<<','<<r.input<<','<<r.bin*48000./4096<<','<<s.fundamental
        <<','<<s.thdnDb<<','<<s.secondDb<<','<<s.mean<<','<<s.peak<<','<<s.maxIterations<<'\n';}
    csv.flush();lab::require(bool(csv),"Cannot finish CMOS spectrum report");
}
void selfTest(){
    std::vector<float> pure(4096);for(unsigned i=0;i<pure.size();++i)pure[i]=static_cast<float>(.2*std::sin(2*cmoslab::pi*85*i/pure.size()));
    lab::require(std::abs(cmoslab::amplitude(pure,85)-.2)<1e-8&&cmoslab::amplitude(pure,170)<1e-8,"Coherent projection failed known-sine test");
    const auto m=measure();
    std::cout<<"CMOS SPICE DC/RMS/peak V "<<m.references.dcPeak<<'/'<<m.references.transientRms<<'/'<<m.references.transientPeak
             <<"; folded-third alias 1x/4x dBc "<<m.aliasOne<<'/'<<m.aliasFour<<'\n';
}
void auditions(const std::filesystem::path& output,const std::filesystem::path& input){
    const auto wave=input.empty()?soundforge::syntheticPlucks(.05):lab::read(input);
    const std::string provenance=input.empty()?"SYNTHETIC shared harmonic plucks/strums; raw peak .05 model volts; not recorded guitar":"USER-PROVIDED mono PCM16; ownership/provenance unverified; 1 unit=1 model volt";
    const std::array<std::string,8> names{"00_dry","01_healthy_low","02_healthy_mid","03_healthy_high","04_weak_input","05_strong_input","06_coupling_small","07_feedback_low"};
    const std::array<float,8> drives{0,0,.5f,1,.5f,.5f,.5f,.5f};
    const auto caseName=[](unsigned i){return i==6?"coupling_small":i==7?"feedback_low":"healthy";};
    if(!input.empty()){
        std::vector<std::filesystem::path> files{"renders.csv","measurements.json","frequency-and-level.csv","LISTEN.txt","configuration.json"};
        for(const auto& name:names)files.emplace_back(name+".wav");
        for(const auto& name:files){const auto p=output/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)
            &&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite input recording");}
    }
    std::array<std::vector<float>,8> signals;signals[0]=wave.samples;
    std::array<std::uint64_t,8> calls{},loops{},failed{};std::array<unsigned,8> maximum{};
    for(unsigned i=1;i<signals.size();++i){Cmos model;lab::require(model.configure(float(wave.rate),{drives[i],.5f},cmoslab::variation(caseName(i))),"CMOS audition config rejected");
        signals[i].resize(wave.samples.size());const float gain=i==4?.1f:i==5?2.f:1.f;
        for(std::size_t s=0;s<wave.samples.size();++s)signals[i][s]=model.process(wave.samples[s]*gain);
        calls[i]=model.solverCalls();loops[i]=model.loopEvaluations();failed[i]=model.failedSolves();maximum[i]=model.maximumLoopEvaluations();
        lab::require(failed[i]==0,"CMOS audition used numerical fallback; refuse to normalize it away");}
    const double target=soundforge::rmsTarget(signals,std::pow(10.,-22./20),.89);
    std::filesystem::create_directories(output);std::ofstream csv(output/"renders.csv");lab::require(bool(csv),"Cannot write CMOS render manifest");csv<<std::setprecision(12);
    csv<<"# "<<provenance<<"\n# rate="<<wave.rate<<", frames="<<wave.samples.size()<<"; regulated 9V IC rail; 4x circuit; FIR delay 48 base samples plus circuit delay\n";
    csv<<"file,drive,volume,input_gain,rail_volts,interstage_farads,second_feedback_ohms,raw_rms_dbfs,raw_peak_dbfs,raw_mean,matching_gain,final_rms_dbfs,final_peak_dbfs,final_mean,solver_calls,loop_evaluations,max_loop_evaluations,failed_solves\n";
    for(unsigned i=0;i<signals.size();++i){const auto raw=soundforge::stats(signals[i]);
        lab::require(raw.peak<.999999,"CMOS audition reached the numerical output rail; refuse level matching");
        const double gain=soundforge::rmsGain(raw,target);
        for(auto& s:signals[i])s=static_cast<float>(s*gain);
        const auto final=soundforge::stats(signals[i]);lab::require(final.peak<=.890001,"CMOS render exceeds headroom");
        lab::write(output/(names[i]+".wav"),{wave.rate,signals[i]});const auto c=cmoslab::variation(caseName(i));
        csv<<names[i]<<".wav,"<<drives[i]<<",0.5,"<<(i==4?.1:i==5?2:1)<<','<<c.railVolts<<','<<c.interstageFarads<<','<<c.secondFeedbackOhms
           <<','<<soundforge::db(raw.rms)<<','<<soundforge::db(raw.peak)<<','<<raw.mean<<','<<gain<<','<<soundforge::db(final.rms)<<','<<soundforge::db(final.peak)<<','<<final.mean
           <<','<<calls[i]<<','<<loops[i]<<','<<maximum[i]<<','<<failed[i]<<'\n';}
    csv.flush();lab::require(bool(csv),"Cannot finish CMOS render manifest");reports(output,measure());
    const CmosCircuit nominal;std::ofstream config(output/"configuration.json");lab::require(bool(config),"Cannot write CMOS configuration");config<<std::setprecision(17);
    config<<"{\n\"source_kind\":\""<<(input.empty()?"synthetic":"user-provided-unverified")<<"\",\n\"sample_rate_hz\":"<<wave.rate<<",\n\"frames\":"<<wave.samples.size()
          <<",\n\"rail_volts\":"<<nominal.railVolts<<",\n\"input_ohms\":"<<nominal.inputOhms<<",\n\"input_farads\":"<<nominal.inputFarads
          <<",\n\"first_feedback_ohms\":"<<nominal.firstFeedbackOhms<<",\n\"drive_pot_ohms\":"<<nominal.drivePotOhms<<",\n\"first_feedback_farads\":"<<nominal.firstFeedbackFarads
          <<",\n\"interstage_farads\":"<<nominal.interstageFarads<<",\n\"second_feedback_ohms\":"<<nominal.secondFeedbackOhms<<",\n\"second_feedback_farads\":"<<nominal.secondFeedbackFarads
          <<",\n\"output_farads\":"<<nominal.outputFarads<<",\n\"volume_pot_ohms\":"<<nominal.volumePotOhms<<",\n\"load_ohms\":"<<nominal.loadOhms
          <<",\n\"volume_electrical_fraction\":0.5,\n\"oversampling\":4,\n\"coupling_small_farads\":3.3e-10,\n\"feedback_low_ohms\":100000,\n\"dry_has_no_circuit\":true,\n\"per_file_controls_gains_work\":\"renders.csv\"\n}\n";
    config.flush();lab::require(bool(config),"Cannot finish CMOS configuration");
    std::ofstream readme(output/"LISTEN.txt");lab::require(bool(readme),"Cannot write CMOS listening notes");
    readme<<provenance<<"\nCompare 01/02/03 healthy Drive, 04/05 input dynamics, then 06/07 against 02.\nWhole-file RMS target "<<soundforge::db(target)
          <<" dBFS, peak target .89 before PCM16 quantization; no perceptual/LUFS claim. Raw/final levels, gains, circuit units and bounded solver work in renders.csv.\n"
          <<"Reduced published two-inverter signal path; 9V stiff rail at IC, no battery/dropper/supply dynamics. Coupling fault: 33nF to 330pF; feedback fault: 1Meg to 100k. No new curves per fault.\n"
          <<"No cabinet/amp, alignment or extra sonic post-processing. Output length equals input; long RC tails may be truncated. Include trailing silence in external recordings.\n"
          <<"Not calibrated to your CD4049, full factory unit, real guitar, hardware CPU/latency or founder musical approval. Model fit extrapolation and numerical limits: ADR-004.\n";
    readme.flush();lab::require(bool(readme),"Cannot finish CMOS listening notes");std::cout<<"Created 8 CMOS WAVs in "<<output<<'\n'<<provenance<<'\n';
}
}
int main(int argc,char** argv){try{
    if(argc==2&&std::string(argv[1])=="--self-test"){selfTest();return 0;}
    std::filesystem::path output="cmos-auditions",input;
    for(int i=1;i<argc;++i){const std::string key=argv[i];if((key=="--output"||key=="--input")&&i+1<argc){
        const std::string value=argv[++i];lab::require(!value.empty(),"Empty input/output path");(key=="--output"?output:input)=value;
    }else throw std::invalid_argument("Usage: parable_cmos_lab [--output DIR] [--input mono-pcm16.wav] | --self-test");}
    auditions(output,input);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
