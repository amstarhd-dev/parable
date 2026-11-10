#include "germanium_measure.h"
#include "soundforge/excitation.h"
#include <array>
#include <iomanip>
#include <iostream>
using namespace parable::dsp;
namespace {
struct Measurements {
    germaniumlab::Errors reference,coarse;
    double aliasOne=0,aliasFour=0;
    struct Row{std::string name;float fuzz;double source,level;unsigned bin;GermaniumRate mode;germaniumlab::Spectrum value;};
    std::vector<Row> rows;
};
Measurements measure(){
    Measurements m;m.reference=germaniumlab::references();m.coarse=germaniumlab::references(96000,false);
    auto add=[&](const char* name,float fuzz,double source,double level,unsigned bin,GermaniumRate mode=GermaniumRate::Oversampled4x){
        auto c=germaniumlab::variation(name);c.sourceOhms=source;
        m.rows.push_back({name,fuzz,source,level,bin,mode,germaniumlab::spectrum(c,{fuzz,.5f},bin,level,mode)});};
    for(const auto* name:{"healthy","q1_leaky","feedback_wrong"})for(unsigned bin:{17u,85u,511u})add(name,.8f,1000,.0001,bin);
    for(float fuzz:{0.f,.5f,1.f})for(double level:{.001,.01,.05,.1})add("healthy",fuzz,1000,level,85);
    for(double source:{1000.,10000.,50000.})add("healthy",.8f,source,.0001,85,GermaniumRate::Reference1x);
    for(double source:{1000.,50000.})for(double level:{.005,.05})add("healthy",.8f,source,level,85);
    // Coarse1x high-frequency .05V case can leave the supported junction domain;
    // keep that rejected diagnostic separate, never measure its fallback as alias.
    m.aliasOne=germaniumlab::spectrum({}, {1,.5f},751,.01,GermaniumRate::Reference1x).foldedThird;
    m.aliasFour=germaniumlab::spectrum({}, {1,.5f},751,.01).foldedThird;
    lab::require(m.aliasOne-m.aliasFour>6,"Ge selected alias line did not improve");
    lab::require(m.rows[1].value.fundamental>m.rows[4].value.fundamental*2,"Q1 leakage did not attenuate the small-signal response");
    lab::require(m.rows[21].value.zinReal>100&&m.rows[21].value.zinReal<50000,"Implausible declared small-signal input impedance");
    lab::require(m.rows[21].value.fundamental>m.rows[23].value.fundamental*2,"Source loading did not affect response");
    lab::require(m.rows[10].value.thdn<m.rows[18].value.thdn-3,"Fuzz control did not change nonlinear behavior");
    return m;
}
void reports(const std::filesystem::path& output,const Measurements& m){
    std::ofstream f(output/"measurements.json");lab::require(bool(f),"Cannot write Ge measurements");f<<std::setprecision(12);
    const auto& e=m.reference;
    f<<"{\n\"device_relative_error_floor_1na\":"<<e.deviceRelative<<",\n\"bias_peak_volts\":"<<e.biasPeak<<",\n\"node_rms_volts\":"<<e.nodeRms
     <<",\n\"node_peak_volts\":"<<e.nodePeak<<",\n\"jack_rms_volts\":"<<e.jackRms<<",\n\"jack_peak_volts\":"<<e.jackPeak<<",\n\"input_rms_amps\":"<<e.inputRms
     <<",\n\"native_reference_rate_hz\":192000,\n\"coarse_96k_node_rms_volts\":"<<m.coarse.nodeRms<<",\n\"coarse_96k_node_peak_volts\":"<<m.coarse.nodePeak
     <<",\n\"selected_folded_third_1x_dbc\":"<<m.aliasOne<<",\n\"selected_folded_third_4x_dbc\":"<<m.aliasFour
     <<",\n\"host_instance_bytes\":"<<sizeof(Germanium)<<"\n}\n";f.flush();lab::require(bool(f),"Cannot finish Ge measurements");
    std::ofstream csv(output/"frequency-and-level.csv");lab::require(bool(csv),"Cannot write Ge response");csv<<std::setprecision(12);
    csv<<"case,fuzz,source_ohms,input_volts,frequency_hz,oversampling,fundamental_volts,thdn_dbc,second_harmonic_dbc,mean,peak,zin_real_ohms_1x_only,zin_imag_ohms_1x_only,max_all_phase_evaluations\n";
    for(const auto& r:m.rows){const auto& s=r.value;const bool one=r.mode==GermaniumRate::Reference1x;
        csv<<r.name<<','<<r.fuzz<<','<<r.source<<','<<r.level<<','<<r.bin*48000./4096<<','<<(one?1:4)<<','<<s.fundamental<<','<<s.thdn<<','<<s.second<<','<<s.mean<<','<<s.peak<<',';
        if(one)csv<<s.zinReal;
        csv<<',';if(one)csv<<s.zinImag;
        csv<<','<<s.maxEvaluations<<'\n';}
    csv.flush();lab::require(bool(csv),"Cannot finish Ge response");
}
void selfTest(){
    std::vector<float> pure(4096);for(unsigned i=0;i<pure.size();++i)pure[i]=static_cast<float>(.2*std::sin(2*germaniumlab::pi*85*i/pure.size()));
    lab::require(std::abs(std::abs(germaniumlab::projection(pure,85))-.2)<1e-8,"Ge coherent projection known-sine failure");
    const auto m=measure();std::cout<<std::setprecision(12)<<"Ge SPICE bias/node RMS/peak "<<m.reference.biasPeak<<'/'<<m.reference.nodeRms<<'/'<<m.reference.nodePeak
        <<" jack RMS/peak "<<m.reference.jackRms<<'/'<<m.reference.jackPeak<<" alias1x/4x "<<m.aliasOne<<'/'<<m.aliasFour
        <<" Zin "<<m.rows[21].value.zinReal<<"+j"<<m.rows[21].value.zinImag<<'\n';
}
void auditions(const std::filesystem::path& output,const std::filesystem::path& input){
    const auto wave=input.empty()?soundforge::syntheticPlucks(.05):lab::read(input);
    lab::require(soundforge::stats(wave.samples).peak<=.5,"Input peak exceeds .5 modelV; attenuate owned input before the2x dynamics comparison");
    const std::string provenance=input.empty()?"SYNTHETIC shared harmonic plucks/strums; .05 modelV peak; not recorded guitar":"USER-PROVIDED mono PCM16; ownership unverified; input1=1 modelV";
    const std::array<std::string,10> names{"00_dry","01_healthy_fuzz_low","02_healthy_fuzz_mid","03_healthy_fuzz_high","04_weak_input","05_strong_input",
        "06_source_50k","07_source_50k_weak","08_q1_leaky","09_feedback_wrong"};
    const std::array<float,10> fuzz{0,0,.8f,1,.8f,.8f,.8f,.8f,.8f,.8f};
    auto caseName=[](unsigned i){return i==8?"q1_leaky":i==9?"feedback_wrong":"healthy";};
    auto inputGain=[](unsigned i){return i==4||i==7?.1:i==5?2.:1.;};
    std::vector<std::filesystem::path> files{"renders.csv","measurements.json","frequency-and-level.csv","LISTEN.txt","configuration.json"};
    for(const auto& name:names)for(const auto* kind:{"matched_","relative_"})files.emplace_back(kind+name+".wav");
    if(!input.empty())for(const auto& name:files){const auto p=output/name;
        lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)
            &&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite input recording");}
    std::array<std::vector<float>,10> signals;signals[0]=wave.samples;
    std::array<std::uint64_t,10> calls{},evaluations{},failed{},contained{};std::array<unsigned,10> maximum{};std::array<GermaniumNodes,10> biases{};
    for(unsigned i=1;i<signals.size();++i){auto c=germaniumlab::variation(caseName(i));if(i==6||i==7)c.sourceOhms=50e3;
        Germanium m;lab::require(m.configure(float(wave.rate),{fuzz[i],.5f},c),"Ge audition setup rejected");biases[i]=m.bias();signals[i].resize(wave.samples.size());
        for(std::size_t s=0;s<wave.samples.size();++s)signals[i][s]=m.process(static_cast<float>(wave.samples[s]*inputGain(i)));
        calls[i]=m.solverCalls();evaluations[i]=m.loopEvaluations();failed[i]=m.failedSolves();contained[i]=m.containedOutputs();maximum[i]=m.maximumLoopEvaluations();
        lab::require(failed[i]==0&&contained[i]==0,"Ge audition used fallback/clipping; refuse normalization");}
    const double target=soundforge::rmsTarget(signals,std::pow(10.,-22./20),.89);
    double relativePeak=0;for(const auto& signal:signals)relativePeak=std::max(relativePeak,soundforge::stats(signal).peak);
    const double commonGain=relativePeak>.89?.89/relativePeak:1;
    // Compare numerical/scientific gates before publishing any files.
    const auto measurements=measure();std::filesystem::create_directories(output);reports(output,measurements);
    std::ofstream csv(output/"renders.csv");lab::require(bool(csv),"Cannot write Ge manifest");csv<<std::setprecision(12);
    csv<<"# "<<provenance<<"\n# rate="<<wave.rate<<", frames="<<wave.samples.size()<<"; positive ground, -9V supply, 4x circuit, FIR delay48 base samples\n";
    csv<<"case,fuzz,volume,input_gain,source_ohms,feedback_ohms,q1_bc_leakage_amps,base1_dc_volts,collector1_dc_volts,emitter2_dc_volts,collector2_dc_volts,raw_rms_dbfs,raw_peak_dbfs,raw_mean,relative_common_gain,matched_gain,matched_rms_dbfs,matched_peak_dbfs,matched_mean,solver_calls,loop_evaluations,max_evaluations,failed_solves,contained_outputs\n";
    for(unsigned i=0;i<signals.size();++i){const auto raw=soundforge::stats(signals[i]);lab::require(raw.peak<.999999,"Ge render hit numerical rail");
        auto relative=signals[i];for(auto& s:relative)s=static_cast<float>(s*commonGain);
        lab::write(output/("relative_"+names[i]+".wav"),{wave.rate,relative});const double gain=soundforge::rmsGain(raw,target);
        for(auto& s:signals[i])s=static_cast<float>(s*gain);
        const auto final=soundforge::stats(signals[i]);lab::require(final.peak<=.890001,"Ge matched peak exceeds headroom");
        lab::write(output/("matched_"+names[i]+".wav"),{wave.rate,signals[i]});const auto c=germaniumlab::variation(caseName(i));const auto& b=biases[i];
        csv<<names[i]<<','<<fuzz[i]<<",0.5,"<<inputGain(i)<<','<<(i==6||i==7?50e3:1000)<<','<<c.feedbackOhms<<','<<c.q1.bcLeakageAmps
           <<','<<b.base1<<','<<b.collector1<<','<<b.emitter2<<','<<b.collector2<<','<<soundforge::db(raw.rms)<<','<<soundforge::db(raw.peak)<<','<<raw.mean
           <<','<<commonGain<<','<<gain<<','<<soundforge::db(final.rms)<<','<<soundforge::db(final.peak)<<','<<final.mean<<','<<calls[i]<<','<<evaluations[i]<<','<<maximum[i]<<','<<failed[i]<<','<<contained[i]<<'\n';}
    csv.flush();lab::require(bool(csv),"Cannot finish Ge manifest");
    std::ofstream config(output/"configuration.json");lab::require(bool(config),"Cannot write Ge config");config<<std::setprecision(17);
    config<<"{\n\"source_kind\":\""<<(input.empty()?"synthetic":"user-provided-unverified")<<"\",\n\"sample_rate_hz\":"<<wave.rate<<",\n\"frames\":"<<wave.samples.size()
          <<",\n\"supply_volts\":-9,\n\"ground_volts\":0,\n\"source_ohms\":1000,\n\"input_farads\":2.2e-6,\n\"collector1_ohms\":33000,\n\"feedback_ohms\":100000,\n\"fuzz_ohms\":1000,\n\"bypass_farads\":20e-6,\n\"collector2_ohms\":8200,\n\"supply_tap_ohms\":470,\n\"output_farads\":10e-9,\n\"volume_ohms\":500000,\n\"load_ohms\":1000000,\n\"each_bc_farads\":100e-12,\n\"volume_electrical_fraction\":0.5,\n\"oversampling\":4,\n\"both_transistors\":{\"transport_is_amps\":20.66e-6,\"beta_f\":229.6,\"beta_r\":14.66,\"nf\":1.133,\"nr\":1.140,\"thermal_volts\":0.0255,\"vaf_volts\":19.68,\"var_volts\":88.28,\"ikf_amps\":0.463,\"ikr_amps\":0.2415,\"ise_amps\":2.190e-6,\"isc_amps\":7.546e-6,\"ne\":1.796,\"nc\":1.364},\n\"q1_leaky_isc_amps\":75.46e-6,\n\"feedback_wrong_ohms\":10000,\n\"relative_common_gain\":"<<commonGain
          <<",\n\"dry_bypasses_circuit\":true,\n\"per_case_controls_bias_gains\":\"renders.csv\"\n}\n";config.flush();lab::require(bool(config),"Cannot finish Ge config");
    std::ofstream notes(output/"LISTEN.txt");lab::require(bool(notes),"Cannot write Ge listening notes");
    notes<<provenance<<"\nFirst compare matched_01/02/03 for Fuzz, 04/05 for level, 06/07 for50k source, 08/09 against02.\n"
         <<"Then compare relative_ files: one common gain="<<commonGain<<" preserves actual attenuation/dynamics. Matched files use individual whole-file RMS gains to common target="<<soundforge::db(target)<<"dBFS; not LUFS/perceptual matching.\n"
         <<"Raw RMS/peak/DC, all gains, component values, sourceR, negative physical bias and solver work are in renders.csv/configuration.json. Dry bypasses the circuit.\n"
         <<"Q1 leakage is a controlled10x BC leakage parameter experiment, not a calibrated temperature sweep or universal failed-device threshold. Feedback wrong part100k->10k.\n"
         <<"Untrimmed published fitted AC128 sample used twice; Q2 collector about-7.55V, not forced-4.5V. Reduced model and measured-parameter limits: ADR005.\n"
         <<"No cabinet/amp/alignment; output length equals input, tails may truncate. Add trailing silence to recordings. No physical circuit, real guitar, hardware CPU or founder musical approval claimed.\n";
    notes.flush();lab::require(bool(notes),"Cannot finish Ge listening notes");std::cout<<"Created20 Ge WAVs in "<<output<<'\n'<<provenance<<'\n';
}
}
int main(int argc,char** argv){try{
    if(argc==2&&std::string(argv[1])=="--self-test"){selfTest();return 0;}
    std::filesystem::path output="germanium-auditions",input;
    for(int i=1;i<argc;++i){const std::string key=argv[i];if((key=="--output"||key=="--input")&&i+1<argc){
        const std::string value=argv[++i];lab::require(!value.empty(),"Empty input/output path");(key=="--output"?output:input)=value;
    }else throw std::invalid_argument("Usage: parable_germanium_lab [--output DIR] [--input mono-pcm16.wav] | --self-test");}
    auditions(output,input);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
