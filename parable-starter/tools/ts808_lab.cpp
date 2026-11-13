// Host-only audio/electrical evidence; the same native Ts808 creates every output.
#include "parable_ts808.h"
#include "ts808_stimulus.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
using parable::dsp::Ts808;
namespace {
constexpr double pi=ts808lab::pi;
// Unchanged owned seven-line FFT, also used by the merged laboratories.
void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
struct Point {double time=0,clip=0,output=0;};
struct Render {
    std::vector<float> y;std::vector<Point> points;
    double us=0,meanEvaluations=0,clipMin=99,clipMax=-99,toneMin=99,toneMax=-99,maxU=0,d1=0,d2=0;
    uint32_t maximum=0,rails=0,slews=0;
};
Render render(const lab::Wave& source,bool fault,const char* label,std::ofstream& trace,std::ofstream& polarity) {
    Ts808 m;lab::require(m.prepare(float(source.rate),{.5f,.5f,.5f,fault}),"TS808 prepare failed");
    Render r;r.y.resize(source.samples.size());const bool periodic=std::string(label)=="sine"||std::string(label)=="triangle";
    for(std::size_t n=0;n<r.y.size();++n) {
        r.y[n]=m.process(source.samples[n]);const auto& nodes=m.nodes();
        r.clipMin=std::min(r.clipMin,nodes[Ts808::Clip]);r.clipMax=std::max(r.clipMax,nodes[Ts808::Clip]);
        r.toneMin=std::min(r.toneMin,nodes[Ts808::ToneOut]);r.toneMax=std::max(r.toneMax,nodes[Ts808::ToneOut]);
        r.maxU=std::max({r.maxU,std::abs(m.internalVolts(0)),std::abs(m.internalVolts(1))});
        r.d1=std::max(r.d1,std::abs(m.diodeAmps(0)));r.d2=std::max(r.d2,std::abs(m.diodeAmps(1)));
        if(n%128==0) {
            trace<<label<<','<<fault<<','<<source.rate<<','<<n<<','<<(double(n)+.9375)/source.rate<<','<<source.samples[n]<<','<<r.y[n];
            for(double v:nodes)trace<<','<<v;
            for(unsigned c=0;c<9;++c)trace<<','<<m.capacitorVolts(c);
            trace<<','<<m.internalVolts(0)<<','<<m.internalVolts(1)<<','<<m.diodeAmps(0)<<','<<m.diodeAmps(1)<<','<<m.residualVolts()<<','<<m.failures()<<'\n';
        }
        const double time=double(n)/source.rate;
        if(periodic&&time>=.08&&time<.088) {
            polarity<<label<<','<<fault<<','<<time<<','<<(double(n)+.9375)/source.rate<<','<<source.samples[n]<<','<<r.y[n]<<','<<nodes[Ts808::Plus]<<','<<nodes[Ts808::Minus]<<','<<nodes[Ts808::Clip]<<','<<nodes[Ts808::ToneOut]<<','<<nodes[Ts808::Jack]<<','<<m.internalVolts(0)<<','<<m.internalVolts(1)<<','<<m.diodeAmps(0)<<','<<m.diodeAmps(1)<<'\n';
            if(std::string(label)=="sine")r.points.push_back({time,nodes[Ts808::Clip]-4.5,r.y[n]});
        }
    }
    lab::require(m.failures()==0&&m.outputClamps()==0&&m.inputClamps()==0&&m.invalidInputs()==0,"Refuse numerical containment as a diode fault");
    lab::require(!fault||r.d2==0,"Open D2 must have exactly zero current");
    r.maximum=m.maximumEvaluations();r.meanEvaluations=double(m.evaluations())/m.solves();r.rails=m.railSamples();r.slews=m.slewSamples();
    m.reset();std::vector<float> timed(r.y.size());const auto start=std::chrono::steady_clock::now();
    for(std::size_t n=0;n<timed.size();++n)timed[n]=m.process(source.samples[n]);
    r.us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(timed==r.y&&m.failures()==0,"Reset/repeat changed audio");return r;
}
void spectrum(std::ofstream& csv,const char* label,unsigned rate,const std::vector<float>& signal) {
    constexpr std::size_t size=8192;const std::size_t start=std::min<std::size_t>(rate,signal.size()-size);
    std::vector<std::complex<double>> bins(size);double sum=0;
    for(std::size_t i=0;i<size;++i){const double w=.5-.5*std::cos(2*pi*i/(size-1));sum+=w;bins[i]=signal[start+i]*w;}
    fft(bins);
    for(std::size_t i=0;i<=size/2;++i)csv<<label<<','<<double(i)*rate/size<<','<<std::abs(bins[i])*(i==0||i==size/2?1:2)/sum<<','<<rate<<','<<start<<','<<size<<'\n';
}
void plot(const std::filesystem::path& path,const Render& h,const Render& f) {
    std::ofstream svg(path);lab::require(bool(svg),"Cannot write polarity plot");
    svg<<std::setprecision(9)<<"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1000 500\"><rect width=\"1000\" height=\"500\" fill=\"white\"/><g font-family=\"sans-serif\" font-size=\"16\"><text x=\"55\" y=\"30\">TS808: healthy green / D2 truly open red; SAME balanced 0.2V 1kHz source</text>";
    for(unsigned panel=0;panel<2;++panel) {
        const double center=panel?365:150,scale=panel?300:30;
        svg<<"<text x=\"55\" y=\""<<center-100<<"\">"<<(panel?"Filtered normalized output, RAW common gain (frame time)":"Clipping op-amp output minus4.5V (last internalphase15)")<<"</text><path d=\"M55 "<<center<<"H950\" stroke=\"#bbb\"/>";
        for(unsigned fault=0;fault<2;++fault) {
            svg<<"<polyline fill=\"none\" stroke=\""<<(fault?"#ce3030":"#148040")<<"\" stroke-width=\"1.7\" points=\"";
            for(const auto& p:(fault?f:h).points)svg<<55+895*(p.time-.08)/.008<<','<<center-scale*(panel?p.output:p.clip)<<' ';
            svg<<"\"/>";
        }
    }
    svg<<"<text x=\"55\" y=\"485\">80–88ms; panels have different units and FIR timing. No physical-pedal calibration.</text></g></svg>\n";
    svg.flush();lab::require(bool(svg),"Cannot finish plot");
}
void audition(const std::filesystem::path& input,const std::filesystem::path& out) {
    auto music=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    lab::require(music.samples.size()>=music.rate&&music.samples.size()<=28*music.rate,"Input must be1–28s, plus2s tail within shared30s WAV bound");
    const auto raw=soundforge::stats(music.samples);lab::require(raw.peak>0,"Cannot calibrate silence");
    std::vector<std::string> names={"00_dry_raw.wav","01_healthy_raw.wav","02_D2_open_raw.wav","03_dry_matched.wav","04_healthy_matched.wav","05_D2_open_matched.wav"};
    for(const char* k:{"attacks","long_note","sine","triangle"})for(const char* v:{"input_raw","healthy_raw","D2_open_raw","healthy_common","D2_open_common"})names.push_back(std::string(k)+"_"+v+".wav");
    auto paths=names;for(const char* n:{"renders.csv","state.csv","polarity.csv","polarity.svg","spectrum.csv","stage_stats.csv","envelopes.csv","settings.json","LISTEN.txt"})paths.push_back(n);
    if(!input.empty())for(const auto& name:paths) {
        const auto p=out/name;
        lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output aliases input source");
    }
    const double calibration=.2/raw.peak;for(auto& x:music.samples)x=float(x*calibration);
    music.samples.resize(music.samples.size()+2*music.rate,0);std::filesystem::create_directories(out);
    std::ofstream trace(out/"state.csv"),polarity(out/"polarity.csv"),receipt(out/"renders.csv"),spectra(out/"spectrum.csv"),stages(out/"stage_stats.csv"),envelopes(out/"envelopes.csv");
    for(auto* f:{&trace,&polarity,&receipt,&spectra,&stages,&envelopes}){lab::require(bool(*f),"Cannot write evidence");*f<<std::setprecision(15);}
    trace<<"source,fault,rate_hz,frame,last_phase15_s,dry_input,filtered_output,b1,e1,plus,minus,clip,tone_plus,tone_minus,tone_out,level_top,b3,e3,jack";
    for(unsigned c=1;c<=9;++c)trace<<",C"<<c<<"_V";
    trace<<",u1_relative4V5,u2_relative4V5,D1_A,D2_A,port_residual_V,solver_failures\n";
    polarity<<"source,fault,frame_s,last_phase15_s,input,filtered_output,plus_V,minus_V,clip_V,tone_V,jack_V,u1_V,u2_V,D1_A,D2_A\n";
    receipt<<"file,source_kind,rate_hz,frames,input_gain,offline_gain,raw_rms,raw_peak,final_rms,final_peak,dc,crest_db,clipped_samples,solver_failures,numerical_output_containment,mean_evaluations,max_evaluations,host_processor_us\n";
    spectra<<"signal,frequency_hz,peak_amplitude,rate_hz,start_frame,hann_frames\n";
    stages<<"source,fault,clip_min_V,clip_max_V,tone_min_V,tone_max_V,max_internal_relative_V,D1_peak_A,D2_peak_A,physical_rail_samples,physical_slew_samples\n";
    envelopes<<"fault,attack,start_s,end_s,filtered_normalized_rms\n";
    auto run=[&](const lab::Wave& source,bool fault,const char* label) {
        auto r=render(source,fault,label,trace,polarity);
        stages<<label<<','<<fault<<','<<r.clipMin<<','<<r.clipMax<<','<<r.toneMin<<','<<r.toneMax<<','<<r.maxU<<','<<r.d1<<','<<r.d2<<','<<r.rails<<','<<r.slews<<'\n';return r;
    };
    const auto h=run(music,false,"music"),f=run(music,true,"music");
    const std::vector<std::vector<float>> signals={music.samples,h.y,f.y};
    double target=soundforge::rmsTarget(signals,std::pow(10.,-23./20),.89);
    for(const auto& s:signals)target=std::min(target,16*soundforge::stats(s).rms);
    auto write=[&](const std::string& name,const lab::Wave& source,const std::vector<float>& signal,const Render* r,const char* kind,double inputGain,double gain) {
        auto y=signal;for(auto& x:y)x=float(x*gain);const auto a=soundforge::stats(signal),b=soundforge::stats(y);
        lab::require(b.peak<.999,"Refuse clipped listening output");lab::write(out/name,{source.rate,y});
        receipt<<name<<','<<kind<<','<<source.rate<<','<<y.size()<<','<<inputGain<<','<<gain<<','<<a.rms<<','<<a.peak<<','<<b.rms<<','<<b.peak<<','<<b.mean<<','<<soundforge::db(b.peak/std::max(b.rms,1e-30))<<",0,0,0,"<<(r?r->meanEvaluations:0)<<','<<(r?r->maximum:0)<<','<<(r?r->us:0)<<'\n';
    };
    for(unsigned n=0;n<6;++n)write(names[n],music,signals[n%3],n%3==0?nullptr:n%3==1?&h:&f,input.empty()?"owned-synthetic":"caller-provided-PCM16",calibration,n<3?1:soundforge::rmsGain(soundforge::stats(signals[n%3]),target));
    spectrum(spectra,"music_dry",music.rate,music.samples);spectrum(spectra,"music_healthy",music.rate,h.y);spectrum(spectra,"music_D2_open",music.rate,f.y);
    for(unsigned k=0;k<4;++k) {
        const char* label=k==0?"attacks":k==1?"long_note":k==2?"sine":"triangle";
        lab::Wave source;source.rate=48000;source.samples.resize(k==0?48000:k==1?96000:24000);
        for(std::size_t n=0;n<source.samples.size();++n)source.samples[n]=k==0?ts808lab::attacks(double(n)/source.rate):k==1?ts808lab::note(double(n)/source.rate):ts808lab::periodic(double(n)/source.rate,k==2?.2:.3,1000,k==3);
        const auto a=run(source,false,label),b=run(source,true,label);
        const double common=std::min(8.,.89/std::max(soundforge::stats(a.y).peak,soundforge::stats(b.y).peak));
        write(names[6+k*5],source,source.samples,nullptr,"owned-controlled",1,1);
        write(names[7+k*5],source,a.y,&a,"owned-controlled",1,1);write(names[8+k*5],source,b.y,&b,"owned-controlled",1,1);
        write(names[9+k*5],source,a.y,&a,"owned-controlled",1,common);write(names[10+k*5],source,b.y,&b,"owned-controlled",1,common);
        if(k==0)for(unsigned fault=0;fault<2;++fault)for(unsigned attack=0;attack<3;++attack) {
            const double start=.03+.22*attack,end=start+.1;double energy=0;std::size_t count=0;
            for(std::size_t n=std::size_t(start*source.rate);n<std::size_t(end*source.rate);++n){const auto v=(fault?b:a).y[n];energy+=double(v)*v;++count;}
            envelopes<<fault<<','<<attack<<','<<start<<','<<end<<','<<std::sqrt(energy/count)<<'\n';
        }
        if(k==2)plot(out/"polarity.svg",a,b);
    }
    std::ofstream settings(out/"settings.json"),notes(out/"LISTEN.txt");lab::require(bool(settings)&&bool(notes),"Cannot write settings");
    settings<<std::setprecision(15)<<"{\"standalone_only\":true,\"source_kind\":\""<<(input.empty()?"owned-synthetic":"caller-provided-PCM16")<<"\",\"input_gain\":"<<calibration<<",\"input_peak_model_V\":0.2,\"drive\":0.5,\"tone\":0.5,\"level\":0.5,\"healthy_D2\":\"connected\",\"fault_D2\":\"true_open_anode_clip_cathode_minus\",\"supply_V\":9,\"virtual_ground_V\":4.5,\"source_ohms\":1000,\"load_ohms\":1000000,\"output_V_per_unit\":4,\"oversampling\":16,\"FIR_taps\":769,\"nominal_FIR_delay_base_frames\":48,\"maximum_offline_music_gain\":16,\"state_bytes\":"<<sizeof(Ts808)<<"}\n";
    notes<<"SF-010: experimental source-defined TS808; full coupled diode FEEDBACK, two emitter followers and active tone. No calibrated Ibanez TS808/vintageJRC4558 or founder approval.\n"
         <<"Start04_healthy_matched.wav /05_D2_open_matched.wav; compare01/02 RAW too. Whole-file RMS matching, not LUFS; gain capped16x, peak.89. All actual gains/levels/crest inrenders.csv (float before PCM16 quantization). Controlled *_common files use SAME gain for healthy/fault≤8.\n"
         <<"D2 output(anode)->minus(cathode) is a true open: its current/Jacobian branch disappears. Other diode, R6/P1,C4, op-amp, controls and gains remain. Finite resistor feedback remains; positive excursion may meet the modeled op-amp rail. No after-output asymmetry/gate/noise/severity. Whether useful is a listening question.\n"
         <<"New primary DI rights/original/excerpt are supplied by CI; arbitrary --input does not imply provenance. Softwarepeak.2modelV and jackVolts/4 are not measured pickup/Seed3 calibration. Same source/settings plus2s silence. attacks/long_note/sine/triangle are OWNED SYNTHETIC, balanced bipolar inputs.\n"
         <<"state.csv is every128frames at last internalphase15 time(frame+.9375)/rate. Filtered emitted WAV decimates phase0 with nominal48frame FIR delay. polarity.csv/plot show dense80-88ms physical/filtered waveforms with separate timestamps; do not infer aligned phase from the two panels.\n"
         <<"Physical op-amp rail/slew samples are model behavior and separately counted; numerical solver/input/output fallback must beZERO in ordinary evidence. Closed-form piecewise-linear amplifier elimination preserves the original dominant-pole/slew/rail law inside feedback. Nominal silicon and ideal9V/4.5V supply; intrinsic charge, Early/high-injection, IC bias/offset/noise/current limit and detailed saturation recovery omitted.\n"
         <<"Host timing is a separate reset/repeat full-process pass, excluding prepare/trace/IO/analysis. No measured Daisy cycles/deadline, boot/audio/hardware or physical-pedal fidelity.\n";
    for(auto* p:{&trace,&polarity,&receipt,&spectra,&stages,&envelopes,&settings,&notes}){p->flush();lab::require(bool(*p),"Cannot finish receipts");}
    std::cout<<"Created26 TS808 WAVs, true D2-open state/current/polarity evidence and gains in "<<out<<'\n';
}
}
int main(int argc,char** argv){try {
    std::filesystem::path input,out;bool si=false,so=false;
    for(int i=1;i<argc;++i){const std::string k=argv[i];lab::require((k=="--input"||k=="--output")&&i+1<argc,"Use --output DIR [--input monoPCM16.wav]");const std::string v=argv[++i];lab::require(!v.empty(),"Empty path");bool& seen=k=="--input"?si:so;lab::require(!seen,"Duplicate argument");seen=true;(k=="--input"?input:out)=v;}
    lab::require(so,"Explicit output required");audition(input,out);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
