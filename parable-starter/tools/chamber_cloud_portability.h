#pragma once
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
namespace std { using ::sinf; }
inline int strcpy_s(char* dst, std::size_t n, const char* src) { if(std::strlen(src)>=n)return 1;std::memcpy(dst,src,std::strlen(src)+1);return 0; }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
