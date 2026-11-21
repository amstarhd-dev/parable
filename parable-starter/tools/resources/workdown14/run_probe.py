import csv, hashlib, json, sys, time
from pathlib import Path
from seed3_oneboot_batch import Device
from seed3_runner import atomic,require,Stop

root=Path(__file__).resolve().parent
names=[p+'/'+str(w)+' branches' for p in ('cmos','germanium','muff','klon','blender','fz1','tape_saturation') for w in (6,12,20)]
names+=['rat/native1x','rat/2x33','rat/4x33','spring/48 modes','spring/96 modes','spring/192 modes','leo_hall/half rate','leo_hall/full rate','chain/4x native','chain/1x rotor 8','chain/1x rotor 32']
def run():
    manifest=json.loads((root/'manifest.json').read_text())
    binary=root/'workdown14.bin'; data=binary.read_bytes()
    require(8<len(data)<=131072 and hashlib.sha256(data).hexdigest()==manifest['sha256'],'Firmware checksum or size mismatch')
    require(sys.argv[1:] in ([],['--read-only']),'Supported option is --read-only')
    out=root/('measurement-'+time.strftime('%Y%m%d-%H%M%S'));out.mkdir()
    d=Device(Path.home()/'Parable-PAR009/runner84-v11/darwin_amd64/dfu-util',out,'200364500000')
    if not sys.argv[1:]:
        print('Looking for your Daisy in ROM DFU. Hold BOOT, tap RESET, then release BOOT if needed.',flush=True)
        selected=d.wait_dfu(45);d.flash_selected(selected,binary)
    print(f'Measuring {len(names)} candidates. Allow about two minutes. The board blinks between captures.',flush=True)
    port=d.open_cdc(210);end=time.monotonic()+18;header=None;rows={}
    try:
        while len(rows)<len(names):
            line=port.line(end).strip()
            if line.startswith('WORK14 '):
                fields=dict(x.split('=',1) for x in line.split()[1:])
                require(fields.get('id')==manifest['id'],'Unexpected firmware identity')
                for field,value in dict(seed3=1,core_hz=480000000,rate=48000,frames=48,counter=1,count=len(names)).items():
                    require(int(fields[field])==value,'Invalid '+field)
                header=line
            elif header and line.startswith('ROW '):
                i,blocks,peak,hits,bad,frames=map(int,line.split()[1:])
                require(0<=i<len(names),'Invalid row')
                require(i not in rows or rows[i]==(blocks,peak,hits,bad,frames),'Inconsistent repeated capture')
                rows[i]=(blocks,peak,hits,bad,frames)
    finally:port.close()
    raw=[header]+['ROW '+' '.join(map(str,[i,*rows[i]])) for i in range(len(names))]
    atomic(out/'report.txt',('\n'.join(raw)+'\n').encode('ascii'))
    with (out/'timing.csv').open('w',newline='') as f:
        writer=csv.writer(f);writer.writerow(['candidate','blocks','peak_cycles','block_percent','deadline_hits','nonfinite','bad_frames','timing'])
        for i,name in enumerate(names):
            n,peak,hits,bad,frames=rows[i];pct=peak/4800
            status='invalid' if not 0<n<=3000 or not peak or bad or frames else 'deadline miss' if hits else 'incomplete' if n!=3000 else 'within 20%' if peak<=96000 else 'above 20%'
            print(f'{name:27s} {pct:6.2f}%   {status}');writer.writerow([name,n,peak,round(pct,4),hits,bad,frames,status])
    print('Saved:',out/'timing.csv')
if __name__=='__main__':
    try:run()
    except (Stop,OSError,ValueError,KeyError) as error:sys.exit('STOP: '+str(error))
