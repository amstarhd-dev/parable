#!/usr/bin/env python3
"""Research arithmetic/evidence checks; never declares mixed-signal hardware valid."""
import argparse,hashlib,json,math,sys,bisect
sys.dont_write_bytecode=True
from pathlib import Path
from budget import calculate
from linear_port import calculate as linear
PROJECT=Path(__file__).resolve().parents[2]
FIXTURES=PROJECT/'tests/fixtures/sf004'
def close(a,b):
    if isinstance(a,dict):assert a.keys()==b.keys();[close(a[k],b[k]) for k in a]
    elif isinstance(a,list):assert len(a)==len(b);[close(x,y) for x,y in zip(a,b)]
    elif isinstance(a,float):assert math.isclose(a,b,rel_tol=1e-10,abs_tol=1e-16),(a,b)
    else:assert a==b,(a,b)
def main():
    p=argparse.ArgumentParser();p.add_argument('--continuous',type=Path);p.add_argument('--rc',type=Path);a=p.parse_args()
    close(calculate(),json.loads((FIXTURES/'budget.json').read_text()))
    close(linear(),json.loads((FIXTURES/'linear-port.json').read_text()))
    data=json.loads((a.continuous or FIXTURES/'continuous-results.json').read_text())
    assert len(data['cases'])==7
    assert all(c['mode']['name']=='continuous' and c['passes'] for c in data['cases'])
    blocked=json.loads((FIXTURES/'blocked-coupled-results.json').read_text())
    assert len(blocked['cases'])==2 and all(not c['passes'] and 'NUMERICAL BLOCKER' in c['error'] for c in blocked['cases'])
    if a.rc:
        rows=[[float(v) for v in line.split()] for line in a.rc.read_text().splitlines()[1:]]
        times=[r[0] for r in rows]
        for t,expected in [(1e-3,0.),(2e-3,1-math.exp(-1)),(3e-3,1-math.exp(-2))]:
            i=bisect.bisect_left(times,t);i=min(i,len(rows)-1)
            assert abs(times[i]-t)<1e-12 and abs(rows[i][2]-expected)<1e-5,(t,rows[i])
    for path,digest in json.loads((FIXTURES/'protected-inputs.json').read_text()).items():
        assert hashlib.sha256((PROJECT/path).read_bytes()).hexdigest()==digest,path
    print('SF004 arithmetic,7 continuous-reference cases and explicit sampled-model blocker verified; NOT HIL validation')
if __name__=='__main__':main()
