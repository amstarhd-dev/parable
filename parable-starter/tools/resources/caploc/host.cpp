#include "capricorn_loc.h"
#include <cstdio>
#include <cstdlib>
int main(int argc,char**argv){
    if(argc!=3)return 2;
    CapricornLoc drive(std::strtof(argv[1],nullptr),std::atoi(argv[2])!=0);
    float x;
    while(std::fread(&x,sizeof(x),1,stdin)==1){
        float y=drive.process(x);
        if(std::fwrite(&y,sizeof(y),1,stdout)!=1)return 3;
    }
    return 0;
}
