#!/usr/bin/env python3
"""Exact two-pole sampled interface to a RESISTOR surrogate, NOT the GP circuit."""
import argparse,heapq,json,math
from pathlib import Path

def quantize(value,span=1,bits=16):
    half=2**(bits-1);raw=math.floor(value/span*half+.5)
    return min(half-1,max(-half,raw))*span/half,raw< -half or raw>half-1

def advance(v,s,command,dt,gain,td=.2e-6,ts=.1e-6):
    ed,es=math.exp(-dt/td),math.exp(-dt/ts)
    return (command+(v-command)*ed,
            gain*command+(s-gain*command)*es+gain*(v-command)*td/(td-ts)*(ed-es))

def experiment(source,rate,delay):
    # Rload1048 is only a constant-resistance diagnostic inspired by the published
    # ~1kHz Zin real part. It discards the GP circuit's dynamics/nonlinearity.
    rload=1048.;r0=100.;burden=1.;g=1/(r0+burden+rload)
    virtual=source-r0;duration=.05;level=.05;freq=997
    events=[];frames=int(duration*rate)
    for n in range(frames):
        heapq.heappush(events,(n/rate,0,n,None))
    v=s=command=vr=sr=reference_command=0.;time=0.;clips=0;values=[]
    while events:
        t,kind,n,word=heapq.heappop(events)
        v,s=advance(v,s,command,t-time,g)
        vr,sr=advance(vr,sr,reference_command,t-time,1/(source+burden+rload))
        time=t
        if kind==0:
            u=level*math.sin(2*math.pi*freq*t)
            measured,adc_clip=quantize((s*(1+1e-4)+1e-9)*1000)
            delivered,dac_clip=quantize(u-virtual*measured/1000)
            baseline,_=quantize(u)
            clips+=adc_clip+dac_clip
            if t+delay<duration:heapq.heappush(events,(t+delay,1,n,(delivered,baseline)))
            if t>.01:values.append((t,rload*g*v,rload*vr/(source+burden+rload),g*v,vr/(source+burden+rload)))
        else:command,reference_command=word
    differences=[row[1]-row[2] for row in values]
    currents=[row[3]-row[4] for row in values]
    rms=lambda x:math.sqrt(sum(v*v for v in x)/len(x))
    return dict(source_ohms=source,rate=rate,transport_s=delay,duration_s=duration,source_peak_v=level,source_frequency_hz=freq,driver_tau_s=.2e-6,sensor_tau_s=.1e-6,
        r0_ohms=r0,sensor_burden_ohms=burden,rload_surrogate_ohms=rload,adc_gain_error=1e-4,adc_offset_a=1e-9,
        bits=16,adc_current_span_a=.002,dac_span_v=2,clipped_samples=clips,
        output_rms_error_v=rms(differences),output_peak_error_v=max(map(abs,differences)),input_rms_error_a=rms(currents),
        output_peak_v=max(abs(row[1]) for row in values),
        boundary_budget_pass=clips==0 and rms(differences)<=.0002 and max(map(abs,differences))<=.001 and rms(currents)<=1e-7,
        ideal_memoryless_zero_pole=-virtual/(rload+r0+burden))

def calculate():
    # Analytic ODE self-check, independent of delayed-loop recurrence.
    v,s=advance(0,0,1,.2e-6,1)
    assert abs(v-(1-math.exp(-1)))<1e-14
    assert abs(s-(1-2*math.exp(-1)+math.exp(-2)))<1e-14
    return dict(schema=1,scope='Linear resistor surrogate ONLY, not nonlinear germanium, not hardware',
        reference='same sampled/quantized/transported source, all resistance physically present; exact continuous two-pole evolution',
        omitted=['GP transistor dynamics/nonlinearity','Cin/bypass/Cbc/Cout','voltage/current noise','converter HPF/decimator/full transfer response','global stability proof'],
        cases=[experiment(rs,rate,delay) for rs in (1000,50000) for rate,delay in ((192000,1e-6),(48000,300e-6))])
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(calculate(),indent=2)+'\n')
