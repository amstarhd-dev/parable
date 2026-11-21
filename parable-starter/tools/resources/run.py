#!/usr/bin/env python3
"""Compile original pinned DSP, measure identical48-frame host callbacks; never estimates M7 utilization."""
import argparse,hashlib,json,os,platform,posixpath,re,resource,shutil,subprocess,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1];REPO=ROOT.parent
WRAPS=['malloc','calloc','realloc','free','pthread_mutex_lock','pthread_mutex_trylock','read','write','fopen','fwrite']
SIGNALS=['guitar','silence','pluck','high_note','noise','hard_edges','burst_tail','dc','invalid','extreme','denormal']
def digest(data):return hashlib.sha256(data).hexdigest()
def command(args,**kw):return subprocess.run(list(map(str,args)),check=True,capture_output=True,text=True,**kw).stdout
def git(*args):return command(['git','-C',REPO,*args])
def blob(commit,path):return subprocess.check_output(['git','-C',str(REPO),'show',commit+':'+path])
def require(condition,message):
 if not condition:raise ValueError(message)
def validate(catalog):
 require(catalog['schema']=='parable.resource-catalog.v1','Unknown catalog schema')
 ids=[]
 for m in catalog['models']:
  require(bool(re.fullmatch('[a-z0-9_]+',m['id'])) and bool(re.fullmatch('[0-9a-f]{40}',m['commit'])),'Invalid profile ID/source pin');ids.append(m['id'])
  require(bool(re.fullmatch('[A-Za-z][A-Za-z0-9]*',m['adapter'])),'Invalid adapter identifier')
  for f in m['source_files']+m.get('extra_sources',[]):require(not f.startswith('/') and '..' not in Path(f).parts,'Escaping source path')
  for d in m['defines']:require(bool(re.fullmatch('[A-Z_0-9]+(?:=[0-9]+)?',d)),'Invalid build definition')
 require(len(ids)==len(set(ids)),'Duplicate model ID')
 model_ids=set(ids)
 for c in catalog['chains']:
  require(bool(re.fullmatch('[a-z0-9_]+',c['id'])) and c['id'] not in ids,'Duplicate/invalid graph ID');ids.append(c['id'])
  require(all(m in model_ids for m in c['members']) and c['route'] in [0,1,2],'Unknown graph member/route')
 return catalog

class Sources:
 def __init__(self,out,fetch=False):self.out=out;self.fetch=fetch;self.checked=set();self.hashes={};self.dependencies={};self.current_hashes={}
 def extract(self,model):
  commit=model['commit'];directory=self.out/'sources'/commit
  if commit not in self.checked:
   try:git('cat-file','-e',commit+'^{commit}')
   except subprocess.CalledProcessError:
    if not self.fetch:raise RuntimeError('Missing Git source '+commit+'; use --fetch')
    git('fetch','--no-tags','origin',commit)
   self.checked.add(commit)
  includes=['include','src',*model.get('include_dirs',[])]
  def visit(path):
   path=posixpath.normpath(path)
   if path.startswith('../') or path.startswith('/'):raise ValueError('Escaping source path')
   key=commit+':'+path
   if key in self.hashes:return
   data=blob(commit,'parable-starter/'+path);self.hashes[key]=digest(data);self.dependencies[key]=[]
   target=directory/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
   for include in re.findall(rb'^\s*#\s*include\s*"([^"\n]+)"',data,re.M):
    name=include.decode();candidates=[posixpath.normpath(posixpath.join(posixpath.dirname(path),name)),*[posixpath.normpath(i+'/'+name) for i in includes]]
    found=False
    for candidate in dict.fromkeys(candidates):
     if candidate.startswith('../') or candidate.startswith('/'):continue
     try:git('cat-file','-e',commit+':parable-starter/'+candidate)
     except subprocess.CalledProcessError:continue
     visit(candidate);self.dependencies[key].append(commit+':'+candidate);found=True;break
    if not found and name not in {'math.h','stdlib.h','stdint.h','string.h','stdio.h','assert.h','stddef.h','float.h','limits.h'}:raise RuntimeError(f'Unresolved local include {path}: {name}')
  roots=model['source_files']+model.get('extra_sources',[])+model.get('forced_includes',[])
  for f in roots:visit(f)
  used=set();todo=[commit+':'+f for f in roots]
  while todo:
   key=todo.pop()
   if key in used:continue
   used.add(key);todo+=self.dependencies[key]
  self.current_hashes={key:self.hashes[key] for key in sorted(used)}
  return directory,[(directory/f) for f in model['source_files']+model.get('extra_sources',[]) if f.endswith('.cpp')],[(directory/i) for i in includes]
