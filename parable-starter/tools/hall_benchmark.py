#!/usr/bin/env python3
"""Three sequential full-processor passes. Run after builds/tests, not concurrently."""
import argparse,csv,hashlib,json,statistics,subprocess,platform
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);trials=[];hashes={}
for trial in range(3):
 values={}
 for name in ['kCathedral','Verbity']:
  d=out/name;result=subprocess.check_output([a.build.resolve()/('hall_candidate_'+name),a.input.resolve(),d],text=True);(out/(name+f'-{trial}.txt')).write_text(result);values[name]=float(result.split('microseconds_per_frame=')[1].strip())
 d=out/'selected';result=subprocess.check_output([a.build.resolve()/'parable_hall_lab','--input',a.input.resolve(),'--output',d],text=True);(out/f'native-{trial}.txt').write_text(result)
 rows=list(csv.DictReader((d/'levels.csv').open()));values['native_healthy']=float(next(x for x in rows if x['name']=='healthy_raw')['us_per_frame']);values['native_fault']=float(next(x for x in rows if x['name']=='fault_raw')['us_per_frame']);trials.append(values)
 current={str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*.wav'))}
 if trial:assert current==hashes,'recordings changed across benchmark passes'
 hashes=current
report={'host':platform.platform(),'compiler':subprocess.check_output(['c++','--version'],text=True).splitlines()[0],'cpu':next((x.split(':',1)[1].strip() for x in Path('/proc/cpuinfo').read_text().splitlines() if x.startswith('model name')),'unknown'),'frames_per_measured_mode':768000,'block_frames':64,'trials_us_per_frame':trials,'median_us_per_frame':{k:statistics.median(x[k] for x in trials) for k in trials[0]},'protocol':'Actual complete process calls; setup/allocation/statistics/file I/O outside timed loop. Sequential three trials; run with no concurrent DSP builds/tests. Host clock/scheduler uncontrolled; not WCET or MCU estimate.','wav_sha256':hashes}
(out/'performance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='wav_sha256'},indent=2))
