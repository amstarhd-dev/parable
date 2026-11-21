#pragma once
#include <stdint.h>
namespace ram_identity {
inline uint32_t checksum(volatile uint32_t* words) {
 uint32_t crc=0xffffffff;
 for(unsigned i=1;i<31;++i) for(unsigned byte=0;byte<4;++byte) {
  crc^=(words[i]>>(8*byte))&255;
  for(unsigned b=0;b<8;++b) crc=(crc>>1)^((0u-(crc&1))&0xedb88320);
 }
 return ~crc;
}
inline bool validRequest(volatile uint32_t* m,const uint32_t* identity) {
 if(m[0]!=0x50494451 || m[1]!=1 || m[2]!=32 || m[3]!=0) return false;
 uint32_t nonce=0;
 for(unsigned i=0;i<4;++i) nonce|=m[4+i];
 if(!nonce) return false;
 for(unsigned i=0;i<8;++i) if(m[8+i]!=identity[i]) return false;
 for(unsigned i=16;i<32;++i) if(m[i]) return false;
 return true;
}
} // namespace ram_identity
