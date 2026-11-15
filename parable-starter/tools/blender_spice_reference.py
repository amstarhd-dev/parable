#!/usr/bin/env python3
"""Independent built-in ngspice44.2 full Blender references; no native imports.

NumPy2.3.5 offline only. One bounded process at a time; scratch ~400MB, RAM ~1GB.
Fresh regeneration/check mode never refreshes old models or acceptance thresholds.
"""
import argparse,csv,hashlib,json,math,os,shutil,struct,subprocess
from pathlib import Path
import numpy as np
VT=1.380649e-23/1.602176634e-19*300
NODES=['input','b1','c1','e1','e2','fb','st','dry','sw','b3','c3','e3','dc','de','b4','c4','e4','clip','tin','low','dark','bright','tone','level','lw','b5','c5','e5','wet','bw','jack']
PINS=[f'q{q}_{pin}_A' for q in range(1,6) for pin in ['ib','ic','ie']]+[f'd{d}_A' for d in range(1,5)]
BASE=dict(fault=0,sustain=.5,tone=.5,blend=.5,level=.5,boost=False,supply=9,temp=26.85,beta=350,rs=1000,load=1e6,caps=1)
FIELDS=list(BASE)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def saved(c):return '.save all '+' '.join(f'@q{q}[{pin}]' for q in range(1,6) for pin in ['ib','ic','ie'])+' '+' '.join(f'@d{d}[id]' for d in range(1,5) if c['fault']!=d)+'\n'
def vectors(c):return ' '.join('v('+n+')' for n in NODES)+' '+' '.join(f'@q{q}[{pin}]' for q in range(1,6) for pin in ['ib','ic','ie'])+' '+' '.join('zero' if c['fault']==d else f'@d{d}[id]' for d in range(1,5))
def write(path,header,rows):
 with path.open('w',newline='') as f:
  w=csv.writer(f,lineterminator='\n');w.writerow(header)
  for r in rows:w.writerow(int(x) if isinstance(x,(bool,np.bool_)) else format(x,'.17g') if isinstance(x,(float,np.floating)) else x for x in r)
def net(fault=0,sustain=.5,tone=.5,blend=.5,level=.5,boost=False,r4=1500,r8=680,source='.1*sin(2*PI*196*time)',supply=9,temp=26.85,beta=350,rs=1000,load=1e6,caps=1):
 def r(n,p,q,x):return f'R{n} {p} {q} {x:.17g}\n' if x else f'V{n} {p} {q} 0\n'
 s=f'''SF016 independently transcribed factory-style drawing 1015, documented-original-style
Vcc rail 0 {supply}
Bsrc source 0 v={{{source}}}
Rsource source input {rs}
R1 input ci 15k
C1 ci b1 100n
R2 b1 0 150k
R3 b1 fb 560k
R4 e1 0 {r4}
R5 rail c1 47k
R6 e2 fb 120k
C2 fb 0 50n
R7 e2 0 10k
Q1 c1 b1 e1 silicon
Q2 rail c1 e2 silicon
C3 e2 st 10u
'''+r('SU','st','sw',10000*(1-sustain))+r('SL','sw','dry',10000*sustain)+f'''R8 dry 0 {r8}
C4 sw b3 100n
R9 b3 0 150k
R10 rail b3 220k
R11 rail c3 8200
R12 e3 0 8200
Q3 c3 b3 e3 silicon
C5 c3 dc 10u
C6 e3 de 10u
R13 dc 0 27k
R14 de 0 27k
R15 rail b4 100k
Q4 c4 b4 e4 silicon
R16 rail c4 10k
R17 e4 0 820
C7 e4 0 10u
C8 c4 clip 10u
C9 clip tin 10u
R18 tin low 27k
R19 low dark 3900
C10 low 0 100n
C11 tin bright 2.7n
'''+r('TL','dark','tone',25000*tone)+r('TH','tone','bright',25000*(1-tone))+'''R21 tone level 100k
'''+('R20 tin tone 47k\n' if boost else 'Vboost tone level 0\nR20 tin toneunused 47k\n')+r('LU','level','lw',50000*(1-level))+r('LL','lw','0',50000*level)+'''C12 lw lb 10u
R22 lb b5 10k
R23 b5 0 18k
R24 rail b5 150k
R25 b5 f5 3900
C13 f5 c5 1n
R26 rail c5 10k
R27 e5 0 820
Q5 c5 b5 e5 silicon
C14 e5 0 10u
C15 c5 wet 10u
'''+r('BU','wet','bw',100000*(1-blend))+r('BL','bw','dry',100000*blend)+f'''C17 bw jack 100n
Rload jack 0 {load}
'''
 for d,p,n in [(1,'b4','dc'),(2,'b4','de'),(3,'0','clip'),(4,'clip','0')]:
  if fault==d:continue
  s+=f'D{d} {p} {n} germanium\nRL{d} {p} {n} 800k\nCJ{d} {p} {n} {caps*.5e-12/.6**.29:.17g}\n'
 for i,b,c,e in [(1,'b1','c1','e1'),(2,'c1','rail','e2'),(3,'b3','c3','e3'),(4,'b4','c4','e4'),(5,'b5','c5','e5')]:
  s+=f'CBC{i} {b} {c} {caps*4e-12:.17g}\nCBE{i} {b} {e} {caps*20e-12:.17g}\n'
 return s+f'''.model silicon NPN(IS=1e-14 BF={beta} BR=4 NF=1 NR=1 CJE=0 CJC=0 TF=0 TR=0 TNOM=26.85)
.model germanium D(IS=560n N={1/(32*VT):.17g} RS=0 TT=4.0625n CJO=0 EG=.67 TNOM=26.85)
.options TEMP={temp} TNOM=26.85 method=gear maxord=2 reltol=1e-7 abstol=1e-13 vntol=1e-9 gmin=1e-12
'''

