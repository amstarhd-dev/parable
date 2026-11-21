import csv, hashlib, json, sys, time
from pathlib import Path
from seed3_oneboot_batch import Device, cdc_identity
from seed3_runner import atomic,require,Stop

root=Path(__file__).resolve().parent
names=[p+'/'+str(w)+' branches' for p in ('cmos','germanium','muff','klon','blender','fz1','tape_saturation') for w in (6,12,20)]
names+=['rat/native1x','rat/2x33','rat/4x33','spring/48 modes','spring/96 modes','spring/192 modes','leo_hall/half rate','leo_hall/full rate','chain/4x native','chain/1x rotor 8','chain/1x rotor 32']
intervals=[(0,21),(21,27),(27,29),(29,32)]
labels=['drives and tape','Rat and spring','Leo Hall','complete Scorpio chain']
def capture(port,m):
    bank=m['bank'];low,high=intervals[bank];end=time.monotonic()+18
    while True:
        line=port.line(end).strip()
        if line.startswith('WORK14 '):break
    fields=dict(pair.split('=',1) for pair in line.split()[1:]);raw=[line];rows=[]
    require(fields.get('id')==m['id'],'Unexpected firmware identity')
    for field,value in dict(seed3=1,core_hz=480000000,rate=48000,frames=48,counter=1,bank=bank,count=high-low).items():
        require(int(fields[field])==value,'Invalid '+field)
    for i in range(low,high):
        line=port.line(end).strip();parts=line.split();raw.append(line)
        require(len(parts)==7 and parts[0]=='ROW' and int(parts[1])==i,'Invalid or missing candidate')
        rows.append([i,*map(int,parts[2:])])
    return '\n'.join(raw)+'\n',rows
def save(out,results):
    import io
    buf=io.StringIO();writer=csv.writer(buf)
    writer.writerow(['candidate','blocks','peak_cycles','block_percent','deadline_hits','nonfinite','bad_frames','timing'])
    for i,n,peak,hits,bad,frames in results:
        pct=peak/4800
        status='invalid' if not 0<n<=3000 or not peak or bad or frames else 'deadline miss' if hits else 'incomplete' if n!=3000 else 'within 20%' if peak<=96000 else 'above 20%'
        writer.writerow([names[i],n,peak,round(pct,4),hits,bad,frames,status])
    atomic(out/'timing.csv',buf.getvalue().encode())
def run():
    banks=json.loads((root/'manifest.json').read_text())['banks']
    require([m['bank'] for m in banks]==list(range(4)),'Invalid bank list')
    for m in banks:
        data=(root/m['bin']).read_bytes()
        require(8<len(data)==m['size']<=131072 and hashlib.sha256(data).hexdigest()==m['sha256'],'Firmware checksum or size mismatch')
    require(len(sys.argv)==1 or len(sys.argv)==3 and sys.argv[1]=='--resume','Usage: RUN_14.command [--resume measurement-folder]')
    if len(sys.argv)==3:
        out=Path(sys.argv[2]).resolve();require(out.is_dir(),'Missing measurement folder')
        state=json.loads((out/'progress.json').read_text())
        require(state['ids']==[m['id'] for m in banks],'Resume folder belongs to another build')
    else:
        out=root/('measurement-'+time.strftime('%Y%m%d-%H%M%S'));out.mkdir()
        state=dict(ids=[m['id'] for m in banks],bank=0,phase='flash',rows=[],silicon=None,selected=None)
    def checkpoint():atomic(out/'progress.json',json.dumps(state,indent=2).encode())
    checkpoint()
    d=Device(Path.home()/'Parable-PAR009/runner84-v11/darwin_amd64/dfu-util',out,'200364500000')
    print('Results:',out,flush=True)
    for bank in range(state['bank'],4):
        m=banks[bank];label='bank-'+str(bank)
        d.sequence=bank
        if state['phase']=='flash':
            if bank==0:print('Hold BOOT, tap RESET, then release BOOT to enter ROM DFU.',flush=True)
            selected=d.wait_dfu(60)
            require(state['selected'] in (None,selected),'ROM device changed')
            state.update(selected=selected,phase='capture');checkpoint()
            print(f'Flashing {bank+1}/4: {labels[bank]}',flush=True)
            d.flash_selected(selected,root/m['bin'])
        if state['phase']=='capture':
            print(f'Waiting for {labels[bank]} ({3*(intervals[bank][1]-intervals[bank][0])} seconds of audio if every candidate completes)…',flush=True)
            port=d.open_cdc(180)
            try:
                raw,rows=capture(port,m);atomic(out/(label+'.txt'),raw.encode('ascii'))
                nonce,control,silicon=cdc_identity(port,{'artifact_id':m['id']})
                require(state['silicon'] in (None,silicon),'Native silicon identity changed')
                state['silicon']=silicon
                atomic(out/(label+'.identity.json'),json.dumps(control).encode())
                state['rows']=[row for row in state['rows'] if row[0]<intervals[bank][0]]+rows
                save(out,state['rows']);checkpoint()
                for i,n,peak,hits,bad,frames in rows:print(f'{names[i]:27s} {peak/4800:6.2f}%  blocks {n}  hits {hits}  nonfinite {bad}  bad frames {frames}',flush=True)
                # Final bank stays in the reporting application for inspection.
                if bank<3:
                    port.send('ENTER_DFU '+m['id']+' '+nonce+'\n')
                    end=time.monotonic()+10
                    while port.line(end)!='DFU_AUTHORIZED '+nonce+'\n':pass
                    state['phase']='return';checkpoint()
            finally:port.close()
        if bank<3:
            require(d.wait_dfu(60)==state['selected'],'ROM identity changed during handoff')
        state.update(bank=bank+1,phase='flash');checkpoint()
    print('Completed all four banks. Saved:',out/'timing.csv')
if __name__=='__main__':
    try:run()
    except (Stop,OSError,ValueError,KeyError) as error:sys.exit('STOP: '+str(error))
