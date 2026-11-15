from pathlib import Path
import subprocess,os,time,json,sys
repo=Path('/workspace/work/parable-sf-test-perf');ev=Path('/workspace/work/sf-test-perf-evidence');tools=Path('/workspace/work/baseline-tools/bin')
while not (ev/'final-ASAN-build/generated-archives.json').exists():time.sleep(5)
env=dict(os.environ,PATH=str(tools)+':'+os.environ['PATH']);out=ev/'final-audits';out.mkdir(exist_ok=True)
records=[]
def run(name,command,extra=None):
 start=time.monotonic()
 with (out/(name+'.log')).open('w') as f:
  f.write('$ '+repr(list(map(str,command)))+'\n');f.flush()
  result=subprocess.run(list(map(str,command)),cwd=repo,env=dict(env,**(extra or {})),stdout=f,stderr=subprocess.STDOUT)
 records.append(dict(name=name,command=list(map(str,command)),exit_code=result.returncode,wall_s=time.monotonic()-start));print(name,result.returncode,round(records[-1]['wall_s'],3),flush=True)
 (out/'summary.json').write_text(json.dumps(records,indent=2)+'\n')
 return result.returncode
arm=Path('/workspace/work/sf-test-perf-arm');compiler=Path('/workspace/work/par005-fresh-cache/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++')
run('full-arm',[sys.executable,repo/'parable-starter/firmware/build.py','--cache','/workspace/work/par005-fresh-cache','--output',arm])
for name in ['activation','tape','rotary','muff','ts808','klon','wavefolder']:
 cmd=[sys.executable,repo/('parable-starter/tools/check_'+name+'_arm.py'),'--compiler',compiler,'--output',arm/(name+'-audit')]
 if name=='activation':cmd+=['--living']
 run(name+'-arm',cmd)
build=Path('/workspace/work/sf-test-perf-TSAN')
code=run('tsan-configure',[tools/'cmake','-S',repo/'parable-starter','-B',build,'-DCMAKE_BUILD_TYPE=Debug','-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/workspace/work/sf015-deps/nlohmann_json-src','-DCMAKE_CXX_FLAGS=-fsanitize=thread -fno-omit-frame-pointer -fno-pie -O1','-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=thread -no-pie'])
if not code:
 code=run('tsan-build',[tools/'cmake','--build',build,'--target','parable_activation_test','parable_living_test','-j2'])
 if not code:run('tsan-test',[tools/'ctest','--test-dir',build,'-j2','--output-on-failure','--no-tests=error','-R','^(activation|living)_(contract|concurrency)$'],{'TSAN_OPTIONS':'halt_on_error=1'})
run('negative-controls',[sys.executable,repo/'parable-starter/tools/soundforge/negative_controls.py','--project',repo/'parable-starter','--build','/workspace/work/sf-test-perf-final-Release','--output',ev/'negative-controls-final'])
(out/'complete.json').write_text(json.dumps({'completed':True,'source':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),'results':records},indent=2)+'\n')
print('All final audits attempted',flush=True)
