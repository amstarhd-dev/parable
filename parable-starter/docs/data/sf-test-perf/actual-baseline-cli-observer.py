import sys,runpy,subprocess,os,json,hashlib,csv,io
from pathlib import Path
real_run=subprocess.run
index=0
def audited_run(args,*a,**kw):
    global index
    result=real_run(args,*a,**kw)
    if result.returncode==0 and '--output' in args:
        target=Path(args[args.index('--output')+1]);files={}
        if target.is_dir():
            for p in sorted(target.rglob('*')):
                if not p.is_file():continue
                name=str(p.relative_to(target));data=p.read_bytes()
                if name=='renders.csv':
                    rows=list(csv.DictReader(data.decode().splitlines()));fields=[f for f in rows[0] if f!='host_processor_us'];s=io.StringIO();w=csv.DictWriter(s,fieldnames=fields,lineterminator='\n');w.writeheader();w.writerows({k:v for k,v in r.items() if k in fields} for r in rows);data=s.getvalue().encode()
                files[name]=hashlib.sha256(data).hexdigest()
            dest=Path(os.environ['SF_PERF_AUDIT']);dest.mkdir(exist_ok=True);(dest/f'cli-output-{index}.json').write_text(json.dumps(files,indent=2)+'\n');index+=1
    return result
subprocess.run=audited_run
sys.argv=sys.argv[1:]
runpy.run_path(sys.argv[0],run_name='__main__')
