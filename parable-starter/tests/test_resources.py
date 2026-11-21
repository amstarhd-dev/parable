"""Focused resource infrastructure tests; synthetic captures are NOT hardware evidence."""
import copy,hashlib,importlib,json,subprocess,sys,tempfile,unittest
from pathlib import Path
HERE=Path(__file__).resolve().parents[1]/'tools/resources';sys.path.insert(0,str(HERE))
import hardware
import run as bench
CATALOG=json.loads((HERE/'catalog.json').read_text())
class Resources(unittest.TestCase):
 def test_catalog_pins_and_unique_profiles(self):
  bench.validate(CATALOG);self.assertEqual(len(CATALOG['models']),35);self.assertEqual(len(CATALOG['chains']),12)
  self.assertTrue(all(len(m['commit'])==40 for m in CATALOG['models']))
  self.assertEqual(next(m for m in CATALOG['models'] if m['id']=='rat_fir97')['latency_fir_frames'],24)
  bad=copy.deepcopy(CATALOG);bad['models'][0]['source_files']=['../../escape.cpp']
  with self.assertRaises(ValueError):bench.validate(bad)
  bad=copy.deepcopy(CATALOG);bad['models'][0]['commit']='main'
  with self.assertRaises(ValueError):bench.validate(bad)
 def fixture(self):
  c=dict(artifact_id='a'*64,profile='rat_fir97',execution='foreground_kernel',stimulus='synthetic_mix',mode=2,route=0,core_hz_rcc=480000000,sample_rate=48000,block_frames=48,deadline_cycles=480000,counter_enabled=1,blocks=3000,valid_blocks=3000,max_cycles=235475,deadline_hits=0,invalid_intervals=0,frame_mismatches=0,source_set_sha256='b'*64,reserved_sdram_bytes=0,used_sdram_bytes=0,finite_output_failures=0,fpscr=0,fpdscr=0,icache_enabled=1,dcache_enabled=1)
  artifact={k:c[k] for k in ['artifact_id','profile','execution','stimulus','mode','route','source_set_sha256','reserved_sdram_bytes']};artifact.update(status='linked',capture_blocks=3000,bin_sha256='c'*64)
  raw='PARABLE_RESOURCE_V1 '+' '.join(f'{k}={v}' for k,v in c.items())+'\nEND_RESOURCE_CAPTURE\n';return raw,dict(artifacts=[artifact])
 def test_hardware_capture_never_admits(self):
  raw,manifest=self.fixture();result=hardware.check(raw,manifest);self.assertEqual(result['screen'],'FOREGROUND_COST_ONLY');self.assertFalse(result['production_admitted']);self.assertEqual(hardware.check(raw+raw,manifest)['capture'],result['capture'])
  self.assertEqual(hardware.check(raw.replace('max_cycles=235475','max_cycles=370915'),manifest)['screen'],'FAIL')
 def test_reject_wrong_variant_source_and_partial_reports(self):
  raw,manifest=self.fixture()
  for changed in [raw.replace('profile=rat_fir97','profile=rat'),raw.replace('source_set_sha256='+'b'*64,'source_set_sha256='+'d'*64),raw.replace('block_frames=48','block_frames=64'),raw.replace('blocks=3000','blocks=3001',1),raw.replace('END_RESOURCE_CAPTURE',''),raw+'PARABLE_RESOURCE_V1 partial\n',raw.replace('max_cycles=235475','max_cycles=235475 max_cycles=1'),raw.replace('max_cycles=235475','max_cycles=2147483648')]:
   with self.subTest(changed=changed[-120:]):
    with self.assertRaises(ValueError):hardware.check(changed,manifest)
 def test_two_different_usb_captures_reject(self):
  raw,manifest=self.fixture()
  with self.assertRaises(ValueError):hardware.check(raw+raw.replace('max_cycles=235475','max_cycles=235474'),manifest)
 def test_source_closure_independent_of_model_order(self):
  with tempfile.TemporaryDirectory() as t:
   a=bench.Sources(Path(t));models={m['id']:m for m in CATALOG['models']};a.extract(models['rat']);first=dict(a.current_hashes);a.extract(models['muff']);a.extract(models['rat']);self.assertEqual(first,a.current_hashes)
   self.assertEqual(hashlib.sha256((Path(t)/'sources'/models['rat']['commit']/'src/parable_rat.cpp').read_bytes()).hexdigest(),first[models['rat']['commit']+':src/parable_rat.cpp'])
 def test_allocator_and_callback_guards(self):
  with tempfile.TemporaryDirectory() as t:
   binary=Path(t)/'guards';cmd=['c++','-O1','-std=c++17',str(HERE/'tracker_test.cpp'),str(HERE/'host_tracker.cpp'),'-pthread','-Wl,'+','.join('--wrap='+w for w in bench.WRAPS),'-o',str(binary)]
   subprocess.run(cmd,check=True,capture_output=True);subprocess.run([str(binary)],check=True,capture_output=True)
 def test_actual_rat_and_sanitized_parallel_route(self):
  with tempfile.TemporaryDirectory() as t:
   out=Path(t);sources=bench.Sources(out);models={m['id']:m for m in CATALOG['models']}
   for name in ['rat','rat_1x_cost_only']:
    rat,_=bench.build(models[name],[models[name]],sources,out,'c++')
    result=json.loads(subprocess.check_output([str(rat),'invalid','1','480','2','0'],text=True));self.assertTrue(result['prepared'] and result['deterministic']);self.assertEqual(result['nonfinite_outputs'],0)
   profile=next(c for c in CATALOG['chains'] if c['id']=='chain_scorpio_gemini_rooms_1');binary,meta=bench.build(profile,[models[m] for m in profile['members']],sources,out,'c++')
   for signal in ['guitar','invalid','extreme']:
    c=json.loads(subprocess.check_output([str(binary),signal,'1','480','2','1'],text=True));self.assertTrue(c['prepared'] and c['reprepare_ok'] and c['deterministic']);self.assertEqual(c['nonfinite_outputs'],0);self.assertEqual(c['forbidden_callback_operations'],0);self.assertEqual(c['allocation_tracker_overflows'],0)
   # Same actual chain under ASan/UBSan. No host performance claim from this build.
   command=list(meta['command']);command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie'];command[-1]=str(out/'sanitized');command+=['-O1','-fsanitize=address,undefined','-no-pie'];subprocess.run(command,check=True,capture_output=True)
   subprocess.run([str(out/'sanitized'),'invalid','1','480','2','1'],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
