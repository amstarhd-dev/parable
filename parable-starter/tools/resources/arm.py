#!/usr/bin/env python3
"""Compile original kernels and inspect reachable callback operations on pinned Cortex-M7. No physical run."""
import argparse,gzip,hashlib,json,re,subprocess
from pathlib import Path
from run import HERE,ROOT,Sources,validate,command,digest,require
FLAGS=['-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard','-O2','-std=gnu++17','-fno-fast-math','-fno-finite-math-only','-ffunction-sections','-fdata-sections','-fstack-usage','-fno-exceptions','-fno-rtti','-fshort-enums','-fno-move-loop-invariants','-fno-unwind-tables','-fasm','-finline','-finline-functions-called-once']
def analyze(objects,compiler,root='resource_process_frame'):
 tool=lambda name:compiler.parent/('arm-none-eabi-'+name)
 functions={};frames={};sizes=[];model_bytes=None
 for obj in objects:
  size=command([tool('size'),obj]);parts=size.splitlines()[-1].split();sizes.append(dict(object=obj.name,text=int(parts[0]),data=int(parts[1]),bss=int(parts[2])))
  symbols=command([tool('nm'),'-SC',obj])
  for line in symbols.splitlines():
   f=line.split()
   if len(f)==4 and f[3]=='resource_model':model_bytes=int(f[1],16)
  asm=command([tool('objdump'),'-drC',obj]);heads=list(re.finditer(r'^[0-9a-f]+ <(.+)>:\n',asm,re.M))
  for i,h in enumerate(heads):
   body=asm[h.end():heads[i+1].start() if i+1<len(heads) else len(asm)]
   calls=re.findall(r'R_ARM_THM_(?:CALL|JUMP24)\s+(.+)',body)
   functions[h[1]]=dict(calls=calls,hardware_float64_instructions=body.count('.f64'),float32_instructions=len(re.findall(r'\.f32\b',body)))
  su=obj.with_suffix('.su')
  if su.exists():
   for line in su.read_text().splitlines():
    bits=line.rsplit('\t',2)
    if len(bits)==3:
     try:frames[bits[0]]=dict(bytes=int(bits[1]),kind=bits[2])
     except ValueError:pass
 reached=set();external=set();todo=[root]
 while todo:
  name=todo.pop()
  if name in reached:continue
  if name not in functions:external.add(name);continue
  reached.add(name);todo+=functions[name]['calls']
 return dict(native_objects=sizes,probe_object_bytes=model_bytes,reachable_callback_functions={n:functions[n] for n in sorted(reached)},callback_external_symbols=sorted(external),soft_double_helpers=sorted(n for n in external if n.startswith('__aeabi_d') or n.startswith('__aeabi_f2d') or n.startswith('__aeabi_d2f')),callback_libm=sorted(n for n in external if re.fullmatch(r'(exp|expm1|log|log1p|sqrt|pow|sin|cos|tanh|atan|tan|floor|ceil|fmod|copysign|fabs)f?',n)),compiler_stack_frames=frames,largest_static_function_frame_bytes=max([f['bytes'] for f in frames.values() if f['kind']=='static'],default=0),stack_limitations='Individual compiler frames only; external library/SDK/IRQ frames and call-chain high-water unmeasured')
def compile_profile(profile,models,sources,out,compiler):
 directory=out/'arm'/profile['id'];directory.mkdir(parents=True,exist_ok=True);cpp=[];includes=[];defs=['BENCH_ARM'];forced=[]
 for m in models:
  cache,c,i=sources.extract(m);cpp+=c;includes+=i;defs+=m['defines'];forced+=[cache/f for f in m.get('forced_includes',[])]
 selected=models[0]['adapter'] if len(models)==1 else 'Chain<'+','.join(m['adapter'] for m in models)+'>'
 probe=directory/'probe.cpp';probe.write_text('#include "adapters.h"\nnamespace resource_bench {using Selected='+selected+';}\nresource_bench::Selected resource_model;\nextern "C" float resource_process_frame(float x){return resource_model.process(x);}\nextern "C" bool resource_prepare(){return resource_model.prepare(48000,2);}\n')
 objects=[];commands=[]
 for i,src in enumerate([*dict.fromkeys(cpp),probe]):
  obj=directory/(str(i)+'-'+src.stem+'.o');cmd=[compiler,*FLAGS,*['-D'+d for d in dict.fromkeys(defs)],*[x for f in forced for x in ['-include',f]],*['-I'+str(p) for p in dict.fromkeys(includes)],'-I'+str(HERE),'-c',src,'-o',obj];commands.append(list(map(str,cmd)))
  r=subprocess.run(list(map(str,cmd)),capture_output=True,text=True);(directory/(obj.stem+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError('ARM compilation failed: '+str(directory/(obj.stem+'.log')))
  objects.append(obj)
 report=analyze(objects,compiler);report['commands']=commands
 return report

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--only',nargs='+');p.add_argument('--chains',action='store_true');p.add_argument('--fetch',action='store_true');a=p.parse_args();compiler=a.compiler.resolve()
 pins=json.loads((ROOT/'firmware/dependencies.json').read_text());require(command([compiler,'-dumpfullversion']).strip()==pins['toolchain']['version'],'Wrong compiler version');version=command([compiler,'--version']);require('xpack' in version.lower(),'Wrong compiler distribution')
 catalog=validate(json.loads((HERE/'catalog.json').read_text()));by_id={m['id']:m for m in catalog['models']};profiles=catalog['models']+(catalog['chains'] if a.chains else [])
 if a.only:
  if set(a.only)-{m['id'] for m in profiles}:p.error('Unknown profile')
  profiles=[m for m in profiles if m['id'] in a.only]
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);sources=Sources(out,a.fetch);rows=[]
 for profile in profiles:
  row=dict(id=profile['id'],profile=profile)
  try:
   models=[by_id[m] for m in profile['members']] if 'members' in profile else [profile]
   row.update(status='compiled',**compile_profile(profile,models,sources,out,compiler))
  except (RuntimeError,subprocess.CalledProcessError) as e:row.update(status='failed',error=str(e))
  (out/(profile['id']+'-arm.json')).write_text(json.dumps(row,indent=2)+'\n');rows.append(row);print(profile['id'],row['status'],flush=True)
 ledger=dict(schema='parable.arm-resource-results.v1',compiler=version,compiler_sha256=digest(compiler.read_bytes()),flags=FLAGS,source_sha256=sources.hashes,results=rows,hardware_run=False,callback_cycles=None)
 (out/'arm-ledger.json').write_text(json.dumps(ledger,indent=2)+'\n');return int(any(r['status']=='failed' for r in rows))
if __name__=='__main__':raise SystemExit(main())
