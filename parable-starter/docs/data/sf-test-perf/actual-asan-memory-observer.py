from pathlib import Path
import time,json,os
out=Path('/workspace/work/sf-test-perf-evidence/final-ASAN');peak=0;at_peak=[];samples=0
start=time.monotonic()
while not (out/'summary.json').exists():
 parents={};names={};roots=[]
 for directory in Path('/proc').iterdir():
  if not directory.name.isdigit():continue
  try:
   text=(directory/'stat').read_text();end=text.rindex(')');fields=text[end+2:].split();pid=int(directory.name)
   parents[pid]=int(fields[1]);names[pid]=text[text.index('(')+1:end]
   command=(directory/'cmdline').read_bytes().split(b'\0')
   if names[pid]=='ctest' and b'/workspace/work/sf-test-perf-final-ASAN' in command:roots.append(pid)
  except (FileNotFoundError,PermissionError,ProcessLookupError,ValueError):pass
 selected=set(roots)
 while True:
  added={pid for pid,parent in parents.items() if parent in selected}-selected
  if not added:break
  selected|=added
 measured=[]
 for pid in selected:
  try:
   status=Path('/proc')/str(pid)/'status'
   rss=next(int(line.split()[1]) for line in status.read_text().splitlines() if line.startswith('VmRSS:'))
   measured.append({'pid':pid,'name':names[pid],'RSS_KiB':rss})
  except (FileNotFoundError,PermissionError,ProcessLookupError,StopIteration):pass
 if measured:
  samples+=1;total=sum(x['RSS_KiB'] for x in measured)
  if total>peak:peak=total;at_peak=measured
 time.sleep(.5)
(out/'sampled-memory.json').write_text(json.dumps({'sample_interval_s':.5,'samples':samples,'elapsed_including_build_wait_s':time.monotonic()-start,'peak_sum_RSS_KiB':peak,'peak_processes':at_peak,'scope':'complete ASAN CTest process tree, sampled; shared pages counted per process; not PSS, hard peak bound or target memory measurement; observer started before CTest'},indent=2)+'\n')
print('ASAN sampled process-tree peak RSS KiB',peak,flush=True)
