import os,sys,json,time,resource,subprocess,hashlib,platform
from pathlib import Path
TOOLS=Path('/workspace/work/baseline-tools/bin')
REPO=Path('/workspace/work/parable-sf-test-perf')
ROOT=Path('/workspace/work/sf-test-perf-evidence');ROOT.mkdir(exist_ok=True)
if sys.argv[1]=='child':
    out=Path(sys.argv[2]); name=sys.argv[3]; args=sys.argv[4:]; start=time.perf_counter()
    if name in {"ts808_listening_cli","klon_listening_cli"}:
        os.environ['SF_PERF_AUDIT']=str(out/(name+'-artifacts'))
        args=[args[0],'/workspace/work/sf-test-perf-audit-python.py',*args[1:]]
    if out.name.endswith("Debug") and name in {"ts808_circuit_and_safety","ts808_listening_cli","klon_circuit_and_safety","klon_listening_cli","muff_circuit_and_safety","wavefolder_circuit_and_safety"}:
        args=["gprofng","collect","app","-a","off","-p","10","-o",str(out/(name+".er")),*args]
    child=subprocess.Popen(args)
    _,status,r= os.wait4(child.pid,0);code=os.waitstatus_to_exitcode(status);child.returncode=code
    data=dict(name=name,command=args,wall_s=time.perf_counter()-start,user_s=r.ru_utime,system_s=r.ru_stime,max_rss_KiB=r.ru_maxrss,exit_code=code)
    (out/(name+'.json')).write_text(json.dumps(data,indent=2)+'\n');sys.exit(code if code>=0 else 128-code)
config=sys.argv[1]; phase=sys.argv[2]; jobs=sys.argv[3]
build=Path('/workspace/work/sf-test-perf-'+phase+'-'+config)
out=ROOT/(phase+'-'+config);out.mkdir(exist_ok=True)
env=dict(os.environ,PATH=str(TOOLS)+':'+os.environ['PATH'])
def run(args,name):
    start=time.perf_counter()
    with (out/(name+'.log')).open('w') as f:
        f.write('$ '+repr(args)+'\n');f.flush()
        r=subprocess.run(args,env=env,cwd=REPO,stdout=f,stderr=subprocess.STDOUT)
    print(config,phase,name,'exit',r.returncode,'wall',round(time.perf_counter()-start,3),flush=True)
    return r.returncode
args=[str(TOOLS/'cmake'),'-S',str(REPO/'parable-starter'),'-B',str(build),'-DCMAKE_BUILD_TYPE='+config,'-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src']
if config=='ASAN':
    args=[a.replace('-DCMAKE_BUILD_TYPE=ASAN','-DCMAKE_BUILD_TYPE=Debug') for a in args]
    args.extend(['-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -O1','-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie'])
    env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
assert run(args,'configure')==0
assert run([str(TOOLS/'cmake'),'--build',str(build),'-j2'],'build')==0
inventory=json.loads(subprocess.check_output([str(TOOLS/'ctest'),'--test-dir',str(build),'--show-only=json-v1'],env=env))
(out/'inventory.json').write_text(json.dumps(inventory,indent=2)+'\n')
f=build/'CTestTestfile.cmake';original=f.read_text();f.write_text(original)
for test in inventory['tests']:
    name=test['name'];prefix='add_test('+name+' '
    if prefix not in original: raise RuntimeError('unrecognized CTest file: '+name)
    timing=' '.join('"'+str(x).replace('\\','\\\\').replace('"','\\"')+'"' for x in [sys.executable,__file__,'child',out,name])+' '
    original=original.replace(prefix,prefix+timing,1)
f.write_text(original)
start=time.perf_counter(); before=resource.getrusage(resource.RUSAGE_CHILDREN)
code=run([str(TOOLS/'ctest'),'--test-dir',str(build),'-j'+jobs,'--output-on-failure','--no-tests=error'],'ctest')
after=resource.getrusage(resource.RUSAGE_CHILDREN)
data=dict(configuration=config,phase=phase,jobs=int(jobs),wall_s=time.perf_counter()-start,cpu_user_s=after.ru_utime-before.ru_utime,cpu_system_s=after.ru_stime-before.ru_stime,exit_code=code,source=subprocess.check_output(['git','rev-parse','HEAD'],cwd=REPO,text=True).strip(),platform=platform.platform(),affinity=sorted(os.sched_getaffinity(0)),cpu_quota=Path('/sys/fs/cgroup/cpu.max').read_text().strip())
(out/'summary.json').write_text(json.dumps(data,indent=2)+'\n')
print(json.dumps(data),flush=True)
