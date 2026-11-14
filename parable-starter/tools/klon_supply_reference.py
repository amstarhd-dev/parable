"""Offline supply-domain audit, not a firmware charge-pump implementation.

Literal original C19/C20/C21/C22/D5/D6 topology with an illustrative four-switch
ICL7660S equivalent. Switch resistance/ESR/load/frequency are assumptions and
sweeps, NOT measured vintage supply/PSRR/driver-current equivalence.
"""
import math

def audit(run,circuit,write,fixtures):
    rows=[]
    # Renesas FN3179 typical5V Rsw23ohm, oscillator10k,BOOST~3.5x,pump=fosc/2.
    # Applying those typicals at9V is an explicit illustration, not calibration.
    conditions=[(17500,23,1,.001),(17500,23,1,.0028),(17500,23,1,.005),
                (10000,23,1,.0028),(35000,23,1,.0028),(17500,10,1,.0028),(17500,50,1,.0028),(17500,23,5,.0028)]
    for i,(frequency,ron,esr,load) in enumerate(conditions):
        period=1/frequency
        c=f'''Independent original S698 inverter+doubler topology; illustrative IC switches
V9 p9 0 9
Vclock clock 0 PULSE(1 0 0 20n 20n {period/2-20e-9:.17g} {period:.17g})
Bbar bar 0 V=1-v(clock)
S1 p9 cp clock 0 sw
S2 cm 0 clock 0 sw
S3 cp 0 bar 0 sw
S4 cm neg bar 0 sw
.model sw SW(RON={ron:.17g} ROFF=1e12 VT=.5 VH=.01)
R21 cp c21r {esr:.17g}
C21 c21r cm 1u
R19 neg c19r {esr:.17g}
C19 c19r 0 1u
R20 cp c20r {esr:.17g}
C20 c20r mid 1u
D5 p9 mid rectifier
D6 mid pos rectifier
R22 pos c22r {esr:.17g}
C22 c22r 0 1u
.model rectifier D(IS=10n N=2 RS=1 CJO=0 TT=0)
Iload pos neg {load:.17g}
RleakP pos 0 1e9
RleakN neg 0 1e9
'''
        data=run('supply-island-'+str(i),c,'option method=gear maxord=2\ntran .5u .03 0 .5u uic','v(pos) v(neg)')
        assert data[-1][0]>=.03-1e-10
        data=[r for r in data if r[0]>=.02]
        # Time-weighted moments: adaptive SPICE row density must not bias means.
        duration=data[-1][0]-data[0][0];means=[sum((b[0]-a[0])*(a[k]+b[k])/2 for a,b in zip(data,data[1:]))/duration for k in [1,2]]
        assert 9<means[0]<18 and -9<means[1]<0,(i,means)
        rows.append([frequency,ron,esr,load,*means,max(r[1] for r in data)-min(r[1] for r in data),max(r[2] for r in data)-min(r[2] for r in data)])
    write(fixtures/'supply-island.csv',['pump_hz','switch_ohms','cap_ESR_ohms','load_A','mean_positive_V','mean_negative_V','positive_ripple_pp_V','negative_ripple_pp_V'],rows)
    sensitivity=[]
    # The audio macro exposes supply ONLY through output rails (no PSRR/current
    # model). Test exactly that domain reduction; don't claim clock-noise fidelity.
    for label,g,t,amp,hz in [('push',0,.5,.2,1000),('blend',.5,.5,.2,1000),('high',.95,.5,.2,1000),('shelf',.95,1,.35,1000)]:
        baseline=None
        for variant,pos,neg in [('nominal',16.2,-8.6),('ideal',18,-9),('sag',15.2,-7.6),('wrong_single9_domain',9,0)]:
            c=circuit(g=g,t=t,amp=amp,f=hz)
            c=c.replace('V18 p18 0 16.2',f'V18 p18 0 {pos:.17g}').replace('VN neg 0 -8.6',f'VN neg 0 {neg:.17g}')
            c=c.replace('LO=-11.6 HI=10.2',f'LO={neg-3:.17g} HI={pos-6:.17g}')
            data=run(f'supply-audio-{label}-{variant}',c,'option method=gear maxord=2\ntran 1u .04 0 .5u uic','v(out) v(xu2a.u) v(xu2b.u)')
            assert data[-1][0]>=.04-1e-10
            # Uniform final10ms avoids differing adaptive grids/means.
            import bisect
            def at(time):
                k=bisect.bisect_left(data,time,key=lambda r:r[0]);k=max(1,min(k,len(data)-1));a,b=data[k-1],data[k];q=(time-a[0])/(b[0]-a[0]);return [a[j]+q*(b[j]-a[j]) for j in range(1,4)]
            values=[at(.03+k/192000) for k in range(1920)]
            if baseline is None:baseline=values
            differences=[a[0]-b[0] for a,b in zip(values,baseline)]
            sensitivity.append([label,variant,g,t,amp,hz,pos,neg,math.sqrt(sum(d*d for d in differences)/len(differences)),max(abs(d) for d in differences),min(v[0] for v in values),max(v[0] for v in values),max(abs(v[1]) for v in values),max(abs(v[2]) for v in values)])
    write(fixtures/'supply-sensitivity.csv',['case','variant','gain','treble','amplitude_V','hz','positive_V','negative_V','jack_difference_rms_V','jack_difference_peak_V','jack_min_V','jack_max_V','sum_relative4V5_peak_V','tone_relative4V5_peak_V'],sensitivity)
