#!/usr/bin/env python3
"""Offline ONE-port virtual-resistor/analog-island experiment; not hardware DSP."""
import argparse,bisect,csv,hashlib,json,math,sys,subprocess,re
from pathlib import Path
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from germanium_spice_reference import CASES,circuit,reference,write_csv

PROJECT=Path(__file__).resolve().parents[2]
VECTORS='v(b1) v(c1) v(e2) v(c2) v(tap) v(jack) i(Vprobe) v(rawcmd) v(drvcmd)'
MODES=[
 dict(name='continuous',rate=0,delay=0,bits=0,driver_tau=0,sensor_tau=0,offset=0,gain=0,extra_r=0),
 dict(name='fast_192k',rate=192000,delay=1e-6,bits=16,driver_tau=.2e-6,sensor_tau=.1e-6,offset=1e-9,gain=1e-4,extra_r=1),
 dict(name='codec_48k',rate=48000,delay=300e-6,bits=16,driver_tau=.2e-6,sensor_tau=.1e-6,offset=1e-9,gain=1e-4,extra_r=1)]
def network(case,mode,port_reference=False):
    name,fuzz,rs,amplitude,leak,feedback=case;r0=100
    n=circuit(fuzz,r0+mode['extra_r'],amplitude,leak,feedback)
    n=n.replace(f'Vinput source 0 SIN(0 {amplitude:.17g} 997)','')
    if port_reference:n=n.replace(f'Rsource source intermediate {r0+mode["extra_r"]:.17g}',f'Rsource source intermediate {rs+mode["extra_r"]:.17g}')
    n+=f'\nVprobe drive source 0\nBstim stimulus 0 V={{{amplitude:.17g}*sin(2*pi*997*time)}}\n'
    virtual=0 if port_reference else rs-r0
    if mode['rate']:
        n+=f"""Bisense sensor0 0 V={{1000*(i(Vprobe)*(1+{mode['gain']:.17g})+{mode['offset']:.17g})}}
Rsen sensor0 sensor1 1000
Csen sensor1 0 {mode['sensor_tau']/1000:.17g}
Vword drvcmd 0 0
Braw rawcmd 0 V={{v(drvcmd)}}
Rsettle drvcmd settled 1000
Csettle settled 0 {mode['driver_tau']/1000:.17g}
Edrive drive 0 settled 0 1
"""
    else:
        n+=f'Bsum rawcmd 0 V={{v(stimulus)-{virtual:.17g}*i(Vprobe)}}\nBcommand drvcmd 0 V={{v(rawcmd)}}\nEdrive drive 0 drvcmd 0 1\n'
    return n+'\n.options method=gear maxord=2\n'
def analysis(case,mode,port_reference=False):
    if not mode['rate']:return 'tran 0.25u 0.012 0 0.25u'
    events={}
    # Both read and delivery times are explicit; words remain constant between updates.
    for n in range(1,int(.012*mode['rate'])):
        sample=n/mode['rate'];delivery=sample+mode['delay']
        events.setdefault(sample,[]).append(('sample',n))
        if delivery<.012:events.setdefault(delivery,[]).append(('update',n))
    schedule=sorted(events.items())
    out=['set numdgt=17','let adc_clips=0','let dac_clips=0']
    out += [f'stop when time={time:.17g}' for time,_ in schedule]
    out.append('tran 0.25u 0.012 0 0.25u')
    half=2**(mode['bits']-1);lsb=1/half
    virtual=0 if port_reference else case[2]-100
    for event_index,(time,actions) in enumerate(schedule):
        out += ['let actual_time=time[length(time)-1]',f'echo EVENT {event_index}','print actual_time']
        for action,n in actions:
            if action=='sample':
                u=case[3]*math.sin(2*math.pi*997*n/mode['rate'])
                out += [f'let adc_code=floor(v(sensor1)[length(time)-1]/{lsb:.17g}+0.5)',
                        f'if adc_code>{half-1}','let adc_clips=adc_clips+1',f'let adc_code={half-1}','end',
                        f'if adc_code< -{half}','let adc_clips=adc_clips+1',f'let adc_code= -{half}','end',
                        f'let dac_code=floor(({u:.17g}-{virtual*lsb/1000:.17g}*adc_code)/{lsb:.17g}+0.5)',
                        f'if dac_code>{half-1}','let dac_clips=dac_clips+1',f'let dac_code={half-1}','end',
                        f'if dac_code< -{half}','let dac_clips=dac_clips+1',f'let dac_code= -{half}','end',
                        f'let word{n}=dac_code*{lsb:.17g}']
            else:out.append(f'alter Vword dc=$&word{n}')
        out += [f'delete {event_index+1}','resume']
    out += ['echo CLIPS $&adc_clips $&dac_clips']
    return '\n'.join(out)