def build(profile,models,sources,out,compiler):
 directory=out/'build'/profile['id'];directory.mkdir(parents=True,exist_ok=True)
 cpp=[];inc=[];defs=[];forced=[];source_hashes={}
 for m in models:
  cache,c,i=sources.extract(m);cpp+=c;inc+=i;defs+=m['defines'];forced+=[cache/f for f in m.get('forced_includes',[])];source_hashes.update(sources.current_hashes)
 selected=models[0]['adapter'] if len(models)==1 else 'Chain<'+','.join(m['adapter'] for m in models)+'>'
 defs+=['BENCH_SELECTED='+selected]
 if len(models)>1:defs+=['BENCH_ROUTE']
 flags=['-O3','-DNDEBUG','-std=c++17','-fno-fast-math','-fno-finite-math-only','-ffp-contract=off','-fstack-usage','-Wall','-Wextra','-Wpedantic']
 cmd=[compiler,*flags,*[x for f in forced for x in ['-include',str(f)]],*['-D'+d for d in dict.fromkeys(defs)],*['-I'+str(i) for i in dict.fromkeys(inc)],'-I'+str(HERE),'-I'+str(ROOT/'tools'),'-DBENCH_GUITAR_PATH='+json.dumps(str(ROOT/'tests/fixtures/resources/guitar-1s.wav')),*dict.fromkeys(cpp),HERE/'host_bench.cpp',HERE/'host_tracker.cpp','-pthread','-Wl,'+','.join('--wrap='+w for w in WRAPS),'-o',directory/'bench']
 (directory/'command.json').write_text(json.dumps(list(map(str,cmd)),indent=2)+'\n')
 r=subprocess.run(list(map(str,cmd)),cwd=directory,capture_output=True,text=True);(directory/'build.log').write_text(r.stdout+r.stderr)
 if r.returncode:raise RuntimeError('Compilation failed: '+str(directory/'build.log'))
 frames={}
 for f in directory.glob('*.su'):
  for line in f.read_text().splitlines():
   fields=line.rsplit('\t',2)
   if len(fields)==3:
    try:frames[fields[0]]=dict(bytes=int(fields[1]),kind=fields[2])
    except ValueError:pass
 return directory/'bench',dict(source_sha256=source_hashes,command=list(map(str,cmd)),compiler_stack_frames=frames,largest_static_function_frame_bytes=max([v['bytes'] for v in frames.values() if v['kind']=='static'],default=0),binary_sha256=digest((directory/'bench').read_bytes()),harness_sha256={f.name:digest(f.read_bytes()) for f in HERE.glob('*') if f.is_file()},guitar_sha256=digest((ROOT/'tests/fixtures/resources/guitar-1s.wav').read_bytes()))
