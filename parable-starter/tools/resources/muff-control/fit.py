"""Fit a shared six-branch nonlinear core for each R15 state."""
from pathlib import Path
import sys
import numpy as np
from scipy.signal import bilinear, freqz, lfilter

CAPTURES = Path(sys.argv[1])
POLES = np.exp(-2*np.pi*np.array([30,180,1000,5500])/48000)
POST = np.exp(-2*np.pi*np.array([180,1000,5500])/48000)
WIDTH = 6
DELAY = 48
KNOBS = (0., .25, .5, .75, 1.)

def read(name):
    return np.fromfile(CAPTURES / (name+".f32"), "<f4").astype(float)

def filt(x,p):
    return lfilter([1-p],[1,-p],x,axis=0)

def tone_coeff(t):
    # 39k/10n low branch, 4n/22k high branch, 100k pot and 100n
    # coupling into the estimated 33k loading of the recovery stage.
    num=[-14157*t,-50457000,412500000000*t-503250000000,0]
    den=[42900*t*t-42900*t-14157,597500000*t*t-661850000*t-293914500,
         1250000000000*t*t-1037500000000*t-1333640000000,-201250000000000]
    return bilinear(num,den,fs=48000)

def deembed(y):
    n=len(y)*2
    omega=2*np.pi*np.fft.rfftfreq(n)
    _,h=freqz(*tone_coeff(.5),worN=omega)
    inverse=np.conj(h)/(abs(h)**2+0.0002)
    return np.fft.irfft(np.fft.rfft(y,n)*inverse,n)[:len(y)]

def features(x):
    p=np.stack([filt(x,k) for k in POLES],axis=1)
    return np.column_stack([x,x-np.r_[0,x[:-1]],p,x[:,None]-p,
                            *[filt(abs(x),k) for k in POLES[:2]],
                            *[filt(np.clip(x*10,-1,1),k) for k in POLES[:2]]])

def fmt(x):
    if np.ndim(x)==0:return f"{x:.9e}f"
    return "{"+",".join(map(fmt,x))+"}"

def fit(fault,x):
    x=x[:-DELAY]
    scale=np.maximum(.002,np.std(x,axis=0))
    x=x/scale
    targets=np.array([deembed(read(f"train-{fault}-{s}-0.5-0.5"))[DELAY:] for s in KNOBS])
    norm=np.sqrt(np.mean(targets**2))
    targets/=norm
    rng=np.random.default_rng(144+fault)
    w=np.stack([rng.normal(0,.25,(14,WIDTH)),np.zeros((14,WIDTH))])
    b=np.stack([rng.uniform(-.7,.7,WIDTH),np.zeros(WIDTH)])
    v=np.stack([rng.normal(0,.12,(WIDTH,4)),np.zeros((WIDTH,4)),np.zeros((WIDTH,4))])
    params=(w,b,v)
    ms=[np.zeros_like(p) for p in params]
    vs=[p.copy() for p in ms]
    for i in range(1,1801):
        k=rng.integers(5)
        t=KNOBS[k]*2-1
        start=rng.integers(len(x)-2500)
        a=x[start:start+2500]
        bias=b[0]+t*b[1]
        raw=np.tanh(a@(w[0]+t*w[1])+bias)
        off=np.tanh(bias)
        h=raw-off
        readout=v[0]+v[1]*t+v[2]*t*t
        z=h@readout
        estimate=z[:,0].copy()
        for j,p in enumerate(POST):estimate+=filt(z[:,j+1],p)
        error=(estimate-targets[k,start:start+2500])
        error[:430]=0
        error*=2/2070
        dz=np.column_stack([error,*[filt(error[::-1],p)[::-1] for p in POST]])
        dh=dz@readout.T
        da=dh*(1-raw*raw)
        dw=a.T@da
        db=da.sum(axis=0)-dh.sum(axis=0)*(1-off*off)
        dv=h.T@dz
        grads=(np.stack([dw,t*dw]),np.stack([db,t*db]),np.stack([dv,t*dv,t*t*dv]))
        for j,(param,g) in enumerate(zip(params,grads)):
            ms[j]=.9*ms[j]+.1*g
            vs[j]=.999*vs[j]+.001*g*g
            lr=.007 if i<1200 else .002
            param-=lr*(ms[j]/(1-.9**i))/(np.sqrt(vs[j]/(1-.999**i))+1e-8)
    return w.transpose(0,2,1)/scale,b,v*norm

x=features(read("train"))
w,b,v=map(np.array,zip(*(fit(f,x) for f in range(2))))
coeff=np.array([np.r_[tone_coeff(t)[0],tone_coeff(t)[1][1:]]
                for t in np.linspace(0,1,65)])
print("#pragma once\nnamespace muff_control_detail {")
for name,values in dict(inPole=POLES,outPole=POST,weight=w,bias=b,readout=v,tone=coeff, activation=np.tanh(np.linspace(-8,8,1025)),
                        ).items():
    dims="".join(f"[{i}]" for i in values.shape)
    print(f"inline constexpr float {name}{dims}={fmt(values)};")
print("}")