def generate(exe,work,fixtures):
 work.mkdir(parents=True,exist_ok=True);fixtures.mkdir(parents=True,exist_ok=True);nets=fixtures/'netlists';nets.mkdir(exist_ok=True)
 (work/'spinit').write_text('set num_threads=1\n')
 version=subprocess.check_output([str(exe),'--version'],text=True)
 if 'ngspice-44.2' not in version:raise ValueError('Require pinned ngspice44.2')
 receipts=[]
 def run(name,circuit,analysis,vec,end=None):
  p=work/(name+'.cir');raw=work/(name+'.txt');log=work/(name+'.log')
  p.write_text(circuit+f'\n.control\nset noaskquit\nset numdgt=17\nset wr_singlescale\nset wr_vecnames\n{analysis}\nwrdata {raw.name} {vec}\nquit\n.endc\n.end\n')
  if raw.exists():raw.unlink()
  try:q=subprocess.run([str(exe),'-n','-b',str(p.resolve())],cwd=work,env=dict(os.environ,SPICE_SCRIPTS=str(work.resolve())),capture_output=True,text=True,timeout=60)
  except subprocess.TimeoutExpired as e:
   log.write_bytes((e.stdout or b'')+(e.stderr or b''));raise
  log.write_text(q.stdout+q.stderr)
  if q.returncode or 'aborted' in (q.stdout+q.stderr).lower() or not raw.exists():raise RuntimeError(name+' incomplete reference')
  data=np.loadtxt(raw,skiprows=1,ndmin=2)
  if not len(data) or not np.isfinite(data).all() or (end is not None and data[-1,0]<end-1e-9):raise ValueError(name+' incomplete/nonfinite span')
  receipts.append(dict(name=name,netlist_sha256=sha(p),raw_sha256=sha(raw),rows=len(data),end_s=float(data[-1,0])))
  shutil.copy(p,nets/p.name);raw.unlink();return data
 cases=[]
 for fault in [0,1]:
  for supply in [7.5,9,10]:
   for beta in [250,350,500]:
    for s in [0,.5,1]:cases.append(dict(BASE,fault=fault,supply=supply,beta=beta,sustain=s))
  for key,values in [('temp',[5,40]),('rs',[100,10000]),('load',[100000,2e6]),('tone',[0,1]),('blend',[0,1]),('level',[0,1]),('boost',[True])]:
   for v in values:cases.append(dict(BASE,fault=fault,**{key:v}))
 bias=[]
 for i,c in enumerate(cases):
  d=run('dc-'+str(i),net(**c)+saved(c),'op\nlet zero = v(jack)*0',vectors(c));bias.append([c[k] for k in FIELDS]+d[0,1:].tolist())
 write(fixtures/'bias.csv',FIELDS+NODES+PINS,bias)
 probes=[('held-low',110,.05,.1,{}),('held-high',880,.05,.1,{}),('quiet',196,.005,.08,{}),('hard',196,.2,.08,{}),('pluck',110,.2,.45,{}),('chord',110,.1,.12,{}),('sustain0',196,.05,.08,{'sustain':0}),('sustain1',196,.05,.08,{'sustain':1}),('tone0',196,.05,.08,{'tone':0}),('tone1',196,.05,.08,{'tone':1}),('boost',196,.05,.08,{'boost':True}),('blend0',196,.05,.08,{'blend':0}),('blend1',196,.05,.08,{'blend':1}),('level0',196,.05,.08,{'level':0}),('level1',196,.05,.08,{'level':1}),('source-load',196,.05,.08,{'rs':10000,'load':100000}),('cold',196,.05,.08,{'temp':5}),('hot',196,.05,.08,{'temp':40}),('half-junction-caps',196,.05,.08,{'caps':.5}),('double-junction-caps',196,.05,.08,{'caps':2})]
 metadata=[];payload=bytearray(b'SF016F32'+struct.pack('<II',0,50));counts={};quant=0;rows=0
 def append(id,indices,values):
  nonlocal rows,quant
  for n,v in zip(indices,values):
   b=struct.pack('<50f',*v);decoded=struct.unpack('<50f',b);quant=max(quant,max(abs(float(x)-y) for x,y in zip(v,decoded)));payload.extend(struct.pack('<II',id,int(n))+b);rows+=1
  counts[id]=len(indices)
 def transient(c,tag,hz,amp,end,extra=False):
  id=len(metadata);bypass=tag=='bypass';rate=768000 if extra and not bypass else 192000
  sig=f'{amp}*sin(2*PI*{hz}*time)'
  if tag=='chord':sig=f'{amp}/3*(sin(2*PI*110*time)+sin(2*PI*138.59*time)+sin(2*PI*164.81*time))'
  sig+='*(1-exp(-time/.002))'
  if tag=='pluck':sig+='*exp(-time/.12)'
  s=net(**c,source=sig)+saved(c)
  if bypass:s=s.replace('C17 bw jack 100n','Vbypass input jack 0')
  name=f'tran-{c["fault"]}-'+('bypass' if bypass else 'HF'+str(hz) if extra else tag)
  if extra:
   # Refined8k grid is explicit. Coarse.5us/0.1us failures remain in report data.
   dt='.05u' if hz==8000 else '.5u';mx='.05u' if hz==8000 else '.25u'
   analysis=f'tran {dt} .012 0 {mx}\nlinearize\nlet zero = v(jack)*0'
  else:analysis=f'tran 2u {end} 0 2u\nlet zero = v(jack)*0'
  data=run(name,s,analysis,vectors(c),end);last=round(end*rate)
  if extra:idx=np.unique(np.r_[np.linspace(0,last,501).astype(int),np.arange(last-2048,last+1)])
  else:
   original=set(np.linspace(0,last,1501).astype(int))|set(range(last-2048,last+1))
   whole=[int(i*last/1500) for i in range(1501)]
   keep=set(whole[::1 if tag=='pluck' else 4])|set(range(last-2048,last+1,1 if tag in ['held-low','held-high','quiet','hard','pluck'] else 8))
   idx=np.asarray(sorted(original&keep))
  t=idx/rate;values=np.column_stack([np.interp(t,data[:,0],data[:,j]) for j in range(1,51)]);append(id,idx,values)
  cfg=dict(c,effect=not bypass);metadata.append(dict(id=id,kind=tag,hz=hz,amplitude=amp,end_s=end,grid_rate=rate,config=cfg));print(name,len(data),len(idx),flush=True)
 for fault in [0,1]:
  for tag,hz,amp,end,changes in probes:transient(dict(BASE,fault=fault,**changes),tag,hz,amp,end)
 for fault in [0,1]:
  for hz in [1760,3520,8000,196]:transient(dict(BASE,fault=fault),'bypass' if hz==196 else 'high',hz,.05,.012,True)
 struct.pack_into('<I',payload,8,rows);(fixtures/'transient.f32').write_bytes(payload)
 fields=FIELDS+['effect']
 with (fixtures/'cases.csv').open('w',newline='') as f:
  w=csv.writer(f,lineterminator='\n');w.writerow(['id','kind','hz','amplitude','end_s','grid_rate']+fields)
  for c in metadata:w.writerow([c[k] for k in ['id','kind','hz','amplitude','end_s','grid_rate']]+[int(c['config'][k]) if isinstance(c['config'][k],bool) else c['config'][k] for k in fields])
 manifest=dict(format='SF016F32',version=1,endian='little',row_format='uint32 case, uint32 sample, 50 IEEE754 binary32 values',rows=rows,values_per_row=50,columns=NODES+PINS,cases=counts,quantization_peak_absolute_V_or_A=quant,transient_sha256=sha(fixtures/'transient.f32'),generator_sha256=sha(Path(__file__)),independent='Pinned built-in ngspice BJT/diode, independent full MNA; no native output/stamps',high_frequency_export={'1760_3520_Hz':'0.5us uniform export, max0.25us adaptive','8000_Hz':'0.05us uniform export/maxstep, separately refined from0.1us; original coarse failures preserved'},tool_version=version,reference_receipts=receipts)
 (fixtures/'spice-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--check-existing',type=Path);a=p.parse_args()
 generate(a.ngspice.resolve(),a.output.resolve(),a.fixtures.resolve())
 if a.check_existing:
  for name in ['bias.csv','cases.csv','transient.f32']:
   if sha(a.fixtures/name)!=sha(a.check_existing/name):raise ValueError('Reference replay differs: '+name)
  print('Reference bias/cases/binary replay hashes match')