def constrained(cpu):
 resource.setrlimit(resource.RLIMIT_AS,(1024**3,1024**3));os.sched_setaffinity(0,{cpu})
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--fetch',action='store_true');p.add_argument('--only',nargs='+');p.add_argument('--frames',type=int,default=4800);p.add_argument('--repetitions',type=int,default=2);p.add_argument('--signals',nargs='+',choices=SIGNALS,default=SIGNALS);p.add_argument('--modes',nargs='+',type=int,choices=[0,1,2,3,4],default=None);p.add_argument('--chains',action='store_true');p.add_argument('--compile-only',action='store_true');p.add_argument('--reuse-builds',action='store_true');p.add_argument('--current-checkout',action='store_true');p.add_argument('--cpu',type=int);a=p.parse_args()
 if a.frames%48 or not 48<=a.frames<=480000 or not 2<=a.repetitions<=10:p.error('frames must be48..480000/divisible by48; repetitions2..10')
 catalog=validate(json.loads((HERE/'catalog.json').read_text()))
 if a.current_checkout:
  head=git('rev-parse','HEAD').strip()
  for model in catalog['models']:
   if model['commit']==catalog['baseline']:model['commit']=head
 allowed=os.sched_getaffinity(0);cpu=min(allowed) if a.cpu is None else a.cpu
 if cpu not in allowed:p.error('CPU is not in this process affinity mask')
 by_id={m['id']:m for m in catalog['models']};profiles=catalog['models']+(catalog['chains'] if a.chains else [])
 if a.only:
  known={m['id'] for m in profiles}
  if set(a.only)-known:p.error('Unknown profile(s): '+str(set(a.only)-known))
  profiles=[m for m in profiles if m['id'] in a.only]
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);sources=Sources(out,a.fetch)
 environment=dict(platform=platform.platform(),cpu=next((s.split(':',1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines() if s.startswith('model name')),'unknown'),compiler=command([a.compiler,'--version']).splitlines()[0],python=platform.python_version(),pinned_cpu=cpu,timer='steady_clock; no background compilation during reuse-builds measurement; OS scheduling/timer overhead included; no M7 conversion',allocator='requested bytes of explicit wrapped native allocations and new/delete; excludes allocator overhead/global/static memory; initialization includes tracking overhead')
 results=[]
 for profile in profiles:
  identity=profile['id'];row=dict(id=identity,source_profile=profile,measured_at_unix=time.time(),status='pending',cases=[])
  try:
   models=[by_id[m] for m in profile['members']] if 'members' in profile else [profile]
   if a.reuse_builds:
    binary=out/'build'/identity/'bench';meta=json.loads((binary.parent/'build-metadata.json').read_text())
    require(digest(binary.read_bytes())==meta['binary_sha256'],'Binary changed')
    for name in ['host_bench.cpp','adapters.h','host_tracker.cpp','host_tracker.h']:require(meta['harness_sha256'][name]==digest((HERE/name).read_bytes()),'Stale computational harness; recompile first')
    expected={}
    for model in models:sources.extract(model);expected.update(sources.current_hashes)
    require(expected==meta['source_sha256'],'Pinned source closure changed')
   else:
    binary,meta=build(profile,models,sources,out,a.compiler)
    (binary.parent/'build-metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
   row['build']=meta
   row['source_sha256']=meta['source_sha256']
   for mode in ([] if a.compile_only else (a.modes or profile.get('modes',[0,1,2]))):
    for signal in a.signals:
     try:
      r=subprocess.run([str(binary),signal,str(mode),str(max(a.frames,48000) if signal=='guitar' else max(a.frames,144000) if signal=='burst_tail' and models[-1]['category'] in ['reverb','delay','modulation'] else a.frames),str(a.repetitions),str(profile.get('route',0))],capture_output=True,text=True,timeout=45,preexec_fn=lambda:constrained(cpu))
      if r.returncode:raise RuntimeError(f'exit{r.returncode}: '+r.stderr[-1000:])
      data=json.loads(r.stdout);data.update(stimulus=signal,mode=mode);row['cases'].append(data)
     except (RuntimeError,ValueError,subprocess.TimeoutExpired) as e:row['cases'].append(dict(stimulus=signal,mode=mode,error=str(e)))
   row['status']='compiled_only' if a.compile_only else 'measured' if all('error' not in c and c['prepared'] for c in row['cases']) else 'partial'
  except (RuntimeError,subprocess.CalledProcessError) as e:row.update(status='build_failed',error=str(e))
  row['integrity_pass']=all('error' not in c and c.get('prepared') and c.get('reprepare_ok') and c.get('deterministic') and not c.get('nonfinite_outputs') and not c.get('forbidden_callback_operations') and not c.get('allocation_tracker_overflows') for c in row['cases']) if row['cases'] else None
  row['reported_solver_failure_max']=max((c.get('failures',0) for c in row['cases']),default=None)
  row['production_admitted']=False
  (out/(identity+'.json')).write_text(json.dumps(row,indent=2)+'\n');results.append(row)
  print(identity,row['status'],len(row['cases']),'cases',flush=True)
 ledger=dict(schema='parable.resource-results.v1',catalog_sha256=digest((HERE/'catalog.json').read_bytes()),harness_sha256={f.name:digest(f.read_bytes()) for f in HERE.glob('*') if f.is_file()},environment=environment,configuration=dict(sample_rate=48000,block_frames=48,frames=a.frames,repetitions=a.repetitions,signals=a.signals,modes=a.modes),results=results,hardware_admission=False,limitations=['Short host screen; not a tail-lifetime stability campaign or WCET','Compiler frames are individual functions, not call-chain/high-water','Nominal, fault and high-drive corners are not exhaustive parameter coverage','Return units differ by original processor; common normalized stimuli are not a calibrated perceptual comparison'])
 (out/'ledger.json').write_text(json.dumps(ledger,indent=2)+'\n');print('Ledger:',out/'ledger.json')
 return int(any(r['status']=='build_failed' or r['integrity_pass'] is False for r in results))
if __name__=='__main__':raise SystemExit(main())
