#!/usr/bin/env python3
"""Object-only admission evidence; no firmware placement/link/timing promise."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();cc=a.compiler.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);commands=[]
def run(args):
 args=list(map(str,args));commands.append(args);return subprocess.check_output(args,text=True)
version=run([cc,'--version']);assert 'xPack GNU Arm Embedded GCC' in version;assert run([cc,'-dumpfullversion']).strip()=='14.2.1'
flags=['-std=c++17','-O2','-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-fno-fast-math','-fno-finite-math-only','-fstack-usage','-DFFTCONVOLVER_DONT_USE_SSE','-I'+str(root/'tools'),'-I'+str(root/'include'),'-I'+str(root/'third_party/fftconvolver')]
objects={}
for n in ['FFTConvolver','AudioFFT','Utilities']:
 obj=out/(n+'.o');run([cc,*flags,'-c',root/'third_party/fftconvolver'/(n+'.cpp'),'-o',obj]);size=run([cc.parent/'arm-none-eabi-size',obj]);undefined=run([cc.parent/'arm-none-eabi-nm','-uC',obj]);asm=run([cc.parent/'arm-none-eabi-objdump','-drC',obj]);(out/(n+'.disassembly.txt')).write_text(asm)
 objects[n]=dict(zip(['text','data','bss'],map(int,size.splitlines()[-1].split()[:3])),undefined=undefined,static_frames=(out/(n+'.su')).read_text().splitlines(),float64_instructions=asm.count('.f64'))
report=dict(compiler=version,compiler_sha256=hashlib.sha256(cc.read_bytes()).hexdigest(),objects=objects,commands=commands,firmware_linked=False,cycles_measured=False,hardware_tested=False,limitations='Convolver upstream objects only. Includes control-side allocation/exception/math symbols. Ooura uses double FFT math. No full graph link, flash/SDRAM placement, callback stack high-water or Seed3 admission.')
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(objects,indent=2))
