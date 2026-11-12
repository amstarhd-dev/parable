// Host lab; borrowed PCM/stats/excitation and unchanged seven-line owned FFT.
#include "parable_rotary.h"
#include "lab_wav.h"
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <complex>
#include <iomanip>
#include <iostream>
namespace {
constexpr double pi=3.14159265358979323846;
inline void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}
using parable::dsp::Rotary;
float command(size_t frame,unsigned rate) {return frame>=rate&&frame<5*rate?1.f:0.f;}
struct Point {double t,h,m,b,capacity,required;};
struct Render {std::vector<float> full,horn,drum;std::vector<Point> points;double timeUs=0;uint32_t releases=0,captures=0;float maximumDelta=0;};
Render render(const lab::Wave& input,bool worn,std::ofstream* trace=nullptr) {
    Rotary model;lab::require(model.prepare(float(input.rate),{worn,1}),"Rotary prepare failed");
    Render r;r.full.resize(input.samples.size());r.horn.resize(input.samples.size());r.drum.resize(input.samples.size());
    for(size_t i=0;i<input.samples.size();++i) {
        const float speed=command(i,input.rate);r.full[i]=model.process(input.samples[i],speed);
        r.horn[i]=model.hornContribution();r.drum[i]=model.drumContribution();
        if(i)r.maximumDelta=std::max(r.maximumDelta,std::abs(r.full[i]-r.full[i-1]));
        if(trace&&(i%128==0||model.horn().released||model.horn().captured)) {
            const auto& h=model.horn();const auto& b=model.drum();const double t=double(i+1)/input.rate;
            *trace<<worn<<','<<input.rate<<','<<i<<','<<t<<','<<(speed?6.5:.8)<<','<<(speed?5:.6)<<','
                <<h.motorOmega<<','<<h.omega<<','<<h.motorAngle<<','<<h.angle<<','<<h.beltPhase<<','<<b.omega<<','<<b.angle<<','
                <<h.driveNm<<','<<h.loadNm<<','<<h.requiredNm<<','<<h.staticNm<<','<<h.beltNm<<','<<h.motorOmega-h.omega<<','
                <<h.slipping<<','<<h.released<<','<<h.captured<<','<<h.captureLossJ<<','<<model.hornDelaySamples()<<','<<model.drumDelaySamples()<<'\n';
            r.points.push_back({t,h.omega/(2*pi),h.motorOmega/(2*pi),b.omega/(2*pi),h.staticNm,std::abs(h.requiredNm)});
        }
    }
    r.releases=model.horn().releases;r.captures=model.horn().captures;
    lab::require(model.containmentEvents()==0,"Refuse to present numerical containment as a belt fault");
    lab::require(model.drum().releases==0,"The healthy drum must remain coupled");
    lab::require(worn?r.releases>=5&&r.captures>=5:r.releases==0,"Slip/capture evidence missing");
    model.reset();std::vector<float> timed(input.samples.size());const auto start=std::chrono::steady_clock::now();
    for(size_t i=0;i<timed.size();++i)timed[i]=model.process(input.samples[i],command(i,input.rate));
    r.timeUs=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(timed==r.full,"Reset changed repeat audio");return r;
}
void plot(const std::filesystem::path& path,const Render& h,const Render& f,double duration) {
    std::ofstream svg(path);lab::require(bool(svg),"Cannot create motion plot");
    svg<<std::setprecision(9)<<"<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1000 660\"><rect width=\"1000\" height=\"660\" fill=\"white\"/><g font-family=\"sans-serif\" font-size=\"15\" fill=\"#222\">"
       <<"<text x=\"60\" y=\"28\">SF-007: SAME slow (0-1s), fast (1-5s), slow command; reduced model, not measured hardware</text>"
       <<"<text x=\"60\" y=\"52\">Healthy green | worn horn red | its motor gray | unaffected drum blue | traction orange</text>";
    for(unsigned panel=0;panel<3;++panel) {
        const double top=80+panel*185,bottom=top+140;
        svg<<"<path d=\"M60 "<<top<<"V"<<bottom<<"H960\" fill=\"none\" stroke=\"#bbb\"/>";
        for(unsigned t=0;t<=unsigned(duration);++t) {const double x=60+900*t/duration;
            svg<<"<text x=\""<<x<<"\" y=\""<<bottom+20<<"\">"<<t<<"s</text>";
        }
        for(double t:{1.,5.}) {const double x=60+900*t/duration;svg<<"<path d=\"M"<<x<<' '<<top<<"V"<<bottom<<"\" stroke=\"#888\" stroke-dasharray=\"4 4\"/>";}
        svg<<"<text x=\"60\" y=\""<<top-8<<"\">"<<(panel==0?"Horn / motor speed, 0-7 Hz":panel==1?"Drum speed, 0-7 Hz (sample-exact between states)":"Worn horn static traction and required transfer, 0-.22 N m (pre-step)")<<"</text>";
        const auto line=[&](const std::vector<Point>& points,const char* color,int field,double scale) {
            svg<<"<polyline fill=\"none\" stroke=\""<<color<<"\" stroke-width=\"1.5\" points=\"";
            for(const auto& p:points) {const double value=field==0?p.h:field==1?p.m:field==2?p.b:field==3?p.capacity:p.required;
                svg<<60+900*p.t/duration<<','<<bottom-140*std::min(value/scale,1.)<<' ';}
            svg<<"\"/>";
        };
        if(panel==0) {line(h.points,"#148040",0,7);line(f.points,"#888",1,7);line(f.points,"#cb3030",0,7);}
        if(panel==1)line(h.points,"#1666c0",2,7);
        if(panel==2) {line(f.points,"#e58b10",3,.22);line(f.points,"#cb3030",4,.22);}
    }
    svg<<"</g></svg>\n";svg.flush();lab::require(bool(svg),"Cannot finish plot");
}
void spectrum(const std::filesystem::path& path,const lab::Wave& input,const Render& healthy,const Render& fault) {
    std::ofstream csv(path);lab::require(bool(csv),"Cannot create spectrum");
    csv<<std::setprecision(12)<<"source,frequency_hz,peak_amplitude,hann_window_frames,window_start_frame,rate_hz\n";
    constexpr size_t size=8192;const size_t start=3*input.rate;
    for(unsigned variant=0;variant<3;++variant) {
        const auto& signal=variant==0?input.samples:variant==1?healthy.full:fault.full;
        std::vector<std::complex<double>> bins(size);double windowSum=0;
        for(size_t i=0;i<size;++i) {const double w=.5-.5*std::cos(2*pi*i/(size-1));windowSum+=w;bins[i]=signal[start+i]*w;}
        fft(bins);
        for(size_t i=0;i<=size/2;++i)csv<<(variant==0?"dry":variant==1?"healthy":"worn_horn_belt")<<','<<i*double(input.rate)/size<<','<<std::abs(bins[i])*(i==0||i==size/2?1:2)/windowSum<<','<<size<<','<<start<<','<<input.rate<<'\n';
    }
    csv.flush();lab::require(bool(csv),"Cannot finish spectrum");
}
void aliasAudit(const std::filesystem::path& path) {
    std::ofstream csv(path);lab::require(bool(csv),"Cannot create alias audit");
    csv<<std::setprecision(12)<<"input_hz,native_rate,reference_rate,native_window_frames,reference_window_frames,reference_power_above_native_nyquist_fraction,reference_out_of_band_db_total,native_window_rms,reference_window_rms\n";
    for(double frequency:{4000.,23800.}) {
        double nativeRms=0,referenceRms=0,fraction=0;
        for(unsigned rate:{48000u,96000u}) {
            Rotary model;lab::require(model.prepare(float(rate),{false,1}),"Alias audit prepare failed");
            const size_t size=rate==48000?32768:65536,start=3*rate;
            std::vector<std::complex<double>> bins(size);double energy=0;
            for(size_t i=0;i<start+size;++i) {
                const float y=model.process(float(.15*std::sin(2*pi*frequency*i/rate)),command(i,rate));
                if(i>=start) {const size_t n=i-start;const double w=.5-.5*std::cos(2*pi*n/(size-1));bins[n]=y*w;energy+=double(y)*y;}
            }
            if(rate==48000)nativeRms=std::sqrt(energy/size);
            else {
                referenceRms=std::sqrt(energy/size);fft(bins);double total=0,above=0;
                for(size_t bin=1;bin<size/2;++bin) {const double e=std::norm(bins[bin]);total+=e;if(bin>size/4)above+=e;}
                fraction=above/total;
            }
            lab::require(model.containmentEvents()==0,"Alias probe used numerical containment");
        }
        csv<<frequency<<",48000,96000,32768,65536,"<<fraction<<','<<soundforge::db(std::sqrt(fraction))<<','<<nativeRms<<','<<referenceRms<<'\n';
    }
    csv.flush();lab::require(bool(csv),"Cannot finish alias audit");
}
void audition(const std::filesystem::path& input,const std::filesystem::path& output) {
    auto source=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    lab::require(source.samples.size()>=6*source.rate&&source.samples.size()<=24*source.rate,"Input must be 6-24 s to exercise the fixed 1/5s command");
    const auto original=soundforge::stats(source.samples);lab::require(original.peak>0,"Cannot calibrate silent input");
    const std::vector<std::string> names={"00_dry_raw.wav","01_healthy_raw.wav","02_worn_belt_raw.wav","03_dry_matched.wav","04_healthy_matched.wav","05_worn_belt_matched.wav","06_healthy_horn_matched.wav","07_worn_horn_matched.wav","08_unaffected_drum_raw.wav","09_probe_input.wav","10_probe_healthy_raw.wav","11_probe_worn_raw.wav"};
    auto paths=names;for(const char* p:{"renders.csv","motion.csv","motion.svg","spectrum.csv","alias.csv","comparison.csv","settings.json","LISTEN.txt"})paths.push_back(p);
    if(!input.empty())for(const auto& name:paths) {const auto p=output/name;lab::require(std::filesystem::weakly_canonical(p)!=std::filesystem::canonical(input)&&(!std::filesystem::exists(p)||!std::filesystem::equivalent(p,input)),"Output would overwrite input");}
    const double calibration=.25/original.peak;for(auto& x:source.samples)x=float(x*calibration);
    source.samples.resize(source.samples.size()+2*source.rate,0);
    std::filesystem::create_directories(output);std::ofstream trace(output/"motion.csv");lab::require(bool(trace),"Cannot create trace");
    trace<<std::setprecision(15)<<"worn,rate_hz,frame,end_seconds,command_horn_hz,command_drum_hz,motor_rad_s,horn_rad_s,motor_angle_rad,horn_angle_rad,belt_phase_rad,drum_rad_s,drum_angle_rad,pre_drive_Nm,pre_load_Nm,pre_required_Nm,pre_static_capacity_Nm,pre_belt_Nm,relative_rad_s,slipping,released,captured,capture_loss_J,horn_delay_samples,drum_delay_samples\n";
    const auto healthy=render(source,false,&trace),fault=render(source,true,&trace);
    lab::require(healthy.drum==fault.drum,"Fault altered the unaffected drum");
    lab::Wave probe;probe.rate=48000;probe.samples.resize(8*probe.rate);
    for(size_t i=0;i<probe.samples.size();++i) {
        const double t=double(i)/probe.rate;
        probe.samples[i]=float(t<.1?.2*std::sin(2*pi*3000*t)*std::sin(pi*t/.1):t<.5?0:.12*std::sin(2*pi*997*t)+.08*std::sin(2*pi*97*t));
    }
    const auto probeH=render(probe,false),probeF=render(probe,true);
    const std::vector<std::vector<float>> music={source.samples,healthy.full,fault.full};
    const double musicTarget=soundforge::rmsTarget(music,std::pow(10.,-23./20),.89);
    const std::array<std::vector<float>,2> horns={healthy.horn,fault.horn};
    const double hornTarget=soundforge::rmsTarget(horns,std::pow(10.,-26./20),.89);
    std::ofstream receipt(output/"renders.csv");lab::require(bool(receipt),"Cannot create levels");
    receipt<<std::setprecision(12)<<"file,source_kind,rate,frames,software_input_gain,raw_rms,raw_peak,raw_dc,offline_gain,final_rms,final_peak,final_dc,horn_releases,horn_captures,max_raw_sample_delta,host_processing_us\n";
    std::vector<std::vector<float>> primaryMatched;
    for(size_t i=0;i<names.size();++i) {
        const auto& raw=i<6?music[i%3]:i==6?healthy.horn:i==7?fault.horn:i==8?healthy.drum:i==9?probe.samples:i==10?probeH.full:probeF.full;
        const auto stats=soundforge::stats(raw);const double gain=i>=3&&i<=7?soundforge::rmsGain(stats,i<6?musicTarget:hornTarget):1;
        auto signal=raw;for(auto& x:signal)x=float(x*gain);const auto final=soundforge::stats(signal);
        lab::require(final.peak<.999&&((i<3||i>7)||final.peak<=.890001),"Output clipped or exceeded matching headroom");
        const auto& run=i==2||i==5||i==7?fault:i==11?probeF:i==10?probeH:healthy;
        const bool dry=i==0||i==3||i==9;float rawDelta=0;
        for(size_t n=1;n<raw.size();++n)rawDelta=std::max(rawDelta,std::abs(raw[n]-raw[n-1]));
        const unsigned rate=i>=9?probe.rate:source.rate;
        lab::write(output/names[i],{rate,signal});if(i==4||i==5)primaryMatched.push_back(signal);
        receipt<<names[i]<<','<<(i>=9?"owned-synthetic-probe":input.empty()?"owned-synthetic-plucks":"external-recording")<<','<<rate<<','<<signal.size()<<','<<(i>=9?1:calibration)<<','
            <<stats.rms<<','<<stats.peak<<','<<stats.mean<<','<<gain<<','<<final.rms<<','<<final.peak<<','<<final.mean<<','<<(dry?0:run.releases)<<','<<(dry?0:run.captures)<<','<<rawDelta<<','<<(dry?0:run.timeUs)<<'\n';
    }
    double hh=0,ff=0,dot=0,difference=0;
    for(size_t i=0;i<primaryMatched[0].size();++i) {const double h=primaryMatched[0][i],f=primaryMatched[1][i];hh+=h*h;ff+=f*f;dot+=h*f;difference+=(h-f)*(h-f);}
    lab::require(std::sqrt(difference/primaryMatched[0].size())>.001,"Matched outputs are not distinguishable by waveform");
    std::ofstream comparison(output/"comparison.csv"),settings(output/"settings.json"),notes(output/"LISTEN.txt");lab::require(bool(comparison)&&bool(settings)&&bool(notes),"Cannot create receipts");
    comparison<<std::setprecision(12)<<"matched_correlation,matched_difference_rms,drum_sample_exact,healthy_releases,fault_releases,fault_captures,containment_events\n"<<dot/std::sqrt(hh*ff)<<','<<std::sqrt(difference/primaryMatched[0].size())<<",true,0,"<<fault.releases<<','<<fault.captures<<",0\n";
    settings<<std::setprecision(12)<<"{\"standalone_not_canonical_patch\":true,\"rate_hz\":"<<source.rate<<",\"software_input_gain\":"<<calibration<<",\"input_peak\":0.25,\"mix\":1,\"native_wet_gain\":0.3333333333333333,\"fast_from_seconds\":1,\"slow_from_seconds\":5,\"horn_wear_healthy\":0,\"horn_wear_fault\":0.96,\"belt_laps_per_pulley_turn\":0.25,\"static_horn_Nm\":0.22,\"kinetic_static_ratio\":0.7,\"horn_Jm\":0.0002,\"horn_Jr\":0.002,\"drum_Jm\":0.001,\"drum_Jr\":0.015,\"speed_of_sound_m_s\":343,\"receiver_m\":1,\"horn_radius_m\":0.18,\"drum_effective_radius_m\":0.091,\"crossover_hz\":800,\"horn_dark_bright_hz\":[1500,8000],\"delay_cells_per_band\":384,\"trig_table_points\":257,\"processor_bytes\":"<<sizeof(Rotary)<<"}\n";
    notes<<"SF-007 reduced two-rotor rotary speaker, not a calibrated Leslie or approved Moon affliction policy.\n"
         <<"Compare 04_healthy_matched.wav with 05_worn_belt_matched.wav. Same input, commands, settings, RMS -23dBFS target, gains in renders.csv. 01/02 raw common gain1; 00/03 dry. Whole-file RMS, not LUFS/perceptual approval.\n"
         <<"06/07 isolate the actual horn contributions, separately RMS-matched at -26dBFS target. 08 is the drum raw; both processed states produced that exact drum signal. 09 is the OWNED SYNTHETIC transient/steady input and 10/11 its actual processed outputs, not guitar.\n"
         <<"Commands slow0-1s, fast1-5s, then slow. Listen at and after those points. Fault is ONE localized worn horn-belt section, wear0->.96. Traction depends on belt material phase advected by the motor; torque/relative velocity cause release and momentum-conserving recapture. No random/noisy LFO, phase reset or output dropout.\n"
         <<"motion.svg summarizes actual motion. motion.csv stamps END-of-sample velocities/angles at (frame+1)/rate; torque/capacity columns are evaluated PRE-step. Normal trace decimated128frames plus every release/capture. Angles wrap2pi; do not interpret wrapped phase jumps as motor resets.\n"
         <<"800Hz split, different horn/drum directivity, angle coloration and fractional propagation delay in mono. Radius/receiver/torque/inertia values are explicit uncalibrated model assumptions; no exact cabinet, motor electrical circuit, belt material or mic response. No stereo/room/noise/saturation/oversampling. Linear audio interpolation has known HF attenuation/alias limits.\n"
         <<"alias.csv separately renders 4k/23.8k probes at48/96k. The96k spectrum's energy above24k exposes what can alias at48k; this is an alias-risk experiment, not a native alias-rejection/noise-floor measurement. Delivered guitar is never oversampled/resampled.\n"
         <<"Input peak .25 normalized, NOT physical voltage; equal2s silent tail, no resampling. Output is wet only with fixed native1/3 gain to bound sum; offline matching is separate.\n"
         <<"Source rights/hash/card in artifact. Optional CLI input has no inferred license; default plucks are synthetic. Founder musical approval and physical Seed3 timing/IO remain unproven. Host timing is a separate full-processor reset pass, excluding prepare/IO/statistics and including output stores.\n";
    spectrum(output/"spectrum.csv",probe,probeH,probeF);aliasAudit(output/"alias.csv");plot(output/"motion.svg",healthy,fault,double(source.samples.size())/source.rate);
    for(auto* s:{&trace,&receipt,&comparison,&settings,&notes}){s->flush();lab::require(bool(*s),"Cannot finish listening evidence");}
    std::cout<<"Created 12 rotary music/band/probe WAVs, mechanical trace/plot and listening receipts in "<<output<<'\n';
}
}
int main(int argc,char** argv) {try {
    std::filesystem::path input,output;
    for(int i=1;i<argc;++i) {const std::string key=argv[i];lab::require((key=="--input"||key=="--output")&&i+1<argc,"Use --output DIR [--input mono-pcm16.wav]");const std::string value=argv[++i];lab::require(!value.empty(),"Empty path");(key=="--input"?input:output)=value;}
    lab::require(!output.empty(),"Explicit output required");audition(input,output);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
