#!/usr/bin/env python3
"""Verify and prepare the pinned NEW Guitar-TECHS take03, host evidence only."""
import argparse,hashlib,io,json,struct,wave,zipfile
from pathlib import Path
ARCHIVE_SHA='033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2'
SOURCE_SHA='84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0'
PREPARED_SHA='75ae5e73c72e6774fb75e6318dc91ba66347fb65290949e947144033eb648f52'
MEMBER='P3_music/audio/directinput/directinput_03.wav'
def prepare(archive,out):
    digest=hashlib.sha256()
    with archive.open('rb') as f:
        for chunk in iter(lambda:f.read(1048576),b''):digest.update(chunk)
    if digest.hexdigest()!=ARCHIVE_SHA:raise ValueError('Wrong pinned archive')
    with zipfile.ZipFile(archive) as z:original=z.read(MEMBER)
    if hashlib.sha256(original).hexdigest()!=SOURCE_SHA:raise ValueError('Wrong original performance')
    with wave.open(io.BytesIO(original)) as w:
        if (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())!=(2,3,48000,1406000):raise ValueError('Unexpected source format')
        data=w.readframes(w.getnframes())
    if any(data[i:i+3]!=data[i+3:i+6] for i in range(0,len(data),6)):raise ValueError('Expected identical dual-mono channels')
    pcm=bytearray()
    # Explicit first20s excerpt preserves the unchanged laboratory's30s WAV bound
    # and leaves2s for the circuit tail. Full original29.291666667s is retained separately.
    for i in range(0,20*48000*6,6):
        value=int.from_bytes(data[i:i+3],'little',signed=True)
        q=(value+128)//256 if value>=0 else -((-value+128)//256)
        if not -32768<=q<=32767:raise ValueError('Quantization clipping')
        pcm.extend(struct.pack('<h',q))
    buffer=io.BytesIO()
    with wave.open(buffer,'wb') as w:w.setparams((1,2,48000,0,'NONE','not compressed'));w.writeframes(pcm)
    prepared=buffer.getvalue()
    if hashlib.sha256(prepared).hexdigest()!=PREPARED_SHA:raise ValueError('Prepared-source mismatch')
    out.mkdir(parents=True,exist_ok=True)
    for name in ['source-original-stereo24.wav','take03-20s-mono16.wav','source-receipt.json']:
        if (out/name).exists() and (out/name).samefile(archive):raise ValueError('Output aliases input archive')
    (out/'source-original-stereo24.wav').write_bytes(original);(out/'take03-20s-mono16.wav').write_bytes(prepared)
    receipt=dict(dataset='Guitar-TECHS',record='14963133',version='record14963133',license='CC-BY-4.0',license_url='https://creativecommons.org/licenses/by/4.0/',source_url='https://zenodo.org/api/records/14963133/files/P3_music.zip/content',archive_sha256=ARCHIVE_SHA,member=MEMBER,original_sha256=SOURCE_SHA,prepared_sha256=PREPARED_SHA,original_frames=1406000,prepared_frames=960000,rate_hz=48000,source_channels_identical=True,selected_zero_based_channel=0,excerpt_start_frame=0,excerpt_end_frame=960000,conversion='First20s of full29.291666667s performance, channel0,24->16bit nearest/ties-away quantization; no resampling/EQ/dynamics/gain',distinct_from_GuitarJam050_take11_and_take06=True)
    (out/'source-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();prepare(a.archive,a.output)