def interpolate(rows,time):
    times=[r[0] for r in rows] if not isinstance(rows,tuple) else rows[0]
    values=rows if not isinstance(rows,tuple) else rows[1]
    hi=bisect.bisect_left(times,time);lo=max(0,hi-1);hi=min(hi,len(values)-1)
    if not times[lo]<=time<=times[hi]:raise ValueError('Missing reference time')
    weight=(time-times[lo])/(times[hi]-times[lo]) if hi!=lo else 0
    return [values[lo][i]+weight*(values[hi][i]-values[lo][i]) for i in range(1,len(values[0]))]
def errors(a,b):
    nodes=[x[j]-y[j] for x,y in zip(a,b) for j in range(5)]
    jack=[x[5]-y[5] for x,y in zip(a,b)];amps=[x[6]-y[6] for x,y in zip(a,b)]
    rms=lambda v:math.sqrt(sum(x*x for x in v)/len(v))
    return dict(node_rms_v=rms(nodes),node_peak_v=max(map(abs,nodes)),jack_rms_v=rms(jack),jack_peak_v=max(map(abs,jack)),input_rms_a=rms(amps))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ngspice',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--case',choices=[c[0] for c in CASES]);p.add_argument('--mode',choices=[m['name'] for m in MODES],required=True);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True);(a.output/'spinit').write_text('* SF004 offline boundary experiment\nset num_threads=1\n')
    version=__import__('subprocess').check_output([a.ngspice,'--version'],text=True).strip()
    if 'ngspice-44.2' not in version:raise ValueError('Use existing pinned ngspice44.2 package')
    fixtures={}
    with (PROJECT/'tests/fixtures/germanium/transient.csv').open() as f:
        for r in csv.DictReader(f):fixtures.setdefault(r['case'],[]).append(r)
    summary=[];samples=[]
    for case in CASES:
        if a.case and case[0]!=a.case:continue
        for mode in MODES:
            if a.mode and mode['name']!=a.mode:continue
            label=case[0]+'-'+mode['name']
            try:
                rows=reference(a.ngspice,a.output,label,network(case,mode),analysis(case,mode),VECTORS)
                path=a.output/(label+'.txt');rawhash=hashlib.sha256(path.read_bytes()).hexdigest();path.unlink()
                log=(a.output/(label+'.log')).read_text()
                if re.search(r'simulation(?:\(s\))? aborted|Timestep too small',log):
                    failure=next((line.strip() for line in log.splitlines() if 'Timestep too small' in line),'SPICE analysis aborted')
                    raise ValueError('NUMERICAL BLOCKER, not physical instability: '+failure)
                if rows[-1][0]<.01199:raise ValueError('Incomplete transient span')
                indexed=([r[0] for r in rows],rows)
                points=[interpolate(indexed,int(r['sample_96khz'])/96000) for r in fixtures[case[0]]]
                expected=[[float(r[k]) for k in ('base1_volts','collector1_volts','emitter2_volts','collector2_volts','tap_volts','jack_volts','input_amps')] for r in fixtures[case[0]]]
                if mode['rate']:
                    refname=label+'-same-source-reference'
                    ref=reference(a.ngspice,a.output,refname,network(case,mode,True),analysis(case,mode,True),VECTORS)
                    (a.output/(refname+'.txt')).unlink()
                    refidx=([r[0] for r in ref],ref)
                    boundary_reference=[interpolate(refidx,int(r['sample_96khz'])/96000) for r in fixtures[case[0]]]
                else:boundary_reference=expected
                raw=errors(points,expected);boundary=errors(points,boundary_reference)
                saturated=sum(abs(r[8])>1 or abs(r[9])>=1-1/32768 for r in rows)/len(rows)
                if mode['rate']:
                    log=(a.output/(label+'.log')).read_text()
                    clips=next(line.split()[1:] for line in log.splitlines() if line.startswith('CLIPS '))
                    saturated=sum(map(float,clips))/int(.012*mode['rate'])
                    # A short/aborted simulation must never pass on its partial output.
                    if 'simulation(s) aborted' in log:raise ValueError('SPICE transient aborted')
                    event_times=[float(line.split('=',1)[1]) for line in log.splitlines() if line.startswith('actual_time =')]
                    expected_times=sorted(set([n/mode['rate'] for n in range(1,int(.012*mode['rate']))]+[n/mode['rate']+mode['delay'] for n in range(1,int(.012*mode['rate'])) if n/mode['rate']+mode['delay']<.012]))
                    if len(event_times)!=len(expected_times) or max(abs(x-y) for x,y in zip(event_times,expected_times))>1e-12:raise ValueError('Sample/delivery times not exact')
                dc=reference(a.ngspice,a.output,label+'-dc',network(case,mode),'op',VECTORS)[0]
                (a.output/(label+'-dc.txt')).unlink()
                with (PROJECT/'tests/fixtures/germanium/bias.csv').open() as f:bias=next(r for r in csv.DictReader(f) if r['case']==case[0])
                dcerror=max(abs(dc[i+1]-float(bias[k])) for i,k in enumerate(('base1_volts','collector1_volts','emitter2_volts','collector2_volts','tap_volts','jack_volts')))
                original_pass=raw['node_rms_v']<.02 and raw['node_peak_v']<.2 and raw['jack_rms_v']<.0015 and raw['jack_peak_v']<.01 and raw['input_rms_a']<1e-6
                boundary_pass=boundary['jack_rms_v']<=.0002 and boundary['jack_peak_v']<=.001 and boundary['input_rms_a']<=1e-7
                record=dict(case=case[0],mode=mode,raw=raw,boundary=boundary,dc_peak_v=dcerror,saturated_row_fraction=saturated,
                    original_fixture_gate=original_pass,boundary_gate=boundary_pass,passes=original_pass and boundary_pass and dcerror<=.001 and saturated==0,
                    same_source_boundary_pass=boundary_pass and dcerror<=.001 and saturated==0,
                    raw_rows=len(rows),raw_sha256=rawhash)
                for f,actual,refpoint in zip(fixtures[case[0]],points,boundary_reference):samples.append([case[0],mode['name'],f['sample_96khz'],*actual[:7],*refpoint[:7]])
            except (RuntimeError,ValueError,subprocess.TimeoutExpired) as exc:record=dict(case=case[0],mode=mode,passes=False,error=str(exc))
            summary.append(record);print(label,'PASS' if record['passes'] else 'FAIL',record.get('error',record.get('boundary')),flush=True)
            (a.output/'results.json').write_text(json.dumps(dict(schema=1,scope='virtual HIL stand-in, ONE delayed source-impedance port; NOT hardware',simulator=version,assumptions='tools/sf004/PLAN.md',cases=summary),indent=2)+'\n')
    write_csv(a.output/'sampled-traces.csv',['case','mode','sample96k',*[x+'_'+s for s in ('actual','same_source_ref') for x in ('b1','c1','e2','c2','tap','jack','input_amps')]],samples)
    if any('error' in record for record in summary):raise SystemExit(2)
if __name__=='__main__':main()
