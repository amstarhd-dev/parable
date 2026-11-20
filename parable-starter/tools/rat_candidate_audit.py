#!/usr/bin/env python3
"""Compile and compare reference, 4x97 and 4x97/floatclip RAT variants.

Eight deterministic two-second synthetic stimuli at 48k. Host-only differential
check, not a claim of perceptual similarity, hardware cycles, or target safety.
Python 3 standard library and C++17 compiler; no network or third-party modules.
"""
import argparse
import math
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FRAMES, NCASES = 96000, 8
CASES = ("pluck", "low_drive_chords", "3517Hz", "7971Hz", "hard_transients", "wideband", "diode_open", "both_diodes_open")

def render(out, compiler, name, defines):
    binary=out/("rat_"+name)
    subprocess.run([compiler,"-O3","-DNDEBUG","-std=c++17","-fno-fast-math",
                    "-ffp-contract=off","-I"+str(ROOT/"include"),*defines,
                    str(ROOT/"src/parable_rat.cpp"),
                    str(ROOT/"tools/rat_candidate_probe.cpp"),"-o",str(binary)],check=True)
    raw=out/(name+".raw")
    subprocess.run([str(binary),str(raw)],check=True)
    data=raw.read_bytes()
    if len(data)!=FRAMES*NCASES*4:raise RuntimeError("Invalid size for "+name)
    values=struct.unpack("<"+"f"*(FRAMES*NCASES),data)
    if not all(math.isfinite(x) for x in values):raise RuntimeError("Nonfinite "+name)
    return [values[i*FRAMES:(i+1)*FRAMES] for i in range(NCASES)]

def error_percent(a,b,lag=0):
    # 4x193 has 24 base samples more delay than 4x97. No gain normalization.
    start,end=5000,95000
    x=a[start:end];y=b[start+lag:end+lag]
    if len(x)!=len(y):raise RuntimeError("Delay alignment outside signal")
    numerator=sum((p-q)**2 for p,q in zip(x,y))
    denominator=sum(p*p for p in x)
    if denominator<1.e-15:raise RuntimeError("Silent reference")
    return 100*math.sqrt(numerator/denominator)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,default=ROOT/"build"/"rat-candidate-audit")
    parser.add_argument("--compiler",default="c++")
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    reference=render(out,args.compiler,"reference4x193",[])
    fir=render(out,args.compiler,"fir4x97",["-DPARABLE_RAT_BENCH_FIR97"])
    fp32=render(out,args.compiler,"fir4x97_floatclip",["-DPARABLE_RAT_BENCH_FIR97","-DPARABLE_RAT_BENCH_FLOATCLIP"])
    worst=0.
    print("case,97tap_vs_193tap_pct,fp32clip_vs_fp64clip_pct")
    for k,name in enumerate(CASES):
        # Compare earlier 97-tap output to later 193-tap output.
        e1=error_percent(reference[k],fir[k],lag=-24)
        e2=error_percent(fir[k],fp32[k])
        print(f"{name},{e1:.6f},{e2:.6f}")
        worst=max(worst,e2)
    print(f"worst_floatclip_pct={worst:.6f}")
    if worst>0.05:raise RuntimeError("FP32 clipping divergence >0.05% in eight probe cases")
    print("PASS: host differential only; no authorization for production use")
if __name__=="__main__":main()
