#pragma once
// Host-only synthetic adapters/strict file replay. No personal or acquired sensor data.
#include "parable_living.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace living_fixture {
namespace l=parable::living;
inline void require(bool b,const char* text) {if(!b) throw std::runtime_error(text);}
inline std::array<l::Profile,15> profiles() {
    std::array<l::Profile,15> p{};
    const l::Kind kinds[]={l::Kind::HeartRate,l::Kind::HeartRate,l::Kind::Interval,l::Kind::HeartRate,
        l::Kind::Eda,l::Kind::Motion,l::Kind::Motion,l::Kind::Ecg,l::Kind::Ppg,l::Kind::HrvSummary,
        l::Kind::Respiration,l::Kind::Temperature,l::Kind::Derived,l::Kind::Control,l::Kind::Control};
    const l::Unit units[]={l::Unit::Bpm,l::Unit::Bpm,l::Unit::Millisecond,l::Unit::Bpm,
        l::Unit::Microsiemens,l::Unit::MilliG,l::Unit::MilliG,l::Unit::Microvolt,l::Unit::Arbitrary,
        l::Unit::Millisecond,l::Unit::BreathsPerMinute,l::Unit::Celsius,l::Unit::Normalized,
        l::Unit::Normalized,l::Unit::Normalized};
    const float hz[]={1.f/300,1,0,1,4,50,200,130,25,.2f,.25f,1,10,1,1};
    for(size_t i=0;i<p.size();++i) {
        p[i].source=static_cast<uint32_t>(i+1); p[i].channel=1;
        p[i].kind=kinds[i]; p[i].unit=units[i]; p[i].acquisitionHz=hz[i]; p[i].deliveryHz=i==3?.2f:hz[i];
        p[i].ttl=i==0?310000000:(i==3?6000000:(i>=13?600000:3000000));
        p[i].timestampResolution=1; p[i].expectedLatency=i==3?5000000:0;
        p[i].permission=l::Permission::Granted; p[i].consentUntil=700000000;
    }
    p[13].destination=l::Destination::PreGain; p[14].destination=l::Destination::PostGain;
    p[13].acquisitionHz=p[13].deliveryHz=2;p[14].acquisitionHz=p[14].deliveryHz=2;
    return p;
}
inline l::Record record(const l::Profile& p,l::Time t,uint64_t sequence,float x) {
    l::Record r; r.source=p.source; r.channel=p.channel; r.kind=p.kind; r.unit=p.unit;
    r.origin=p.origin; r.delivery=p.delivery; r.timebase=p.clock.timebase; r.epoch=p.clock.epoch;
    r.measured=r.arrived=t; r.sequence=sequence; r.value[0]=x;
    r.validity=l::Validity::Valid; r.quality=l::Quality::Good; r.permission=l::Permission::Granted;
    r.flags=l::MeasurementKnown|l::ConfidenceKnown; r.confidence=1; return r;
}
// Transparent artificial signals. NO physiological inference/model fidelity is claimed.
inline std::vector<l::Record> synthetic(l::Time duration) {
    require(duration>0 && duration<=30000000,"Synthetic fixture duration must be <=30 seconds");
    const auto p=profiles(); std::vector<l::Record> out;
    for(size_t i=0;i<13;++i) {
        if(i==2) {
            const l::Time interval[]={2000000,1000000,333334,500000,1250000};
            l::Time t=0; uint64_t n=0;
            while(t<duration) {const auto dt=interval[n%5];t+=dt;if(t<duration)out.push_back(record(p[i],t,++n,float(dt)/1000));}
            continue;
        }
        const double rate=p[i].acquisitionHz;
        for(uint64_t n=0;;++n) {
            const auto t=static_cast<l::Time>(std::llround(double(n)*1000000/rate)); if(t>=duration) break;
            const float v=static_cast<float>(std::sin(double(n)*.1)); float x=0;
            switch(p[i].kind) {
            case l::Kind::HeartRate: x=70+5*v; break;
            case l::Kind::Eda: x=2+.1f*v; break;
            case l::Kind::Motion: x=100*v; break;
            case l::Kind::Ecg: x=1000*v; break;
            case l::Kind::Ppg: x=100+v; break;
            case l::Kind::HrvSummary: x=40+v; break;
            case l::Kind::Respiration: x=12+v; break;
            case l::Kind::Temperature: x=32+.01f*v; break;
            default: x=.5f+.1f*v; break;
            }
            auto r=record(p[i],t,n+1,x);
            if(i==3) r.arrived=(t/5000000+1)*5000000; // five-second batches, original sample times retained.
            if(i==5 || i==6) {r.value[1]=x*.5f;r.value[2]=-x;}
            out.push_back(r);
        }
    }
    std::stable_sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.arrived<b.arrived;});
    return out;
}
inline uint64_t integer(const std::string& s) {
    uint64_t value=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),value);
    require(r.ec==std::errc{} && r.ptr==s.data()+s.size(),"Invalid replay integer");return value;
}
inline std::vector<l::Record> read(std::istream& file) {
    std::vector<l::Record> out; std::string line; size_t bytes=0;
    const auto p=profiles();
    // Limit the read itself; checking std::getline(string) afterward permits an
    // arbitrarily large line allocation before rejection.
    const auto next=[&]() {
        char buffer[258]{};file.getline(buffer,sizeof(buffer));
        require(!file.bad() && (!file.fail() || file.eof()),"Replay line exceeds bound or IO error");
        if(file.gcount()==0) return false;
        line=buffer;require(line.size()<=256,"Replay line exceeds bound");return true;
    };
    require(next() && line=="source,measured_us,arrival_us,sequence,value,validity,quality,permission", "Invalid replay header");
    while(next()) {
        bytes+=line.size(); require(bytes<=1024*1024 && out.size()<20000 && line.size()<=256,"Replay exceeds bound");
        std::array<std::string,8> fields{}; size_t start=0;
        for(size_t i=0;i<8;++i) {
            const auto end=line.find(',',start); require((i==7)==(end==std::string::npos),"Replay field count");
            fields[i]=line.substr(start,end==std::string::npos?end:end-start); start=end+1;
        }
        const auto source=integer(fields[0]); require(source>0 && source<=p.size(),"Unknown fixture source");
        const auto valid=integer(fields[5]),quality=integer(fields[6]),permission=integer(fields[7]);
        require(valid<=2 && quality<=2 && permission<=3,"Invalid fixture enum");
        size_t consumed=0; const float x=std::stof(fields[4],&consumed);
        require(consumed==fields[4].size() && std::isfinite(x),"Invalid replay number");
        auto r=record(p[source-1],integer(fields[1]),integer(fields[3]),x); r.arrived=integer(fields[2]);
        r.validity=static_cast<l::Validity>(valid); r.quality=static_cast<l::Quality>(quality); r.permission=static_cast<l::Permission>(permission);
        require(out.empty() || r.arrived>=out.back().arrived,"Unordered replay arrival"); out.push_back(r);
    }
    require(file.eof(),"Replay IO error"); return out;
}
inline std::vector<l::Record> read(const std::string& name) {
    std::ifstream f(name);require(bool(f),"Cannot open replay");return read(f);
}
// Chunk size only changes ingestion batches; watermark requires all due observations.
inline size_t feed(l::Scheduler& scheduler,const std::vector<l::Record>& rows,size_t cursor,
                   l::Time now,size_t chunk) {
    require(chunk>0 && chunk<=l::kRecords,"Invalid replay chunk");
    while(cursor<rows.size() && rows[cursor].arrived<=now) {
        for(size_t n=0;n<chunk && cursor<rows.size() && rows[cursor].arrived<=now;++n,++cursor)
            require(scheduler.ingest(rows[cursor])==l::Code::Ok,"Replay ingestion rejected");
    }
    return cursor;
}
} // namespace living_fixture
