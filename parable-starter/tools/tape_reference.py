#!/usr/bin/env python3
"""Independent 50-digit material-write recurrence, no native DSP invocation.

Reference uses chronological write history and scheduled playback, NOT a shared
three-head ring implementation. Coefficients/partial overwrite follow ADR-006.
This checks the stated reduced model; it is not physical tape calibration.
"""
import argparse
import csv
from decimal import Decimal as D, localcontext
import hashlib
import json
from pathlib import Path


def generate(output):
    output.mkdir(parents=True, exist_ok=True)
    path = output/'reference.csv'
    with localcontext() as ctx, path.open('w', newline='') as stream:
        ctx.prec = 50
        writer = csv.writer(stream, lineterminator='\n')
        writer.writerow(['rate','retention','feedback','sample','input','erase_before','erase_after',
                         'write_before','record_value','written','play_magnetization','replay'])
        for rate in (44100,48000,96000):
            n, delay, erase = round(rate*.1), round(rate*.02), round(rate*.015)
            cr = 1-(-D('6.283185307179586476925286766559')*4500/rate).exp()
            cp = 1-(-D('6.283185307179586476925286766559')*6000/rate).exp()
            for retention, feedback in [('0','0'),('.85','0'),('.85','.65')]:
                r,f=D(retention),D(feedback)
                history=[]; rec=play=D(0)
                # Piecewise rational different A/B drives, not a sine generated
                # by the implementation being validated. Exact input decimals.
                for t in range(4*n):
                    phase=t % n
                    x = (D('.2') if phase<n//10 else D('-.1') if phase<n//5 else D(0)) if t<n else (D('-.15') if phase<n//8 else D('.05') if phase<n//4 else D(0)) if t<2*n else D(0)
                    mplay=history[t-delay] if t>=delay else D(0)
                    play+=cp*(mplay-play); rec+=cr*(x-rec)
                    field=4*rec+f*play
                    s= -D(1) if field < -3 else D(1) if field>3 else field*(27+field*field)/(27+9*field*field)
                    old=history[t-n] if t>=n else D(0)
                    before=r*old; written=D('.4')*before+D('.6')*s
                    erased=history[t+erase-n] if t+erase>=n else D(0)
                    history.append(written)
                    edges={0,1,delay-1,delay,delay+1,n-erase-1,n-erase,n-1,n,n+1,n+delay-1,n+delay,2*n,3*n,4*n-1}
                    if t%127==0 or t in edges:
                        writer.writerow([rate,retention,feedback,t,x,*[format(v,'.18e') for v in (erased,r*erased,before,s,written,mplay,play)]])
    manifest={'reference':'Python standard-library Decimal, 50 significant digits',
              'algorithm':'chronological write history, analytical erase retention, scheduled replay; no native ring/code/output',
              'model':'ADR-006, normalized remanence; mathematical reference, not a measured tape',
              'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
              'generator_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'rates':[44100,48000,96000],'loop_seconds':.1,'delay_seconds':.02,'erase_to_write_seconds':.015}
    (output/'reference-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    generate(p.parse_args().output)
