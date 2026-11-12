#!/usr/bin/env python3
"""Prepare only the pinned, new Guitar-TECHS DI; no download or effects here."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import wave
import zipfile

ARCHIVE_SHA='033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2'
MEMBER='P3_music/audio/directinput/directinput_11.wav'
SOURCE_SHA='0842bfb4e73142205123d6d362ff8f007e277ce022a4e40dd70b824ecb7543d8'


def prepare(archive,out):
    assert archive.stat().st_size==129505089,'Wrong archive size'
    digest=hashlib.sha256()
    with archive.open('rb') as f:
        for chunk in iter(lambda:f.read(1048576),b''):digest.update(chunk)
    assert digest.hexdigest()==ARCHIVE_SHA,'Wrong archive SHA'
    with zipfile.ZipFile(archive) as z:raw=z.read(MEMBER)
    assert hashlib.sha256(raw).hexdigest()==SOURCE_SHA,'Wrong original recording'
    with wave.open(io.BytesIO(raw)) as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())==(2,3,48000,1014000)
        samples=w.readframes(w.getnframes())
    pcm=bytearray();identical=True
    for i in range(0,len(samples),6):
        a=int.from_bytes(samples[i:i+3],'little',signed=True)
        identical &= samples[i:i+3]==samples[i+3:i+6]
        q=(a+128)//256 if a>=0 else -((-a+128)//256) # nearest, ties away from zero.
        assert -32768<=q<=32767,'Refuse quantization clipping'
        pcm.extend(struct.pack('<h',q))
    assert identical,'Expected dual mono, refuse to silently mix other content'
    out.mkdir(parents=True,exist_ok=True)
    # Caller chooses a disposable output directory; never overwrite the archive.
    for name in ['source-original-stereo24.wav','guitar-techs-mono16.wav','input-receipt.json']:
        p=out/name
        if p.exists():assert not p.samefile(archive),'Output aliases input archive'
    (out/'source-original-stereo24.wav').write_bytes(raw)
    with wave.open(str(out/'guitar-techs-mono16.wav'),'wb') as w:
        w.setparams((1,2,48000,0,'NONE','not compressed'));w.writeframes(pcm)
    mono=out/'guitar-techs-mono16.wav'
    receipt=dict(dataset='Guitar-TECHS',doi='10.5281/zenodo.14963133',record_version='v1',license='CC-BY-4.0',license_url='https://creativecommons.org/licenses/by/4.0/',archive_url='https://zenodo.org/api/records/14963133/files/P3_music.zip/content',archive_sha256=ARCHIVE_SHA,archive_md5='071ba80aecf00f4a31fbd167b3f22198',member=MEMBER,original_sha256=SOURCE_SHA,prepared_sha256=hashlib.sha256(mono.read_bytes()).hexdigest(),source_channels_identical=True,selected_zero_based_channel=0,source_width_bits=24,output_width_bits=16,rate_hz=48000,frames=1014000,conversion='Full performance, channel0 only, nearest/ties-away PCM quantization; no trimming/resampling/EQ/dynamics/gain',distinct_from_rejected_GuitarJam_050=True)
    (out/'input-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();prepare(a.archive,a.output)
