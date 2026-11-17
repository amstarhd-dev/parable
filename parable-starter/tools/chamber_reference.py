#!/usr/bin/env python3
"""Independent double ideal class-B/soft-rail law, not a measured amplifier."""
import argparse,csv,hashlib,json,math
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
points=[-.5,-.1,-.02,-.00701,-.007,-.00699,-.001,0,.001,.00699,.007,.00701,.02,.1,.5]
f=a.output/'return-amplifier.csv'
with f.open('w',newline='')as stream:
 w=csv.writer(stream,lineterminator="\n");w.writerow(['microphone','gain','return'])
 for gain in [0,.02,12,24,64]:
  for x in points:
   conductor=(max(x-.007,0) if x>=0 else min(x+.007,0))
   # Exponential definition supplies an independent bounded amplifier evaluation.
   z=max(-50.,min(50.,gain*conductor/.15));q=math.exp(2*z);value=.15*(q-1)/(q+1)
   w.writerow([x,gain,value])
(a.output/'reference.json').write_text(json.dumps(dict(model='Ideal symmetric biased class-B conduction with common soft voltage rail; software-referred7mV threshold, not measured hardware',rows=75,source_independent=True,sha256=hashlib.sha256(f.read_bytes()).hexdigest(),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),indent=2)+'\n')
