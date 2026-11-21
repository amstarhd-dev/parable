#!/usr/bin/env python3
"""Compare source-bound physical routine costs only within compatible capture contexts."""
import argparse,json
from pathlib import Path
from hardware import check

def compare(manifest,captures):
 artifacts={a['artifact_id']:a for a in manifest['artifacts'] if a.get('status')=='linked'};rows=[]
 for name,raw in captures:
  result=check(raw,manifest);result['capture_file']=name;rows.append(result)
 groups={}
 for row in rows:
  c=row['capture'];a=artifacts[c['artifact_id']];config=a.get('configuration',{});profile=config.get('profile',{});family=profile.get('variant_of',c['profile'])
  # Exclude sticky arithmetic exception flags from the rounding/FZ/DN context key.
  protocol=config.get('harness_sha256',{})
  required=[config.get('compiler_sha256'),config.get('libdaisy_archive_sha256'),protocol.get('firmware_bench.cpp'),protocol.get('adapters.h'),a.get('layout')]
  if not all(required):row['comparison_withheld']='missing compiler/SDK/protocol/layout context';continue
  key=(family,c['execution'],c['stimulus'],c['mode'],c['route'],c['core_hz_rcc'],c['sample_rate'],c['block_frames'],c['blocks'],c['fpscr']&0x07c00000,c['fpdscr']&0x07c00000,c['icache_enabled'],c['dcache_enabled'],a.get('layout'),config.get('compiler_sha256'),config.get('libdaisy_archive_sha256'),protocol.get('firmware_bench.cpp'),protocol.get('adapters.h'))
  groups.setdefault(key,[]).append(row)
 comparisons=[]
 for context,group in groups.items():
  reference=next((r for r in group if r['capture']['profile']==context[0]),None)
  if not reference:continue
  for candidate in group:
   if candidate is reference:continue
   pair=dict(reference=reference['capture']['profile'],candidate=candidate['capture']['profile'],execution=context[1],mode=context[3],context_matched=True)
   captures_to_check=[reference['capture'],candidate['capture']]
   valid=all(c['counter_enabled']==1 and c['valid_blocks']==c['blocks'] and not any(c[k] for k in ['invalid_intervals','frame_mismatches','finite_output_failures','native_failures']) for c in captures_to_check)
   if valid:
    before=reference['capture']['max_cycles'];after=candidate['capture']['max_cycles'];pair.update(reference_max_cycles=before,candidate_max_cycles=after,observed_maximum_cost_change_percent=100*(after/before-1))
   else:pair['comparison']='withheld: invalid capture or numerical failure'
   pair['production_admitted']=False;comparisons.append(pair)
 return dict(schema='parable.hardware-cost-comparison.v1',measurements=rows,comparisons=comparisons,production_admitted=False,limits='Matched observed maxima only, not WCET, statistical speedup, isolated cost subtraction, audio equivalence or graph admission. No cross-execution/cache/FPU/toolchain-context percentage is computed.')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--captures',nargs='+',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=compare(json.loads(a.manifest.read_text()),[(str(f),f.read_text()) for f in a.captures]);a.output.write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
