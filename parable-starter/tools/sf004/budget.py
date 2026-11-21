#!/usr/bin/env python3
"""Reproducible SF004 arithmetic from the pinned native workload and SPICE DC."""
import csv,json,math
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[2]
def calculate():
    fs=48000
    loops=514192/4096/8
    groups=loops*fs
    rates=[]
    for label,multiplier in [('current_only',1),('forward_jacobian',3),('central_jacobian',5)]:
        nominal=groups*multiplier;worst=fs*4*25*multiplier
        rates.append(dict(method=label,states_per_coupled_evaluation=multiplier,pair_states_per_second_nominal=nominal,
            pair_states_per_second_static_bound=worst,maximum_mean_latency_nominal_ns=1e9/nominal,
            maximum_mean_latency_static_bound_ns=1e9/worst,
            payload_4dac_4current_4voltage_16bit_mbps=nominal*12*16/1e6))
    biases=[]
    with (PROJECT/'tests/fixtures/germanium/bias.csv').open() as f:
        for row in csv.DictReader(f):
            b1,c1,e2,c2,tap=[float(row[x]) for x in ('base1_volts','collector1_volts','emitter2_volts','collector2_volts','tap_volts')]
            feedback=float(row['feedback_ohms'])
            ib1=(b1-e2)/feedback
            ie2=-e2/1000+(b1-e2)/feedback
            ic2=(c2-tap)/8200
            ib2=ie2-ic2
            ic1=(c1+9)/33000-ib2
            biases.append(dict(case=row['case'],node_volts=[b1,c1,e2,c2],ve_vc_q1=[-b1,c1-b1],ve_vc_q2=[e2-c1,c2-c1],
                               positive_out_of_device_amps=dict(q1_base=ib1,q1_collector=ic1,q2_base=ib2,q2_collector=ic2)))
    codecs=[]
    for fs,mode,adc,dac in [(48000,'linear',17,17.3),(48000,'low_latency',6.6,7.19),(192000,'linear',11.6,10.7),(192000,'low_latency',6.8,5.41)]:
        codecs.append(dict(fs=fs,filter_mode=mode,adc_group_delay_frames=adc,dac_group_delay_frames=dac,
                           filter_only_round_trip_us=(adc+dac)/fs*1e6))
    delta=100e-6;ilsb=.002/65536
    companion=[]
    for fuzz in (0,.8,1):
        k=2*192000;gc=k*20e-6;top=1000*(1-fuzz);low=1000*fuzz
        ge=1/top if low==0 else (1/low+gc)/(1+top*(1/low+gc))
        history=0 if low==0 else -gc/(1+top*(1/low+gc))
        e=.161834159
        companion.append(dict(fuzz=fuzz,emitter_companion_siemens=ge,equivalent_resistance_ohms=1/ge,
                              shunt_current_at_bias_amps=ge*e,opposing_history_injection_at_bias_amps=-history*fuzz*e))
    return dict(schema=1,main='440a99221af7cc3f3b7123db327bb161306d4856',
      measured_hardware=False,workload=dict(expm1_per_base=514192/4096,coupled_evaluations_per_base=loops,
      coupled_evaluations_per_second=groups,individual_device_evaluations_per_second=groups*2,
      expm1_per_second=groups*8,base_period_us=1e6/48000,internal_period_us=1e6/192000),
      direct_rates=rates,codec_filter_delays=codecs,dc_bias_and_currents=biases,companion=companion,
      precision=dict(illustrative_current_span_amps=.002,ideal_16bit_lsb_amps=ilsb,delta_volts=delta,
      central_jacobian_quantization_bound_siemens=ilsb/(2*delta),
      ideal_bits_for_100pa_residual=math.ceil(math.log2(.002/1e-10)),
      ideal_bits_for_1pa_math_floor=math.ceil(math.log2(.002/1e-12)),
      shot_noise_270ua_1mhz_amps=math.sqrt(2*1.602176634e-19*270e-6*1e6)),
      caveats=['workload counts are host observations, not measured Daisy CPU deadlines',
      'group-delay sums exclude buffering, analog stages and sample/hold; modes not verified on a board',
      'ADC bit depths are ideal quantization, not ENOB or current-sensor specifications',
      'physical current/voltage ranges here cover selected states, never authorize probing the full GP guard domain'])
if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path);a=p.parse_args()
    result=json.dumps(calculate(),indent=2)+'\n'
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(result)
    else:print(result,end='')
