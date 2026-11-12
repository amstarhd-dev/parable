#!/usr/bin/env python3
"""Independent exact locked response and fine RK4 slip/event reference (stdlib).
No native output, lookup table, Euler update or audio renderer is used.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path

PI=math.pi


def locked_step(w,angle,target,j,b,limit,dt):
    # Exact scalar linear ODE, split at the drive-torque clipping boundary.
    remaining=dt
    for _ in range(3):
        demand=.12*(target-w)
        clipped=abs(demand)>limit+1e-12
        torque=math.copysign(limit,demand)
        lam=b/j if clipped else (.12+b)/j
        eq=(torque-.02)/b if clipped else (.12*target-.02)/(.12+b)
        duration=remaining
        if clipped:
            boundary=target-torque/.12
            ratio=(boundary-eq)/(w-eq)
            if 0<ratio<1:duration=min(duration,-math.log(ratio)/lam)
        decay=math.exp(-lam*duration)
        angle+=eq*duration+(w-eq)*(-math.expm1(-lam*duration))/lam
        w=eq+(w-eq)*decay;remaining-=duration
        if remaining<1e-14:return w,angle
    raise RuntimeError('Unexpected exact-response branch count')


def forces(s,target):
    wm,wr,_,_,belt=s
    drive=max(-.14,min(.14,.12*(target-wm)))
    load=.001*wr+.02*max(-1,min(1,wr/.5))
    traction=.22*(1-.96*max(math.cos(belt),0)**4)
    required=.002*(drive-load)/.0022+load
    return drive,load,traction,required


def rk4(s,target,h,locked,direction):
    def derivative(q):
        drive,load,capacity,_=forces(q,target)
        if locked:am=ar=(drive-load)/.0022
        else:
            belt=.7*capacity*direction
            am=(drive-belt)/.0002;ar=(belt-load)/.002
        return am,ar,q[0],q[1],q[0]/4
    a=derivative(s);b=derivative(tuple(x+h*y/2 for x,y in zip(s,a)))
    c=derivative(tuple(x+h*y/2 for x,y in zip(s,b)))
    d=derivative(tuple(x+h*y for x,y in zip(s,c)))
    return tuple(x+h*(a1+2*b1+2*c1+d1)/6 for x,a1,b1,c1,d1 in zip(s,a,b,c,d))


def fault(rate):
    w=(.12*2*PI*.8-.02)/.121;s=(w,w,0.,0.,PI);locked=True;events=[];rows=[]
    releases=captures=0;dt=1/rate
    for k in range(8*rate):
        target=2*PI*(6.5 if rate<=k<5*rate else .8)
        _,_,capacity,required=forces(s,target)
        if locked and abs(required)>capacity:
            locked=False;releases+=1;events.append(('release',k/rate))
        relative=s[0]-s[1]
        direction=math.copysign(1,relative if abs(relative)>1e-14 else required)
        candidate=rk4(s,target,dt,locked,direction)
        if not locked and relative*(candidate[0]-candidate[1])<=0:
            after=candidate[0]-candidate[1]
            fraction=relative/(relative-after) if relative!=after else 0
            cross=rk4(s,target,dt*fraction,False,direction)
            _,_,cap,req=forces(cross,target)
            if abs(req)<=cap:
                common=(.0002*cross[0]+.002*cross[1])/.0022
                cross=(common,common,*cross[2:]);locked=True;captures+=1
                events.append(('capture',(k+fraction)/rate))
                candidate=rk4(cross,target,dt*(1-fraction),True,direction)
        s=candidate
        if (k+1)%(rate//100)==0:rows.append(((k+1)/rate,*s,not locked,releases,captures))
    return rows,events


def generate(output):
    output.mkdir(parents=True,exist_ok=True)
    fine,events=fault(192000);coarse,_=fault(96000)
    refine_velocity=max(abs(a[i]-b[i]) for a,b in zip(fine,coarse) for i in (1,2))
    refine_angle=max(abs(a[i]-b[i]) for a,b in zip(fine,coarse) for i in (3,4,5))
    assert refine_velocity<.01 and refine_angle<.002,(refine_velocity,refine_angle)
    path=output/'mechanics.csv'
    with path.open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n');w.writerow(['rotor','wear','time_s','motor_rad_s','rotor_rad_s','motor_angle_rad','rotor_angle_rad','belt_phase_rad','slipping','releases','captures'])
        for horn in (True,False):
            j,b,cap,slow,fast=(.0022,.001,.14,.8,6.5) if horn else (.016,.002,.22,.6,5)
            omega=(.12*2*PI*slow-.02)/(.12+b);angle=0
            for k in range(800):
                target=2*PI*(fast if 100<=k<500 else slow)
                omega,angle=locked_step(omega,angle,target,j,b,cap,.01)
                row=[(k+1)/100,omega,omega,angle,angle,PI+angle/4]
                w.writerow(['horn' if horn else 'drum',0,*[format(x,'.15e') for x in row],0,0,0])
        for row in fine:w.writerow(['horn',.96,*[format(x,'.15e') for x in row[:6]],int(row[6]),row[7],row[8]])
    with (output/'events.csv').open('w',newline='') as f:
        w=csv.writer(f,lineterminator='\n');w.writerow(['event','seconds']);w.writerows((kind,format(t,'.15e')) for kind,t in events)
    manifest=dict(reference='Exact clipped linear ODE when locked; independent RK4/event-interpolated fault at192k, exact trig/unwrapped angles',
                  coarse_reference_rate=96000,fine_reference_rate=192000,refinement_peak_rad_s=refine_velocity,refinement_peak_rad=refine_angle,
                  physical_calibration=False,events=len(events),generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  files={name:hashlib.sha256((output/name).read_bytes()).hexdigest() for name in ['mechanics.csv','events.csv']})
    (output/'reference-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);generate(p.parse_args().output)
