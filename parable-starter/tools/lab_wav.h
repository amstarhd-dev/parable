#pragma once
// Host laboratory I/O only. Never linked into the DSP kernel or firmware.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace lab {
struct Wave { unsigned rate = 48000; std::vector<float> samples; };
inline void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
inline uint32_t le(const std::vector<uint8_t>& b, std::size_t p, unsigned bytes) {
    require(p <= b.size() && bytes <= b.size() - p, "Truncated WAV integer");
    uint32_t v = 0; for (unsigned j = 0; j < bytes; ++j) v |= uint32_t(b[p+j]) << (8*j); return v;
}
inline bool tag(const std::vector<uint8_t>& b, std::size_t p, const char* s) {
    return p <= b.size() && 4 <= b.size() - p && std::equal(b.begin()+p, b.begin()+p+4, s);
}
inline Wave decode(const std::vector<uint8_t>& b) {
    require(b.size() <= 6*1024*1024 && b.size() >= 44, "WAV must be 44 bytes to 6 MiB");
    require(tag(b,0,"RIFF") && tag(b,8,"WAVE"), "Only little-endian RIFF WAVE is supported");
    require(le(b,4,4) == b.size()-8, "RIFF size must match file exactly");
    Wave w; bool fmt = false, data = false; std::size_t start = 0, length = 0, p = 12;
    unsigned chunks = 0;
    while (p < b.size()) {
        require(++chunks <= 64 && b.size()-p >= 8, "Too many or truncated WAV chunks");
        const auto n = le(b,p+4,4); const auto body = p+8;
        require(n <= b.size()-body, "Truncated WAV chunk");
        if (tag(b,p,"fmt ")) {
            require(!fmt && n == 16, "Require one 16-byte PCM fmt chunk"); fmt = true;
            require(le(b,body,2)==1 && le(b,body+2,2)==1 && le(b,body+14,2)==16,
                    "Input must be mono integer PCM16; stereo/float/extensible are unsupported");
            w.rate = le(b,body+4,4);
            require(w.rate==32000 || w.rate==44100 || w.rate==48000 || w.rate==88200 || w.rate==96000,
                    "Supported WAV rates: 32000, 44100, 48000, 88200, 96000 Hz");
            require(le(b,body+8,4)==w.rate*2 && le(b,body+12,2)==2, "Invalid PCM byte rate/block alignment");
        } else if (tag(b,p,"data")) {
            require(!data && n>0 && n%2==0, "Require one nonempty even PCM data chunk");
            data = true; start = body; length = n;
        }
        p = body + n + (n & 1u); require(p <= b.size(), "Missing WAV chunk padding");
    }
    require(fmt && data && length/2 <= w.rate*30, "Require fmt/data and at most 30 seconds");
    w.samples.reserve(length/2);
    for (std::size_t i = start; i < start+length; i += 2) {
        const auto bits = le(b,i,2);
        const auto signedValue = bits >= 32768 ? static_cast<int>(bits)-65536 : static_cast<int>(bits);
        w.samples.push_back(static_cast<float>(signedValue)/32768.f);
    }
    return w;
}
inline Wave read(const std::filesystem::path& p) {
    std::ifstream f(p,std::ios::binary | std::ios::ate); require(bool(f), "Cannot open input WAV");
    const auto size = f.tellg(); require(size>=0 && size<=6*1024*1024, "Input exceeds 6 MiB");
    std::vector<uint8_t> b(static_cast<std::size_t>(size)); f.seekg(0);
    f.read(reinterpret_cast<char*>(b.data()), size); require(bool(f), "Cannot read WAV"); return decode(b);
}
inline std::vector<uint8_t> encode(const Wave& w) {
    require(!w.samples.empty() && w.samples.size() <= w.rate*30, "Output duration out of bounds");
    std::vector<uint8_t> b;
    const auto text = [&](const char* s){for(unsigned i=0;i<4;++i)b.push_back(static_cast<uint8_t>(s[i]));};
    const auto number = [&](uint32_t n,unsigned bytes){for(unsigned i=0;i<bytes;++i)b.push_back(static_cast<uint8_t>(n>>(8*i)));};
    text("RIFF"); number(36+static_cast<uint32_t>(w.samples.size()*2),4); text("WAVE");
    text("fmt "); number(16,4); number(1,2); number(1,2); number(w.rate,4); number(w.rate*2,4); number(2,2); number(16,2);
    text("data"); number(static_cast<uint32_t>(w.samples.size()*2),4);
    for (float x : w.samples) {
        require(std::isfinite(x) && std::fabs(x) < 1, "Refuse nonfinite/clipped WAV output");
        const int value = static_cast<int>(std::lround(x*32768.));
        require(value >= -32768 && value <= 32767, "PCM16 quantization overflow");
        number(static_cast<uint16_t>(value),2);
    }
    return b;
}
inline void write(const std::filesystem::path& path, const Wave& w) {
    const auto bytes = encode(w); std::ofstream f(path,std::ios::binary);
    require(bool(f), "Cannot create output WAV");
    f.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    f.flush();
    require(bool(f), "Cannot write output WAV");
}
} // namespace lab
