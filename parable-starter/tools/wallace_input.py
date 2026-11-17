#!/usr/bin/env python3
"""Exact late take03 excerpt, preserving natural gaps/sustain; no invented signal."""
import argparse,hashlib,io,json,struct,wave
from pathlib import Path
from klon_input import SOURCE_SHA,ARCHIVE_SHA,MEMBER
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();original=a.original.read_bytes();assert hashlib.sha256(original).hexdigest()==SOURCE_SHA,'wrong original take03'
with wave.open(io.BytesIO(original)) as w:
 assert (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())==(2,3,48000,1406000)
 w.setpos(445440);data=w.readframes(960000)
assert len(data)==5760000
pcm=bytearray()
for i in range(0,len(data),6):
 assert data[i:i+3]==data[i+3:i+6],'source channels differ'
 v=int.from_bytes(data[i:i+3],'little',signed=True);q=(v+128)//256 if v>=0 else -((-v+128)//256);pcm.extend(struct.pack('<h',q))
b=io.BytesIO()
with wave.open(b,'wb') as w:w.setparams((1,2,48000,0,'NONE','not compressed'));w.writeframes(pcm)
a.output.mkdir(parents=True,exist_ok=True);path=a.output/'take03-late20s.wav';path.write_bytes(b.getvalue());receipt=dict(dataset='Guitar-TECHS',record='14963133',license='CC-BY-4.0',authors=['Hegel Emmanuel Pedroza Villalobos','Termeh Taheri','Wallace Abreu','Ryan M. Corey','Iran R. Roman'],source='https://zenodo.org/records/14963133',archive_sha256=ARCHIVE_SHA,member=MEMBER,original_sha256=SOURCE_SHA,prepared_sha256=hashlib.sha256(b.getvalue()).hexdigest(),start_frame=445440,end_frame=1405440,start_seconds=9.28,end_seconds=29.28,rate=48000,frames=960000,channel=0,changes='Excerpt only,24->16 nearest/ties-away, no resample/EQ/dynamics/gating/gain;9s zero tail added by C++ renderer.')
(a.output/'input-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
