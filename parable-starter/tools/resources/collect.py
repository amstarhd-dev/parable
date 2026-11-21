#!/usr/bin/env python3
"""Join measured host/ARM evidence, preserve unknowns, emit a readable resource matrix."""
import argparse,gzip,json,statistics
from pathlib import Path
from run import HERE

def main():
 p=argparse.ArgumentParser();p.add_argument('--host',type=Path,required=True);p.add_argument('--arm',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();host=json.loads(a.host.read_text());arm=json.loads(a.arm.read_text());catalog=json.loads((HERE/'catalog.json').read_text());target=67108864
 arm_rows={r['id']:r for r in arm['results']};rows=[]
 for r in host['results']:
  cases=[c for c in r['cases'] if c.get('prepared')];profile=r['source_profile'];ar=arm_rows.get(r['id'],{});native=ar.get('native_objects',[])[:-1]
  row=dict(id=r['id'],category=profile.get('category','chain'),pr=profile.get('pr'),commit=profile.get('commit'),variant_of=profile.get('variant_of'),members=profile.get('members'),route=profile.get('routing'),status=r['status'],cases=len(cases),production_admitted=False)
  if cases:
   row.update(host_max_case_median_us_frame=max(c['callback_p50_ns']/48/1000 for c in cases),host_max_case_mean_us_frame=max(c['mean_ns_frame']/1000 for c in cases),host_max_callback_wall_us=max(c['callback_max_ns']/1000 for c in cases),host_live_requested_bytes=max(c['live_requested_heap_including_object_bytes'] for c in cases),host_prepare_peak_requested_bytes=max(c['reprepare_peak_requested_heap_bytes'] for c in cases),host_max_initialization_ms=max(c['initialization_mean_ns_includes_tracking']/1e6 for c in cases),host_max_reprepare_ms=max(c['reprepare_mean_ns_includes_tracking']/1e6 for c in cases),observed_solver_failures_max=max(c['failures'] for c in cases),nonfinite_outputs=sum(c['nonfinite_outputs'] for c in cases),deterministic=all(c['deterministic'] for c in cases),forbidden_callback_operations=sum(c['forbidden_callback_operations'] for c in cases))
  row.update(arm_status=ar.get('status','not-measured'),arm_instance_including_adapter_bytes=ar.get('probe_object_bytes'),arm_native_object_text_bytes=sum(n['text'] for n in native) if native else None,arm_native_static_data_bss_bytes=sum(n['data']+n['bss'] for n in native) if native else None,arm_largest_individual_compiler_frame_bytes=ar.get('largest_static_function_frame_bytes'),arm_callback_external_symbols=ar.get('callback_external_symbols',[]),arm_callback_libm=ar.get('callback_libm',[]),arm_callback_hardware_float64_instruction_sites=sum(f['hardware_float64_instructions'] for f in ar.get('reachable_callback_functions',{}).values()))
  holds=[]
  if row.get('host_prepare_peak_requested_bytes',0)>target:holds.append('active-plus-prepare memory exceeds64MiB in host representation; target placement/lifetime requires redesign')
  if profile.get('hold'):holds.append(profile['hold'])
  if row.get('observed_solver_failures_max'):holds.append('native solver reported failures in recorded stress cases; retain separate numerical hold')
  if row.get('nonfinite_outputs') or row.get('forbidden_callback_operations') or not row.get('deterministic',False):holds.append('benchmark integrity/finite-output failure')
  row['holds']=holds;row['admission']='HOLD' if holds else 'NEEDS_SOURCE_BOUND_TARGET_CHAIN_AND_AUDIO_EVIDENCE';rows.append(row)
 summary=dict(schema='parable.resource-matrix.v1',target=catalog['target'],environment=host['environment'],configuration=host['configuration'],rows=rows,scope='Current library plus named active experiments, not all future correspondence voices',limitations=['Host wall-time is never converted to M7 utilization','Native failure counter0 means no reported failures; some processors expose no such counter','Largest compiler frame is NOT total callback stack or measured high-water','Host requested allocation bytes exclude allocator overhead; ARM instances and native static sections reported separately','No source-bound physical captures imported; all admissions remain pending','Short input/control screens are not exhaustive validation or musical approval'])
 a.output.mkdir(parents=True,exist_ok=True);(a.output/'resource-matrix.json').write_text(json.dumps(summary,indent=2)+'\n')
 lines=['# Complete resource matrix','',f"{len(rows)} measured profiles; {sum(r['cases'] for r in rows)} input/control scenarios; 48kHz/48-frame routines. **No effect admitted.**",'', 'Host timing column is the largest case median in microseconds/frame; wall-clock measurements on the recorded host CPU, never M7 utilization. Memory is requested live/reprepare-peak host bytes. ARM text excludes the probe; ARM instance includes the laboratory adapter. Frame is the largest individual compiler frame, including stopped preparation, not total stack.','', '| Profile | Source | Host µs/frame | Host live / reprepare peak B | ARM instance / text B | Max function frame B | Hold |','|---|---|---:|---:|---:|---:|---|']
 for r in rows:
  origin='PR'+str(r['pr']) if r['pr'] else ('pinned mixed chain' if r.get('members') else 'main')
  lines.append(f"| {r['id']} | {origin} | {r.get('host_max_case_median_us_frame',0):.3f} | {r.get('host_live_requested_bytes','—')} / {r.get('host_prepare_peak_requested_bytes','—')} | {r.get('arm_instance_including_adapter_bytes','—')} / {r.get('arm_native_object_text_bytes','—')} | {r.get('arm_largest_individual_compiler_frame_bytes','—')} | {'; '.join(r['holds']) if r['holds'] else 'target/audio pending'} |")
 lines+=['','All raw scenarios, startup/reprepare costs, output hashes, counter values, callback quantiles, compiler frames, math dependencies and source/command fingerprints are in the compressed host/ARM ledgers beside this table. Native source is unchanged.','', 'Observed solver failure counts remain visible even where the output is finite. A successful infrastructure test does not certify those models.']
 (a.output/'MATRIX.md').write_text('\n'.join(lines)+'\n')
 (a.output/'host-ledger.json.gz').write_bytes(gzip.compress(a.host.read_bytes(),mtime=0));(a.output/'arm-ledger.json.gz').write_bytes(gzip.compress(a.arm.read_bytes(),mtime=0))
 print(len(rows),'profiles;',sum(r['cases'] for r in rows),'scenarios')
if __name__=='__main__':main()
