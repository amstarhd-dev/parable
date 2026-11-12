// Host only: existing PCM/stats/FFT, actual Tape processor, no Engine integration.
#include "parable_tape.h"
#include "lab_wav.h"
#include <complex>
#include "soundforge/analysis.h"
#include "soundforge/excitation.h"
#include <chrono>
#include <iomanip>
#include <iostream>

namespace {
constexpr double pi=3.14159265358979323846;
// Narrow unchanged copy of the owned host FFT from rat_measure.h. Keeping its
// protected source intact avoids pulling RAT fixture/solver APIs into this lab.
inline void fft(std::vector<std::complex<double>>& a) {
    const auto n=a.size();
    for(std::size_t i=1,j=0;i<n;++i){auto bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t length=2;length<=n;length<<=1){const auto root=std::polar(1.,-2*pi/length);
        for(std::size_t i=0;i<n;i+=length){std::complex<double> w=1;for(std::size_t j=0;j<length/2;++j){
            const auto u=a[i+j],v=a[i+j+length/2]*w;a[i+j]=u+v;a[i+j+length/2]=u-v;w*=root;}}}
}

using parable::dsp::Tape;
using parable::dsp::TapeConfig;
struct Run { std::vector<float> samples; double processingUs=0; };
Run render(const lab::Wave& input,TapeConfig config,std::ofstream* trace=nullptr) {
    std::vector<float> memory(Tape::requiredCells(float(input.rate),config));Tape tape;
    lab::require(tape.prepare(float(input.rate),config,memory.data(),memory.size()),"Tape preparation failed");
    Run out;out.samples.resize(input.samples.size());
    for(size_t i=0;i<input.samples.size();++i) {
        out.samples[i]=tape.process(input.samples[i]);
        if(trace && (i%128==0 || tape.probe().writeCell==0)) {
            const auto p=tape.probe();
            *trace<<config.eraseRetention<<','<<config.feedback<<','<<i<<','<<double(i)/input.rate<<','<<p.pass<<','<<p.writeCell<<','<<p.eraseCell<<','<<p.playCell<<','
                  <<input.samples[i]<<','<<p.eraseBefore<<','<<p.eraseAfter<<','<<p.writeBefore<<','<<p.recordValue<<','<<p.written<<','<<p.playMagnetization<<','<<p.replay<<'\n';
        }
    }
    // A separate reset/timed pass excludes allocation, preparation, stats and IO.
    tape.reset();std::vector<float> timed(out.samples.size());
    const auto start=std::chrono::steady_clock::now();
    for(size_t i=0;i<input.samples.size();++i)timed[i]=tape.process(input.samples[i]);
    out.processingUs=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    lab::require(timed==out.samples,"Reset did not reproduce exact processing");
    return out;
}
void spectrum(const std::filesystem::path& path) {
    std::ofstream csv(path);lab::require(bool(csv),"Cannot write spectrum");
    csv<<std::setprecision(12)<<"rate,input_peak,frequency_hz,fundamental,third_or_folded_third_dbc,dc,feedback,retention,scope\n";
    constexpr size_t size=4096;
    for(float level:{.05f,.25f})for(unsigned bin:{32u,256u,768u}) {
        TapeConfig config;config.mix=1;config.feedback=0;
        const size_t delay=14400;lab::Wave input;input.rate=48000;input.samples.resize(delay+3*size);
        for(size_t i=0;i<input.samples.size();++i)input.samples[i]=float(level*std::sin(2*pi*bin*i/size));
        auto out=render(input,config);std::vector<std::complex<double>> bins(size);
        for(size_t i=0;i<size;++i)bins[i]=out.samples[out.samples.size()-size+i];
        fft(bins);const double fundamental=2*std::abs(bins[bin])/size;
        unsigned third=(3*bin)%size;if(third>size/2)third=size-third;
        csv<<48000<<','<<level<<','<<bin*48000./size<<','<<fundamental<<','<<soundforge::db((2*std::abs(bins[third])/size)/fundamental)<<','<<bins[0].real()/size
           <<",0,0,first-pass-identical-for-both-erase-states\n";
    }
    csv.flush();lab::require(bool(csv),"Cannot finish spectrum");
}
void audition(const std::filesystem::path& input,const std::filesystem::path& output) {
    auto source=input.empty()?soundforge::syntheticPlucks():lab::read(input);
    lab::require(source.samples.size()<=24*source.rate && source.samples.size()>=source.rate/10,"Input must be .1 to 24 seconds");
    const auto before=soundforge::stats(source.samples);lab::require(before.peak>0,"Silent input cannot be calibrated");
    const std::vector<std::string> music={"00_dry_raw.wav","01_healthy_raw.wav","02_weak_erase_raw.wav","03_dry_matched.wav","04_healthy_matched.wav","05_weak_erase_matched.wav"};
    const std::vector<std::string> proof={"10_AB_input.wav","11_AB_healthy_wet.wav","12_AB_weak_erase_wet.wav","13_B_only_weak_erase_wet.wav","14_A_ghost_isolated.wav"};
    auto paths=music;paths.insert(paths.end(),proof.begin(),proof.end());
    for(const char* p:{"renders.csv","medium.csv","ghost.csv","ghost-spectrum.csv","spectrum.csv","settings.json","LISTEN.txt"})paths.push_back(p);
    if(!input.empty())for(const auto& name:paths) {
        const auto path=output/name;
        lab::require(std::filesystem::weakly_canonical(path)!=std::filesystem::canonical(input)
            &&(!std::filesystem::exists(path)||!std::filesystem::equivalent(path,input)),"Output would overwrite input");
    }
    const double calibration=.25/before.peak;
    for(auto& x:source.samples)x=float(x*calibration);
    const auto phraseLength=std::min<size_t>(source.rate,source.samples.size()/2),bStart=source.samples.size()/2;
    const auto originalFrames=source.samples.size();
    auto musical=source;musical.samples.resize(musical.samples.size()+size_t(std::lround(4.8*source.rate)),0);
    std::filesystem::create_directories(output);
    std::ofstream trace(output/"medium.csv");lab::require(bool(trace),"Cannot create medium trace");
    trace<<std::setprecision(12)<<"retention,feedback,sample,seconds,write_pass,write_cell,erase_cell,play_cell,input,erase_before,erase_after,write_before,record_value,written,play_magnetization,replay\n";
    TapeConfig c;c.feedback=.65f;
    auto healthy=render(musical,c,&trace);c.eraseRetention=Tape::kFaultRetention;auto fault=render(musical,c,&trace);
    std::vector<std::vector<float>> waves={musical.samples,healthy.samples,fault.samples};
    const double target=soundforge::rmsTarget(waves,std::pow(10.,-23./20),.89);
    std::ofstream receipt(output/"renders.csv");lab::require(bool(receipt),"Cannot create render receipt");
    receipt<<std::setprecision(12)<<"file,rate,frames,software_input_gain,raw_rms,raw_peak,raw_dc,matching_gain,final_rms,final_peak,final_dc,retention,feedback,host_processing_us\n";
    for(size_t i=0;i<music.size();++i) {
        const auto variant=i%3;auto signal=waves[variant];const auto raw=soundforge::stats(signal);
        const double gain=i<3?1:soundforge::rmsGain(raw,target);for(auto& x:signal)x=float(x*gain);
        const auto final=soundforge::stats(signal);lab::require(final.peak<.999 && (i<3||final.peak<=.890001),"Output has insufficient headroom");
        lab::write(output/music[i],{source.rate,signal});
        receipt<<music[i]<<','<<source.rate<<','<<signal.size()<<','<<calibration<<','<<raw.rms<<','<<raw.peak<<','<<raw.mean<<','<<gain<<','<<final.rms<<','<<final.peak<<','<<final.mean<<','
               <<(variant==2?Tape::kFaultRetention:0)<<','<<(variant?.65f:0)<<','<<(variant==1?healthy.processingUs:variant==2?fault.processingUs:0)<<'\n';
    }
    // Two DIFFERENT real-guitar excerpts from this same source. They are fed into
    // the write head normally, never stored/mixed as a ghost outside the model.
    lab::Wave ab;ab.rate=source.rate;ab.samples.resize(10*source.rate,0);auto onlyB=ab;
    const size_t turn=2*source.rate,delay=size_t(std::lround(double(source.rate)*double(.3f)));
    for(size_t i=0;i<phraseLength;++i) {
        ab.samples[i]=source.samples[i];
        for(unsigned pass=1;pass<=3;++pass)ab.samples[pass*turn+i]=onlyB.samples[pass*turn+i]=source.samples[bStart+i];
    }
    TapeConfig p;p.loopSeconds=2;p.feedback=0;p.mix=1;
    auto h=render(ab,p);auto hB=render(onlyB,p);p.eraseRetention=Tape::kFaultRetention;
    auto f=render(ab,p,&trace);auto fB=render(onlyB,p);auto isolated=f.samples;
    for(size_t i=0;i<isolated.size();++i)isolated[i]=i>=turn+delay?f.samples[i]-fB.samples[i]:0;
    const std::vector<std::vector<float>> experiments={ab.samples,h.samples,f.samples,fB.samples,isolated};
    for(size_t i=0;i<proof.size();++i)lab::write(output/proof[i],{source.rate,experiments[i]});
    std::ofstream ghost(output/"ghost.csv"),gs(output/"ghost-spectrum.csv");lab::require(bool(ghost)&&bool(gs),"Cannot create ghost receipts");
    ghost<<std::setprecision(12)<<"pass,feedback,healthy_A_residual_rms,fault_A_residual_rms,A_gain,expected_gain,projection_on_A,correlation_with_A,B_wet_rms\n";
    for(unsigned pass=1;pass<=4;++pass) {
        double hh=0,ff=0,aa=0,dot=0,bb=0;
        const size_t start=pass*turn+delay;
        for(size_t i=0;i<phraseLength;++i) {
            const double a=f.samples[delay+i],delta=f.samples[start+i]-fB.samples[start+i],hd=h.samples[start+i]-hB.samples[start+i];
            hh+=hd*hd;ff+=delta*delta;aa+=a*a;dot+=a*delta;bb+=double(fB.samples[start+i])*fB.samples[start+i];
        }
        lab::require(aa>1e-8,"Phrase A is silent; cannot prove a ghost");
        const double gain=std::sqrt(ff/aa),expected=std::pow(double((1-Tape::kWriteFraction)*Tape::kFaultRetention),pass);
        lab::require(std::sqrt(hh/phraseLength)<1e-7 && std::abs(gain-expected)<2e-6,"Feedback-zero ghost proof failed");
        ghost<<pass<<",0,"<<std::sqrt(hh/phraseLength)<<','<<std::sqrt(ff/phraseLength)<<','<<gain<<','<<expected<<','<<dot/aa<<','<<(ff>0?dot/std::sqrt(aa*ff):0)<<','<<std::sqrt(bb/phraseLength)<<'\n';
    }
    // Actual isolated fault energy by frequency, raw gain, Hann window. FFT is
    // the existing LabRAT host routine; bins are descriptive, not noise/alias proof.
    const size_t fftSize=8192;std::vector<std::complex<double>> bins(fftSize);double windowSum=0;
    for(size_t i=0;i<fftSize;++i) {const double w=.5-.5*std::cos(2*pi*i/(fftSize-1));windowSum+=w;bins[i]=isolated[turn+delay+i]*w;}
    fft(bins);gs<<std::setprecision(12)<<"frequency_hz,isolated_A_peak_amplitude,window,feedback,pass\n";
    for(size_t i=0;i<=fftSize/2;++i)gs<<i*double(source.rate)/fftSize<<','<<std::abs(bins[i])*(i==0||i==fftSize/2?1:2)/windowSum<<",Hann,0,1\n";
    spectrum(output/"spectrum.csv");
    std::ofstream settings(output/"settings.json"),notes(output/"LISTEN.txt");
    lab::require(bool(settings)&&bool(notes),"Cannot create settings/instructions");
    settings<<std::setprecision(12)<<"{\"host_experiment_not_canonical_patch\":true,\"rate_hz\":"<<source.rate<<",\"source_frames\":"<<originalFrames
            <<",\"input_peak_normalized\":0.25,\"software_input_gain\":"<<calibration<<",\"loop_seconds\":1.2,\"delay_seconds\":0.3,\"erase_to_write_seconds\":0.015,\"transport_m_per_s\":0.1905,\"write_fraction\":0.6,\"healthy_retention\":0,\"fault_retention\":0.85,\"feedback\":0.65,\"drive\":4,\"record_hz\":4500,\"replay_hz\":6000,\"mix\":0.5,\"loop_cells\":"<<Tape::requiredCells(float(source.rate),c)
            <<",\"write_to_play_cells\":"<<std::lround(double(source.rate)*double(c.delaySeconds))<<",\"erase_to_write_cells\":"<<std::lround(double(source.rate)*Tape::kEraseToWriteSeconds)
            <<",\"proof_loop_seconds\":2,\"proof_feedback\":0,\"proof_mix\":1,\"phrase_frames\":"<<phraseLength<<",\"phrase_A_source_frame\":0,\"phrase_B_source_frame\":"<<bStart<<",\"maximum_medium_bytes\":768000,\"processor_bytes\":"<<sizeof(Tape)<<"}\n";
    notes<<"SF-006 reduced tape medium; NOT measured physical tape, canonical patch or Daisy performance.\n"
         <<"Start 04_healthy_matched.wav vs 05_weak_erase_matched.wav (same full input, shared whole-file RMS target; gains in renders.csv). 01/02 are common-gain raw. 00/03 dry. No LUFS/perceptual approval.\n"
         <<"Defect: residual retention at ERASE head 0 -> .85. Same finite WRITE fraction .6, fixed 4.5/6 kHz record/replay loss and rational saturation, drive4, feedback .65, mix.5, loop1.2s, write/play .3s. 4.8s equal tail; no resampling/wow/flutter/noise.\n"
         <<"10 supplies A once, DIFFERENT B at 2/4/6s (excerpts from SAME source); 11/12 are wet with feedback=0. Replay starts .3s later. Listen around 2.3s in 12: B plus A's retained medium. 13 is the B-only counterfactual through the SAME faulty model. 14 is 12-13 after 2.3s, at raw gain, isolating A for analysis, NOT a ghost mixed into the processor.\n"
         <<"ghost.csv proves healthy near-zero A residual and .34^pass residual in the fault over four turns; ghost-spectrum.csv is Hann-windowed actual isolated A, raw gain. medium.csv traces actual erase/write/play cells/magnetization and pass; normalized units, not Teslas/Volts.\n"
         <<"Full file/input provenance in audio-provenance.json and source-original.wav in CI artifact. Optional CLI input has no inferred rights; default synthetic is NOT recorded guitar. Founder listening acceptance remains pending.\n"
         <<"Integer transport, E->W->P->E, explicit nearest-sample head geometry. Scalar erase/overwrite population approximation, no measured erase current or hysteresis/minor loops/depth/alignment/bias oscillator. spectral third/folded-third diagnostic is not total alias rejection.\n"
         <<"Timing receipt is a separate full-processor host pass including output stores; excludes preparation, allocation, stats and IO. Not ARM cycles/WCET. External medium ownership is stopped/control-only; no callback allocation.\n";
    for(auto* stream:{&trace,&receipt,&ghost,&gs,&settings,&notes}) {stream->flush();lab::require(bool(*stream),"Cannot finish tape evidence");}
    std::cout<<"Tape listening: 6 music WAVs + 5 feedback-zero proof WAVs, medium/ghost/spectrum receipts in "<<output<<'\n';
}
}
int main(int argc,char** argv) {try {
    std::filesystem::path input,output;
    for(int i=1;i<argc;++i) {const std::string key=argv[i];lab::require((key=="--input"||key=="--output")&&i+1<argc,"Use --output DIR [--input mono-pcm16.wav]");const std::string value=argv[++i];lab::require(!value.empty(),"Empty path");(key=="--input"?input:output)=value;}
    lab::require(!output.empty(),"Explicit output required");audition(input,output);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
