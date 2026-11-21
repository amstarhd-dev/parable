#!/usr/bin/env python3
"""Import versioned physical captures. A timing screen NEVER confers production admission."""
import argparse,csv,hashlib,json,re
from pathlib import Path
REQUIRED={'artifact_id','profile','execution','stimulus','mode','route','core_hz_rcc','sample_rate','block_frames','deadline_cycles','counter_enabled','blocks','valid_blocks','max_cycles','deadline_hits','invalid_intervals','frame_mismatches','source_set_sha256','reserved_sdram_bytes','used_sdram_bytes','finite_output_failures','fpscr','fpdscr','icache_enabled','dcache_enabled','native_failures','native_evaluations','native_solves','native_max_iterations','native_containment'}
NUMERIC=REQUIRED-{'artifact_id','profile','execution','stimulus','source_set_sha256'}
def parse(raw):
 blocks=re.findall(r'^PARABLE_RESOURCE_V1\s.*?^END_RESOURCE_CAPTURE\s*$',raw,re.M|re.S)
 if not blocks or len(blocks)!=len(re.findall(r'^PARABLE_RESOURCE_V1\b',raw,re.M)):raise ValueError('No complete unambiguous versioned capture')
 parsed=[]
 for block in blocks:
  fields={}
  for key,value in re.findall(r'\b([a-z_0-9]+)=([^\s]+)',block):
   if key in fields:raise ValueError('Duplicate capture field '+key)
   fields[key]=value
  if set(fields)!=REQUIRED:raise ValueError('Missing/unknown fields: '+str(set(fields)^REQUIRED))
  for name in NUMERIC:
   if not re.fullmatch(r'[0-9]+',fields[name]):raise ValueError('Invalid unsigned capture number '+name)
   fields[name]=int(fields[name])
   if fields[name]>0xffffffff:raise ValueError('Capture uint32 overflow')
  parsed.append(fields)
 if any(p!=parsed[0] for p in parsed):raise ValueError('Mixed captures/artifacts; supply one capture per file')
 return parsed[0]
def check(raw,manifest):
 capture=parse(raw);matches=[a for a in manifest['artifacts'] if a.get('artifact_id')==capture['artifact_id'] and a.get('status')=='linked']
 if len(matches)!=1:raise ValueError('Capture does not identify exactly one linked artifact')
 artifact=matches[0]
 for name in ['profile','execution','stimulus','mode','route','source_set_sha256','reserved_sdram_bytes']:
  if capture[name]!=artifact[name]:raise ValueError('Artifact/capture mismatch: '+name)
 if (capture['core_hz_rcc'],capture['sample_rate'],capture['block_frames'],capture['deadline_cycles'])!=(480000000,48000,48,480000):raise ValueError('Clock/rate/block/deadline mismatch')
 if capture['blocks']!=artifact['capture_blocks'] or capture['valid_blocks']+capture['invalid_intervals']!=capture['blocks'] or capture['frame_mismatches']>capture['invalid_intervals'] or capture['deadline_hits']>capture['valid_blocks']:raise ValueError('Incomplete/inconsistent sample count')
 if not 0<capture['max_cycles']<2**31:raise ValueError('Zero/ambiguous interval')
 if capture['used_sdram_bytes']>capture['reserved_sdram_bytes']:raise ValueError('SDRAM accounting mismatch')
 failures=[]
 if capture['counter_enabled']!=1 or capture['valid_blocks']!=capture['blocks'] or capture['invalid_intervals'] or capture['frame_mismatches']:failures.append('invalid counter/frame intervals')
 if capture['deadline_hits'] or capture['max_cycles']>=480000:failures.append('observed deadline-sized routine')
 if capture['finite_output_failures']:failures.append('nonfinite output')
 if capture['native_failures']:failures.append('native solver/numerical failures reported')
 if capture['max_cycles']>336000:failures.append('exceeds provisional70% whole-graph body allocation')
 return dict(schema='parable.hardware-measurement.v1',capture=capture,artifact_bin_sha256=artifact['bin_sha256'],raw_capture_sha256=hashlib.sha256(raw.encode()).hexdigest(),screen='FAIL' if failures else ('FOREGROUND_COST_ONLY' if capture['execution']=='foreground_kernel' else 'PASS_CALLBACK_BODY_SCREEN_ONLY'),observed_fraction_of_nominal_block=capture['max_cycles']/480000,failures=failures,production_admitted=False,limitations=['Physical evidence supplied by operator; importer cannot attest which board/binary was actually run','Foreground measurements are not audio callback or interrupt-context measurements','No ISR-inclusive WCET, stack high-water, cache worst case, analog or listening approval inferred'])
def matrix(manifest,path):
 with path.open('w') as f:
  w=csv.writer(f,lineterminator="\n");w.writerow(['artifact_id','profile','mode','route','execution','stimulus','capture_file','operator','board_serial','audio_input_connected','notes'])
  for a in manifest['artifacts']:
   if a.get('status')=='linked':w.writerow([a['artifact_id'],a['profile'],a['mode'],a['route'],a['execution'],a['stimulus'],'','','','',''])
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--capture',type=Path);p.add_argument('--matrix',type=Path);p.add_argument('--output',type=Path);a=p.parse_args();manifest=json.loads(a.manifest.read_text())
 if a.matrix:matrix(manifest,a.matrix)
 if a.capture:
  result=check(a.capture.read_text(),manifest);text=json.dumps(result,indent=2)+'\n'
  if a.output:a.output.write_text(text)
  else:print(text,end='')
 if not a.matrix and not a.capture:p.error('Use --matrix or --capture')
if __name__=='__main__':main()
