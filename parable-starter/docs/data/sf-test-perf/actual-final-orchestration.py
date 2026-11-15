from pathlib import Path
import os,sys,json,subprocess,time,gzip,hashlib
repo=Path('/workspace/work/parable-sf-test-perf');ev=Path('/workspace/work/sf-test-perf-evidence');tools=Path('/workspace/work/baseline-tools/bin')
env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH']); source=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
for conf in ['Debug','Release','ASAN']:
 build=Path('/workspace/work/sf-test-perf-final-'+conf);logs=ev/('final-'+conf+'-build');logs.mkdir(exist_ok=True)
 args=[str(tools/'cmake'),'-S',str(repo/'parable-starter'),'-B',str(build),'-DCMAKE_BUILD_TYPE='+('Debug' if conf=='ASAN' else conf),'-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src']
 if conf=='ASAN':
  args+=['-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1','-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie']
  env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
 commands=[('configure',args),('build',[str(tools/'cmake'),'--build',str(build),'-j2']),('ctest',[sys.executable,str(repo/'parable-starter/tools/soundforge/benchmark.py'),'--build',str(build),'--output',str(ev/('final-'+conf)),'--jobs','2','--ctest',str(tools/'ctest')]+(['--profile'] if conf=='Debug' else []))]
 statuses=[]
 for name,cmd in commands:
  start=time.monotonic()
  with (logs/(name+'.log')).open('w') as f:
   f.write('$ '+repr(cmd)+'\n');f.flush();r=subprocess.run(cmd,env=env,cwd=repo,stdout=f,stderr=subprocess.STDOUT)
  statuses.append(dict(stage=name,command=cmd,exit_code=r.returncode,wall_s=time.monotonic()-start));print(conf,name,statuses[-1]['exit_code'],round(statuses[-1]['wall_s'],3),flush=True)
  if r.returncode and name!='ctest':break
 (logs/'workflow.json').write_text(json.dumps({'source':source,'stages':statuses},indent=2)+'\n')
 # Reversible storage only after the suite finishes; no timings include this.
 if conf in ['Debug','ASAN']:
  records=[]
  for p in build.rglob('*'):
   if not p.is_file():continue
   eligible=p.suffix in {'.o','.a'}
   if conf=='ASAN' and p.suffix=='':
    with p.open('rb') as f:eligible|=f.read(4)==b'\x7fELF'
   if not eligible:continue
   data=p.read_bytes();digest=hashlib.sha256(data).hexdigest();dest=p.with_name(p.name+'.gz')
   with gzip.open(dest,'wb',compresslevel=5) as f:f.write(data)
   with gzip.open(dest,'rb') as f:assert hashlib.sha256(f.read()).hexdigest()==digest
   records.append(dict(original=str(p),archive=str(dest),sha256=digest,bytes=len(data),gzip_bytes=dest.stat().st_size));p.unlink()
  (logs/'generated-archives.json').write_text(json.dumps(records,indent=2)+'\n')
print('Final configuration workflow complete',flush=True)
