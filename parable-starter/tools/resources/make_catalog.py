#!/usr/bin/env python3
"""Maintenance helper: explicit audited source selection, never discovers DSP by guessing."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[2];prs={p['number']:p for p in json.loads((root/'docs/resource-pr-snapshot.json').read_text())};main='767a3c402b972fc20df8aea0729adf794f4b7a38'
models=[]
def add(id,adapter,category,kernel,pr=None,**kw):
 models.append(dict(id=id,adapter=adapter,category=category,commit=prs[pr]['sha'] if pr else main,pr=pr,main_integrated=id=='rat',source_files=['include/'+kernel+'.h','src/'+kernel+'.cpp'],defines=['BENCH_USE_'+kw.pop('use',id.upper())],**kw))
add('rat','Rat','drive','parable_rat',use='RAT',family='prepared-IIR/table/4x-FIR',latency_fir_frames=48)
add('cmos','Cmos','drive','parable_cmos',family='4x-nonlinear-Newton')
add('germanium','Germanium','drive','parable_germanium',family='4x-Gummel-Poon-Newton')
add('muff','Muff','drive','parable_muff',family='4x-10port-Newton')
add('ts808','Ts808','drive','parable_ts808',family='16x-7unknown-Newton')
add('klon','Klon','drive','parable_klon',family='12x-5unknown-Newton')
add('wavefolder','Wavefolder','drive','parable_wavefolder',family='4x-prepared-Hermite-ADAA')
add('scorpio','Scorpio','drive','parable_scorpio',family='4x-prepared-Hermite-ADAA/magnetics')
add('libra','Libra','drive','parable_libra',70,family='4x-piecewise-affine-feedback')
add('ds1','Ds1','drive','parable_capricorn',69,family='8x-prepared-curve-ADAA')
add('fz1','Fz1','drive','parable_fz1',61,family='8x-6port-Newton')
add('blender','Blender','drive','parable_blender',63,family='8x-14port-Newton',hold='Existing high-note scientific gates fail')
add('tape_saturation','TapeSat','drive','parable_tape_saturation',56,use='TAPE_SAT',family='4x-magnetic-parcels')
add('tape_medium','Tape','delay','parable_tape',use='TAPE',family='streaming-magnetic-medium')
add('rotary','Rotary','modulation','parable_rotary',family='mechanics/interpolated-delays')
add('spring','Spring','reverb','parable_spring',48,family='192-modal-mechanical/2substeps')
add('taurus_plate','Plate','reverb','parable_plate',72,use='PLATE',family='Dattorro-delay-network')
add('gemini_rooms','Rooms','reverb','parable_coupled_rooms',74,use='ROOMS',family='two-coupled-delay-networks')
add('cancer_cloud','Cloud','reverb','parable_chamber',76,use='CLOUD',family='complete-CloudSeed',extra_sources=['third_party/chamber/cloudseed/DSP/Biquad.cpp','third_party/chamber/cloudseed/DSP/RandomBuffer.cpp'],include_dirs=['third_party/chamber/cloudseed'],forced_includes=['tools/chamber_cloud_portability.h'],block_bridge_frames=64,hold='67011904-byte core near64MiB limit; reprepare peak exceedsSDRAM')
add('leo_hall','Hall','reverb','parable_hall',77,use='HALL',family='borrowed-double-grand-hall',block_bridge_frames=64,extra_sources=['src/hall_core.cpp'])
add('virgo_tiled','Tiled','reverb','parable_tiled_room',80,use='TILED',family='compact-Dattorro-delay-network')
add('generic_drive_prototype','Drive','prototype','parable_drive',11,use='DRIVE',family='4x-prepared-diode-table',hold='Unmerged PAR006 prototype; superseded as general drive direction')
add('gemini_cmos_candidate','Cmos','drive','parable_cmos',42,use='CMOS',family='same-CMOS-core/new-bias-leak',variant_of='cmos');models[-1]['defines'].append('BENCH_GEMINI')
for id,defs,lat in [('rat_pr81_reference',[],48),('rat_fir97',['PARABLE_RAT_BENCH_FIR97'],24),('rat_fir97_fp32',['PARABLE_RAT_BENCH_FIR97','PARABLE_RAT_BENCH_FLOATCLIP'],24),('rat_1x_cost_only',['PARABLE_RAT_BENCH_1X'],0)]:
 add(id,'Rat','drive','parable_rat',81,use='RAT',family='PR81-isolated-experiment',variant_of='rat',latency_fir_frames=lat);models[-1]['defines']+=defs
for i,name in enumerate(['libm','hermite32','hermite64','polynomial8','polynomial12']):
 add('germanium_'+name,'Germanium','drive','parable_germanium',21,use='GERMANIUM',family='PR21-unselected-evaluator',variant_of='germanium');models[-1]['defines'].append('PARABLE_GERMANIUM_EVALUATOR='+str(i))
for m in models:
 m['mode_labels']={'0':'nominal healthy','1':'documented primary fault or parameter corner','2':'healthy high-control corner'}
 if m['adapter']=='Rat':m['mode_labels'].update({'1':'diode_open','3':'ruetz','4':'compensation'})
 if m['adapter']=='Germanium':m['mode_labels'].update({'1':'q1_leaky','3':'feedback_wrong'})
 if m['id']=='cmos':m['mode_labels'].update({'1':'coupling_small','3':'feedback_low'})
 if m['id']=='rat_1x_cost_only':m['hold']='CPU-only lower bound; anti-aliasing removed, never a fidelity candidate'
 if m['adapter']=='Rat':m['modes']=[0,1,2,3,4]
 elif m['adapter']=='Germanium' or m['id']=='cmos':m['modes']=[0,1,2,3]
 if m['adapter']=='Cloud':m['defines']+=['BUFFER_SIZE=64','MAX_STR_SIZE=32']
for id,kind in [('bypass',0),('gain',1),('lowpass',2)]:
 models.append(dict(id=id,adapter='Utility',category='utility',commit=main,pr=None,main_integrated=True,source_files=['include/parable_core.h','src/parable_rat.cpp'],defines=['BENCH_USE_UTILITIES','BENCH_UTILITY_KIND='+str(kind)],family='actual-native-Engine'))
chains=[]
for drive,verb in [('scorpio','gemini_rooms'),('wavefolder','virgo_tiled'),('rat','leo_hall'),('muff','virgo_tiled')]:
 for route in range(3):chains.append(dict(id=f'chain_{drive}_{verb}_{route}',members=[drive,verb,'tape_medium','rotary'],route=route,routing=['serial-drive-mod-delay-verb','parallel-preserved-clean','bounded-feedback-around-serial'][route]))
manifest=dict(schema='parable.resource-catalog.v1',baseline=main,target=dict(core_hz=480000000,sample_rate=48000,block_frames=48,deadline_cycles=480000,provisional_whole_graph_limit=.70),models=models,chains=chains,exclusions=[dict(pr=78,reason='Wallace host routing experiment; uses existing kernels; source routing must still be inventoried'),dict(pr=79,reason='Offline cabinet-IR research; not a bounded realtime native cabinet processor'),dict(local='SF026',reason='Unpublished incomplete Level-Loc workspace; new assignment supersedes its active work, preserved separately')])
(root/'tools/resources/catalog.json').write_text(json.dumps(manifest,indent=2)+'\n');print(len(models),'source identities;',len(chains),'graph configurations')
