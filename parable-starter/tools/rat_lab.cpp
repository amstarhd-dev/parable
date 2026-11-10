#include "rat_measure.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <iomanip>
#include <array>
using namespace parable::dsp;
namespace {
constexpr double pi=ratlab::pi;
using soundforge::stats;
using soundforge::db;

struct Measurements {
    ratlab::ReferenceErrors reference;
    std::array<ratlab::Spectrum,4> faults;
    struct Row {float distortion,filter,amplitude;unsigned bin;ratlab::Spectrum spectrum;};
    std::vector<Row> quality;
    double aliasOne=0,aliasFour=0;
};
Measurements measure(){
    Measurements m;m.reference=ratlab::spiceReferences();
    int i=0;for(const auto* name:{"healthy","diode_open","ruetz","compensation"})
        m.faults[i++]=ratlab::spectrum({.35f,.5f,.7f},ratlab::variation(name),171,.05);
    m.aliasOne=ratlab::spectrum({.9f,0,.7f},{},1003,.05,RatRate::Reference1x).aliasDb;
    m.aliasFour=ratlab::spectrum({.9f,0,.7f},{},1003,.05).aliasDb;
    for(float filter:{0.f,.5f,1.f})for(unsigned bin:{34u,171u,1021u})
        m.quality.push_back({0,filter,.0001f,bin,ratlab::spectrum({0,filter,.7f},{},bin,.0001)});
    for(float drive:{.02f,.35f,.9f})for(float input:{.0005f,.005f,.05f,.2f})
        m.quality.push_back({drive,.5f,input,171,ratlab::spectrum({drive,.5f,.7f},{},171,input)});
    // Upper-band response must darken as Filter increases.
    lab::require(m.quality[2].spectrum.fundamental>m.quality[8].spectrum.fundamental*10,"Filter response regression");
    lab::require(m.aliasOne-m.aliasFour>6,"Oversampling failed the measured alias-reduction fixture");
    lab::require(std::abs(m.faults[1].clipDc-m.faults[0].clipDc)>.1,"Open diode must change rectified node bias");
    return m;
}
void reports(const std::filesystem::path& output,const Measurements& m){
    std::ofstream f(output/"measurements.json");lab::require(bool(f),"Cannot write measurements");f<<std::setprecision(10);
    f<<"{\n\"spice_bilinear_mapped_max_gain_error_db\":"<<m.reference.maxGainDb<<",\"spice_bilinear_mapped_max_phase_error_degrees\":"<<m.reference.maxPhaseDegrees
     <<",\"spice_same_frequency_max_gain_error_db\":"<<m.reference.sameFrequencyGainDb
     <<",\"spice_transient_rms_error_volts\":"<<m.reference.transientRms<<",\"spice_transient_peak_error_volts\":"<<m.reference.transientPeak
     <<",\n\"alias_1x_dbc\":"<<m.aliasOne<<",\"alias_4x_dbc\":"<<m.aliasFour<<",\"alias_reduction_db\":"<<m.aliasOne-m.aliasFour
     <<",\"instance_bytes_host\":"<<sizeof(Rat)<<",\n\"fault_spectra\":[\n";
    const char* names[]={"healthy","diode_open","ruetz","compensation"};
    for(unsigned i=0;i<4;++i){const auto s=m.faults[i];f<<"{\"case\":\""<<names[i]<<"\",\"fundamental\":"<<s.fundamental<<",\"thdn_dbc\":"<<s.thdnDb
        <<",\"second_harmonic_dbc\":"<<s.evenDb<<",\"output_mean\":"<<s.dc<<",\"clip_node_mean_volts\":"<<s.clipDc<<",\"peak\":"<<s.peak<<'}'<<(i<3?",":"")<<'\n';}
    f<<"]}\n";f.flush();lab::require(bool(f),"Cannot finish measurements");
    std::ofstream quality(output/"frequency-and-level.csv");quality<<std::setprecision(10)
        <<"distortion,filter,input_peak_volts,frequency_hz,fundamental_linear,thdn_dbc,second_harmonic_dbc,output_mean\n";
    for(const auto& r:m.quality)quality<<r.distortion<<','<<r.filter<<','<<r.amplitude<<','<<r.bin*48000./ratlab::fftSize
        <<','<<r.spectrum.fundamental<<','<<r.spectrum.thdnDb<<','<<r.spectrum.evenDb<<','<<r.spectrum.dc<<'\n';
    quality.flush();lab::require(bool(quality),"Cannot write frequency/level report");
    std::ofstream curves(output/"diode-transfer.csv");curves<<std::setprecision(10)<<"case,thevenin_volts,clip_volts,port_ohms\n";
    for(const auto* name:{"healthy","diode_open"}){rat_detail::ClipStage stage;stage.prepare(192000,{.35f,.5f,.7f},ratlab::variation(name));
        double previous=-100;
        for(int i=-256;i<=256;++i){const double source=i/64.;const double value=stage.lookup(source);
            lab::require(value>=previous,"Nonmonotonic diode transfer");previous=value;
            curves<<name<<','<<source<<','<<value<<','<<stage.portR<<'\n';}}
    curves.flush();lab::require(bool(curves),"Cannot write diode transfer report");
}
void selfTest(){const auto m=measure();std::cout<<"Independent SPICE gain/phase/RMS/peak errors "<<m.reference.maxGainDb<<' '<<m.reference.maxPhaseDegrees<<' '<<m.reference.transientRms<<' '<<m.reference.transientPeak
    <<"; alias dBc 1x/4x "<<m.aliasOne<<'/'<<m.aliasFour<<"; open diode raw DC "<<m.faults[1].clipDc<<'\n';
    std::vector<std::complex<double>> pure(ratlab::fftSize);
    for(unsigned i=0;i<ratlab::fftSize;++i)pure[i]=std::sin(2*pi*171*i/ratlab::fftSize);
    ratlab::fft(pure);
    lab::require(std::abs(std::abs(pure[171])-ratlab::fftSize/2.)<1e-5,"FFT known coherent-sine amplitude");
    lab::require(std::abs(pure[342])<1e-7,"FFT pure-sine harmonic floor");
    lab::Wave w{44100,{0,.25f,-.5f,.8f}};const auto encoded=lab::encode(w);const auto decoded=lab::decode(encoded);
    lab::require(decoded.rate==44100&&decoded.samples.size()==4,"WAV round trip");
    for(int j=0;j<4;++j)lab::require(std::abs(decoded.samples[j]-w.samples[j])<=1./65536,"PCM quantization");
    for(int scenario=0;scenario<7;++scenario){auto bad=encoded;switch(scenario){case 0:bad.pop_back();break;case 1:bad[0]='X';break;case 2:bad[20]=3;break;case 3:bad[22]=2;break;case 4:bad[34]=24;break;case 5:bad[24]=1;break;case 6:bad[40]=255;break;}
        bool failed=false;try{lab::decode(bad);}catch(const std::exception&){failed=true;}lab::require(failed,"Invalid PCM accepted");}
}
void auditions(const std::filesystem::path& output,const std::filesystem::path& input){
    const auto wave=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    const std::string kind=input.empty()?"SYNTHETIC plucks/strums, 0.15 model-volts peak; not recorded guitar":"USER-PROVIDED mono PCM16; provenance unverified; 1 amplitude unit = 1 model volt";
    const std::array<std::string,11> names{"00_dry","01_healthy_low","02_healthy_medium","03_healthy_high","04_filter_bright","05_filter_dark","06_weak_input","07_strong_input","08_diode_open","09_ruetz","10_compensation"};
    const std::array<RatControls,11> controls{{{0,0,0},{.02f,.5f,.7f},{.35f,.5f,.7f},{.9f,.5f,.7f},{.35f,0,.7f},{.35f,1,.7f},{.35f,.5f,.7f},{.35f,.5f,.7f},{.35f,.5f,.7f},{.35f,.5f,.7f},{.35f,.5f,.7f}}};
    if(!input.empty()){
        std::vector<std::filesystem::path> files{"renders.csv","measurements.json","frequency-and-level.csv","diode-transfer.csv","LISTEN.txt"};for(const auto& n:names)files.emplace_back(n+".wav");
        for(const auto& f:files){const auto p=output/f;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)
            &&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite input recording");}}
    std::array<std::vector<float>,11> signals;signals[0]=wave.samples;
    for(unsigned i=1;i<signals.size();++i){const char* variation=i==8?"diode_open":i==9?"ruetz":i==10?"compensation":"healthy";
        Rat rat;lab::require(rat.configure(float(wave.rate),controls[i],ratlab::variation(variation)),"Audition configuration rejected");
        signals[i].resize(wave.samples.size());const float inputGain=i==6?.1f:i==7?2.f:1.f;
        for(std::size_t sample=0;sample<wave.samples.size();++sample)signals[i][sample]=rat.process(wave.samples[sample]*inputGain);}
    const double target=soundforge::rmsTarget(signals,std::pow(10.,-22./20),.89);
    std::filesystem::create_directories(output);std::ofstream manifest(output/"renders.csv");lab::require(bool(manifest),"Cannot write render manifest");manifest<<std::setprecision(10);
    manifest<<"# "<<kind<<"\n# rate="<<wave.rate<<", frames="<<wave.samples.size()<<"; 4x circuit; nominal FIR delay 48 base samples plus circuit delay\n";
    manifest<<"file,distortion,filter,volume,input_gain,positive_diode_connected,treble_leg_connected,feedback_comp_farads,raw_rms_dbfs,post_gain,rms_dbfs,peak_dbfs,mean\n";
    for(unsigned i=0;i<signals.size();++i){const auto c=ratlab::variation(i==8?"diode_open":i==9?"ruetz":i==10?"compensation":"healthy");
        const auto raw=stats(signals[i]);const double gain=soundforge::rmsGain(raw,target);
        for(auto& v:signals[i])v=static_cast<float>(v*gain);
        const auto final=stats(signals[i]);
        lab::require(final.peak<=.890001,"WAV headroom exceeded");lab::write(output/(names[i]+".wav"),{wave.rate,signals[i]});
        const auto p=controls[i];manifest<<names[i]<<".wav,"<<p.distortion<<','<<p.filter<<','<<p.volume<<','<<(i==6?.1:i==7?2:1)<<','<<c.positiveDiodeConnected<<','<<c.trebleLegConnected<<','<<c.feedbackCompFarads<<','<<db(raw.rms)<<','<<gain<<','<<db(final.rms)<<','<<db(final.peak)<<','<<final.mean<<'\n';}
    reports(output,measure());std::ofstream readme(output/"LISTEN.txt");readme<<kind<<"\nStart with dry, low/medium/high, bright/dark, weak/strong, then the three component variations.\nAll files whole-file RMS matched to "<<db(target)<<" dBFS before PCM16 quantization, peak target .89. No LUFS claim. Exact settings/gains in renders.csv; defaults in source/ADR-003.\nOpen diode removes a current branch. Ruetz disconnects the 47ohm/2.2uF feedback leg (intentional mod). Compensation increases feedback 100pF to 100nF (wrong-part experiment), not the separate 30pF pin capacitor.\nNo delay compensation. Input and output lengths match; include trailing silence for user recordings. Long physical RC recovery/rectification can produce transients.\nCircuit-derived reduced model; historical factory BOM, full LM308/JFET dynamics, real guitar identity, musical acceptance and hardware performance remain unverified.\n";
    manifest.flush();readme.flush();lab::require(bool(manifest)&&bool(readme),"Report write failed");std::cout<<"Created 11 LabRAT WAVs in "<<output<<'\n'<<kind<<'\n';
}
}
int main(int argc,char** argv){try{
    if(argc==2&&std::string(argv[1])=="--self-test"){selfTest();return 0;}
    std::filesystem::path output="rat-auditions",input;
    for(int i=1;i<argc;++i){const std::string key=argv[i];if((key=="--output"||key=="--input")&&i+1<argc){
        const std::string value=argv[++i];lab::require(!value.empty(),"Empty input/output path");(key=="--output"?output:input)=value;
    }else throw std::runtime_error("Usage: parable_rat_lab [--output DIR] [--input mono-pcm16.wav] | --self-test");}
    auditions(output,input);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
