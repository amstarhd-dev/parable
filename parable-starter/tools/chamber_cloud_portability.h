#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#if defined(PARABLE_CLOUD_UNMODIFIED_RESEARCH) && !defined(_MSC_VER)
namespace std { using ::sinf; } // Research-only shim for exact original source
#endif
#ifndef _MSC_VER
inline int strcpy_s(char* dst, std::size_t n, const char* src) { if(std::strlen(src)>=n)return 1;std::memcpy(dst,src,std::strlen(src)+1);return 0; }

#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
