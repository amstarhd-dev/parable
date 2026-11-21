// Benchmark-only bounded foreground command parser. No DSP or USB dependencies.
#pragma once
#include <cstddef>
#include <cstring>

namespace resource_control {
constexpr const char* handoffVersion = "reset-systeminit-v1";
enum class Event { none, hello, enter, rejected };
class Protocol {
    char line_[128]{};
    size_t used_ = 0;
    bool overflow_ = false, ready_ = false, authorized_ = false;
    char nonce_[33]{};
    const char* artifact_;
    static bool hex(const char* p, size_t n) {
        for(size_t i = 0; i < n; ++i)
            if(!((p[i] >= '0' && p[i] <= '9') || (p[i] >= 'a' && p[i] <= 'f')))
                return false;
        return true;
    }
  public:
    explicit Protocol(const char* artifact) : artifact_(artifact) {}
    void reportCompleted() { ready_ = true; }
    const char* nonce() const { return nonce_; }
    Event feed(char c) {
        if(c != '\n') {
            if(used_ < sizeof(line_) - 1) line_[used_++] = c;
            else overflow_ = true;
            return Event::none;
        }
        line_[used_] = 0;
        const size_t n = used_;
        used_ = 0;
        if(overflow_) { overflow_ = false; return Event::rejected; }
        if(!ready_) return Event::rejected;
        if(n == 38 && !std::memcmp(line_, "HELLO ", 6) && hex(line_ + 6, 32)) {
            std::memcpy(nonce_, line_ + 6, 32); nonce_[32] = 0;
            authorized_ = true;
            return Event::hello;
        }
        if(authorized_ && n == 107 && !std::memcmp(line_, "ENTER_DFU ", 10)
           && !std::memcmp(line_ + 10, artifact_, 64) && line_[74] == ' '
           && !std::memcmp(line_ + 75, nonce_, 32)) {
            authorized_ = false; // one use, foreground must wait for ACK completion
            return Event::enter;
        }
        return Event::rejected;
    }
};
} // namespace resource_control
