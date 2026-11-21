#!/usr/bin/env python3
"""Inventory native source versions and duplicate blobs across the recorded PR heads."""
import argparse,collections,gzip,json,re,subprocess
from pathlib import Path
from run import HERE,ROOT,REPO,git

def inventory(fetch=False):
 catalog=json.loads((HERE/'catalog.json').read_text());snapshots=json.loads((ROOT/'docs/resource-pr-snapshot.json').read_text());snapshots=[dict(number=None,sha=catalog['baseline'],head='main',state='main',merged=True),*snapshots]
 known={p for m in catalog['models'] for p in m['source_files']+m.get('extra_sources',[])};versions={};contents={};unknown=[];missing=[]
 for s in snapshots:
  try:listing=git('ls-tree','-r','--format=%(objectname) %(path)',s['sha'],'--','parable-starter/include','parable-starter/src')
  except subprocess.CalledProcessError:
   if not fetch:missing.append(s);continue
   try:git('fetch','--no-tags','origin',s['sha']);listing=git('ls-tree','-r','--format=%(objectname) %(path)',s['sha'],'--','parable-starter/include','parable-starter/src')
   except subprocess.CalledProcessError:missing.append(s);continue
  for line in listing.splitlines():
   blob,path=line.split(' ',1);relative=path.removeprefix('parable-starter/');key=(path,blob)
   if key not in versions:versions[key]=dict(path=path,git_blob_sha=blob,snapshots=[])
   versions[key]['snapshots'].append(dict(pr=s['number'],commit=s['sha'],branch=s['head'],state=s['state'],merged=s['merged']))
   if blob not in contents:contents[blob]=subprocess.check_output(['git','-C',str(REPO),'cat-file','blob',blob]).decode(errors='replace')
   code=contents[blob]
   # Framework processing entry points are deliberately separate from effects.
   if relative.endswith('.cpp') and re.search(r'::process\s*\(',code) and ('namespace parable::dsp' in code or 'namespace parable::soundforge' in code) and relative not in known:
    item=dict(path=path,commit=s['sha'],pr=s['number'])
    if item not in unknown:unknown.append(item)
 result=dict(schema='parable.source-inventory.v1',snapshot_count=len(snapshots),unique_native_file_versions=len(versions),files=sorted(versions.values(),key=lambda x:(x['path'],x['git_blob_sha'])),unknown_processor_sources=unknown,missing_snapshots=missing,notes=['Same Git blob means identical file bytes, not independent implementations','Historical versions are cataloged; active references use exact catalog commits','PR78/79 reuse hall and existing effects for offline routing/cabinet experiments; not new drive/reverb voices','Signalsmith/Cloud candidate comparison runner is research-only behind PARABLE_CHAMBER_CANDIDATES; Signalsmith is externally fetched/unselected, not a vendored native processor','Browser/Python historical prototypes are not portable native production DSP','Incomplete unpublished SF026 workspace is preserved separately and excluded from GitHub source admission'])
 return result

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--fetch',action='store_true');a=p.parse_args();result=inventory(a.fetch);a.output.write_bytes(gzip.compress((json.dumps(result,indent=2)+'\n').encode(),mtime=0));print(result['snapshot_count'],'snapshots;',result['unique_native_file_versions'],'native file versions;',len(result['unknown_processor_sources']),'unknown processors;',len(result['missing_snapshots']),'missing snapshots');return int(bool(result['unknown_processor_sources'] or result['missing_snapshots']))
if __name__=='__main__':raise SystemExit(main())
