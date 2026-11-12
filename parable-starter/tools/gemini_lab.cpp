// SF-005 listening receipt using the existing physical CMOS kernel and lab tools.
#include "cmos_measure.h"
#include "parable_prepare.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace parable::dsp;
namespace {
void audition(const std::filesystem::path& output,const std::filesystem::path& input) {
    auto source=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    lab::require(source.samples.size()<=29*source.rate,"Input must leave room for a one-second tail (<=29 s)");
    const auto sourceStats=soundforge::stats(source.samples);
    lab::require(sourceStats.peak>0,"Cannot calibrate silent input");
    const char* names[]={"00_calibrated_dry","01_healthy_raw","02_bias_leak_raw","03_dry_matched","04_healthy_matched","05_bias_leak_matched"};
    if(!input.empty()) {
        std::vector<std::filesystem::path> paths{"renders.csv","electrical.csv","spectrum.csv","LISTEN.txt","healthy.patch.json","bias-leak.patch.json"};
        for(const char* name:names)paths.emplace_back(std::string(name)+".wav");
        for(const auto& name:paths) {const auto p=output/name;
            lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input) &&
                (!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite input");}
    }
    const double calibration=.15/sourceStats.peak; // explicit software V/unit, not ADC/physical calibration.
    for(float& x:source.samples)x=static_cast<float>(x*calibration);
    source.samples.resize(source.samples.size()+source.rate,0); // same tail in every comparison.
    std::array<std::vector<float>,3> waves;waves[0]=source.samples;
    std::filesystem::create_directories(output);
    std::ofstream electrical(output/"electrical.csv"),csv(output/"renders.csv"),spectrum(output/"spectrum.csv");
    lab::require(bool(electrical)&&bool(csv)&&bool(spectrum),"Cannot create receipts");
    electrical<<std::setprecision(12)<<"variant,second_bias_ohms,sample,seconds,input_volts,gate1_volts,output1_volts,gate2_volts,output2_volts,jack_volts,last_phase_residual_amps,last_phase_loops,nmos2_cutoff\n";
    std::array<uint64_t,3> calls{},loops{},failures{},cutoff{};std::array<unsigned,3> maximum{};
    std::array<double,3> maxResidual{},g2{},o2{},processingUs{};
    for(size_t variant=1;variant<=2;++variant) {
        Cmos model;const auto c=geminiCircuit(variant==2);
        const std::string json=std::string(R"({"format":"parable.patch","version":1,"sampleRate":{"unit":"Hz","value":)")
            +std::to_string(source.rate)+R"(},"outputTrim":{"unit":"linear","value":1},"processors":[{"id":7,"type":"GeminiCmos","revision":1,"enabled":true,"parameters":{"drive":{"unit":"normalized","value":0.75},"volume":{"unit":"normalized","value":0.5},"biasLeak":{"unit":"boolean","value":)"
            +(variant==2?"true":"false")+R"(}}}],"routing":[{"from":"input","to":7},{"from":7,"to":"output"}]})";
        const auto decoded=parable::patch::Codec::decode(json);
        lab::require(std::holds_alternative<parable::patch::ValidatedPatch>(decoded),"Canonical Gemini decode failed");
        const auto& document=std::get<parable::patch::ValidatedPatch>(decoded);
        const auto prepared=parable::preparation::prepare(document,float(source.rate));
        lab::require(std::holds_alternative<parable::Patch>(prepared),"Canonical Gemini prepare failed");
        const auto native=std::get<parable::Patch>(prepared);parable::Engine engine(float(source.rate));
        lab::require(engine.load(native)==parable::Status::Ok,"Engine load failed");
        std::ofstream patch(output/(variant==1?"healthy.patch.json":"bias-leak.patch.json"));
        patch<<parable::patch::Codec::encode(document);patch.flush();lab::require(bool(patch),"Cannot save canonical receipt");
        lab::require(model.configure(float(source.rate),{.75f,.5f},c),"Gemini preparation failed");
        g2[variant]=model.secondGateBiasVolts();o2[variant]=model.secondOutputBiasVolts();waves[variant].resize(source.samples.size());
        for(size_t i=0;i<source.samples.size();++i) {
            waves[variant][i]=engine.process(source.samples[i]);
            lab::require(waves[variant][i]==model.process(source.samples[i]),"Engine altered the physical CMOS implementation");
            const auto n=engine.cmosNodes();
            const bool off=cmos_detail::mos(n.gate2,n.output2,false).value==0;
            cutoff[variant]+=off;maxResidual[variant]=std::max(maxResidual[variant],n.residualAmps);
            // Host-only decimated node receipt; not callback logging or all-phase telemetry.
            if(i%128==0)electrical<<variant<<','<<(variant==2?"220000":"open")<<','<<i<<','<<double(i)/source.rate<<','
                <<source.samples[i]<<','<<n.gate1<<','<<n.output1<<','<<n.gate2<<','<<n.output2<<','<<n.jackVolts<<','<<n.residualAmps<<','<<n.iterations<<','<<off<<'\n';
        }
        calls[variant]=model.solverCalls();loops[variant]=model.loopEvaluations();failures[variant]=model.failedSolves();maximum[variant]=model.maximumLoopEvaluations();
        lab::require(failures[variant]==0,"Refuse to present numerical fallback as a circuit fault");
        // Separate timed pass excludes statistics and receipt IO; same C++ implementation.
        lab::require(engine.load(native)==parable::Status::Ok,"Benchmark preparation failed");
        float checksum=0;const auto start=std::chrono::steady_clock::now();
        for(float x:source.samples)checksum+=engine.process(x);
        processingUs[variant]=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        lab::require(std::isfinite(checksum),"Invalid benchmark pass");
    }
    for(const auto& signal:waves)lab::require(soundforge::stats(signal).peak<.999,"Refuse to normalize numerical-rail clipping");
    lab::require(soundforge::stats(waves[2]).rms>=1e-6,"Fault output is near-silent; refuse to normalize numerical residue into a voice");
    const double target=soundforge::rmsTarget(waves,std::pow(10.,-26./20),.89);
    csv<<std::setprecision(12)<<"file,source_kind,rate,frames,software_input_gain,drive,volume,second_bias_ohms,second_bias_siemens,gate2_dc_volts,output2_dc_volts,raw_rms,raw_peak,raw_dc,matching_gain,final_rms,final_peak,final_dc,solver_calls,loop_evaluations,max_loop_evaluations,failed_solves,max_last_phase_residual_amps,last_phase_nmos_cutoff_count,host_processing_us\n";
    for(size_t i=0;i<6;++i) {
        const size_t variant=i%3;const auto raw=soundforge::stats(waves[variant]);
        const double gain=i<3?1:soundforge::rmsGain(raw,target);
        auto signal=waves[variant];for(float& x:signal)x=static_cast<float>(x*gain);
        const auto final=soundforge::stats(signal);lab::require(final.peak<=.890001 || i<3,"Matched output exceeds headroom");
        lab::write(output/(std::string(names[i])+".wav"),{source.rate,signal});
        csv<<names[i]<<".wav,"<<(input.empty()?"synthetic":"external-recording")<<','<<source.rate<<','<<signal.size()<<','<<calibration
           <<(variant?",0.75,0.5,":",n/a,n/a,")<<(variant==2?"220000":"open")<<','<<(variant==2?1/kGeminiBiasLeakOhms:0)<<','<<g2[variant]<<','<<o2[variant]
           <<','<<raw.rms<<','<<raw.peak<<','<<raw.mean<<','<<gain<<','<<final.rms<<','<<final.peak<<','<<final.mean<<','
           <<calls[variant]<<','<<loops[variant]<<','<<maximum[variant]<<','<<failures[variant]<<','<<maxResidual[variant]<<','<<cutoff[variant]<<','<<processingUs[variant]<<'\n';
    }
    spectrum<<std::setprecision(12)<<"bias_leak,drive,volume,input_volts,frequency_hz,fundamental,thdn_dbc,second_dbc,dc,peak,folded_third_dbc,oversampling\n";
    for(bool fault:{false,true})for(double level:{.005,.05,.15})for(unsigned bin:{17u,85u,511u}) {
        const auto m=cmoslab::spectrum(geminiCircuit(fault),{.75f,.5f},bin,level);
        spectrum<<fault<<",0.75,0.5,"<<level<<','<<bin*48000./4096<<','<<m.fundamental<<','<<m.thdnDb<<','<<m.secondDb<<','<<m.mean<<','<<m.peak<<','<<m.aliasDb<<",4\n";
    }
    for(bool fault:{false,true})for(auto mode:{CmosRate::Reference1x,CmosRate::Oversampled4x}) {
        const auto m=cmoslab::spectrum(geminiCircuit(fault),{.9f,.5f},751,.15,mode);
        spectrum<<fault<<",0.9,0.5,0.15,"<<751*48000./4096<<','<<m.fundamental<<','<<m.thdnDb<<','<<m.secondDb<<','<<m.mean<<','<<m.peak<<','<<m.aliasDb<<','<<(mode==CmosRate::Reference1x?1:4)<<'\n';
    }
    electrical.flush();csv.flush();spectrum.flush();lab::require(bool(electrical)&&bool(csv)&&bool(spectrum),"Cannot finish receipts");
    std::ofstream notes(output/"LISTEN.txt");
    notes<<"SF-005 Gemini CD4049 experimental physical model. Compare 04_healthy_matched.wav with 05_bias_leak_matched.wav.\n"
        <<"Both receive the SAME input/calibration/Drive .75/Volume .5 through actual canonical decode -> prepare -> Engine. Same-sample parity with standalone Cmos verified. Only a gate-2-to-VSS leakage path changes: open -> 220 kohm.\n"
        <<"00 is calibrated dry. 01/02 retain common gain=1; 03/04/05 share whole-file RMS "<<soundforge::db(target)<<" dBFS, peak <=.89, not LUFS/perceptual matching.\n"
        <<"Input: "<<(input.empty()?"owned synthetic plucks, NOT recorded guitar":"external mono PCM16 recording; CI provenance identifies pinned CC0 GuitarJam 050.wav; loader does not infer ownership")<<".\n"
        <<"One second silence appended; no resampling/cabinet/amp/extra glitch layer. 1 input unit=1 model Volt; peak scaled to .15 V, NOT hardware calibration. Output=jack V/4.\n"
        <<"Second gate/output DC, all-phase solver counts/fallback and matching gains in renders.csv. electrical.csv is decimated LAST internal phase; FIR output delay is 48 base samples.\n"
        <<"spectrum.csv contains coherent sine frequency/level/THD+N/DC and one folded-third alias comparison, not total alias rejection.\n"
        <<"The severe damaged bias is partly outside the measured MOS-fit range; endpoint-frozen parameters are unvalidated extrapolation. Not a measured physical unit or approved Gemini sound.\n"
        <<"No solver fallback or numerical output rail was allowed in these auditions. Human musical approval and Daisy runtime performance remain unproven.\n";
    notes.flush();lab::require(bool(notes),"Cannot finish listening instructions");std::cout<<"Created Gemini healthy/fault raw + matched WAVs in "<<output<<'\n';
}
}
int main(int argc,char**argv) {try {
    std::filesystem::path output,input;
    for(int i=1;i<argc;++i) {
        const std::string key=argv[i];lab::require((key=="--input"||key=="--output")&&i+1<argc,"Use --output DIR [--input mono-pcm16.wav]");
        const std::string value=argv[++i];lab::require(!value.empty(),"Empty path");(key=="--input"?input:output)=value;
    }
    lab::require(!output.empty(),"Explicit --output DIR required");audition(output,input);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
