"""Staged startup diagnostics: simulated sessions, no physical success evidence."""
import ast,copy,json,struct,subprocess,sys,tempfile,unittest,zipfile,zlib
from pathlib import Path
from unittest import mock
HERE=Path(__file__).resolve().parents[1]/'tools/resources';sys.path.insert(0,str(HERE))
import ram_startup_test as diag
import seed3_runner as common
META=HERE.parents[1]/'docs/resources/ram-startup/manifest.json'
BUNDLE=json.loads(META.read_text())
DEVICE=dict(path='1-2',serial='SYNTHETIC',layout='@Internal Flash /0x08000000/16*128Kg')
SHA=common.sha(META.read_bytes())
def progress(raw,n):
 raw=bytearray(raw);struct.pack_into('<8I',raw,64+(n-1)*32,0x53544745,n,~n&0xffffffff,83,0x2001ffc0,0,1,0x434d4954);return bytes(raw)
def native(q,mode):
 w=list(struct.unpack('<32I',q));w[0]=0x50494452;w[16:19]=[1,2,3];w[19]=0x450;w[20]=128;w[24]=diag.BASE;w[25]=3 if mode==2 else 0;w[28]=1
 raw=struct.pack('<32I',*w);w[31]=zlib.crc32(raw[4:124])&0xffffffff;return struct.pack('<32I',*w)
class Fake:
 def __init__(self,mode=0):
  self.usb=mock.Mock();self.usb.wait_dfu.return_value=DEVICE;self.memory={};self.calls=[];self.leave_error=False;self.no_entry=False;self.bad_read=False;self.mode=mode
 def operation(self,d,address,data=None,count=None,leave=False):
  self.calls.append(dict(address=address,data=data,count=count,leave=leave))
  if data is not None:self.memory[address]=bytes(data)
  if leave:
   if not self.no_entry:
    for n in range(1,3 if self.mode==0 else 18):self.memory[diag.TRACE]=progress(self.memory[diag.TRACE],n)
    if self.mode:self.memory[diag.MAIL]=native(self.memory[diag.MAIL],self.mode)
   if self.leave_error:raise common.Stop('Failed to get status')
  elif count is not None:return b'wrong' if self.bad_read else self.memory[address]
class Startup(unittest.TestCase):
 def images(self):
  return {i['name']:(META.parent/i['bin_file']).read_bytes() for i in BUNDLE['images']}
 def test_manifest_corrects_documented_stack_boundary(self):
  m,blobs=diag.load_bundle(META,SHA)
  for image in m['images']:
   blob=blobs[image['name']];sp,pc=struct.unpack('<II',blob[:8]);self.assertLess(sp,0x2001ffff-16);self.assertEqual(sp,0x2001ffc0);self.assertTrue(pc&1)
   for name,digest in image['configuration']['sources'].items():
    path=HERE/'ram_startup'/name if name!='protocol.h' else HERE/'ram_identity/protocol.h'
    self.assertEqual(common.sha(path.read_bytes()),digest)
  with self.assertRaises(common.Stop):diag.load_bundle(META,'0'*64)
 def test_versioned_kit_hashes_current_host_and_actual_arm_evidence(self):
  receipt=json.loads((META.parent/'kit.json').read_text());archive=META.parent/receipt['file']
  self.assertEqual(common.sha(archive.read_bytes()),receipt['sha256'])
  self.assertEqual(receipt['manifest_sha256'],SHA);self.assertFalse(receipt['hardware_tested']);self.assertFalse(receipt['flash_programming_permitted'])
  with zipfile.ZipFile(archive) as z:
   self.assertIsNone(z.testzip());self.assertEqual(z.read('manifest.json'),META.read_bytes())
   for line in z.read('SHA256SUMS').decode().splitlines():
    digest,name=line.split('  ',1);self.assertEqual(common.sha(z.read(name)),digest,name)
   for name in ['ram_startup_test.py','ram_identity_test.py','seed3_runner.py','hardware.py']:
    self.assertEqual(z.read(name),(HERE/name).read_bytes())
   self.assertEqual(z.read('README.md'),(META.parent.parent/'RAM_STARTUP.md').read_bytes())
   for image in BUNDLE['images']:
    prefix=image['name']+'/'
    self.assertEqual(common.sha(z.read(prefix+'diagnostic.elf')),image['elf_sha256'])
    self.assertEqual(common.sha(z.read(image['bin_file'])),image['bin_sha256'])
    entry=z.read(prefix+'entry-audit.txt').decode();fault=z.read(prefix+'fault-audit.txt').decode()
    self.assertLess(entry.index('cpsid'),entry.index('str'))
    if image['mode']==0:
     self.assertNotIn('msr',entry);self.assertNotIn('push',entry);self.assertNotIn('pop',entry)
    self.assertNotIn('push',fault);self.assertNotIn('pop',fault)
   host=json.loads(z.read('host-manifest.json'))
   self.assertEqual(host['host_sha256'],receipt['host_sha256'])
   for name,digest in host['dependencies'].items():self.assertEqual(common.sha(z.read(name)),digest)
 def test_no_entry_never_claims_execution_or_identity(self):
  image=BUNDLE['images'][0];nonce=b'a'*16;raw=diag.trace_request(image,nonce);result=diag.decode_trace(raw,image,nonce)
  self.assertFalse(result['entry_evidence']);self.assertIsNone(result['last_committed_stage']);self.assertFalse(result['identity_verified'])
 def test_trace_prefixes_partial_slots_and_gaps(self):
  image=BUNDLE['images'][1];nonce=b'a'*16;raw=diag.trace_request(image,nonce)
  for n in range(1,22):
   raw=progress(raw,n);self.assertEqual(diag.decode_trace(raw,image,nonce)['last_committed_stage'],n)
  bad=bytearray(raw);bad[64+32+28]^=1;r=diag.decode_trace(bad,image,nonce);self.assertFalse(r['trace_contiguous']);self.assertIsNone(r['last_committed_stage']);self.assertIn(2,r['partial_slots'])
  with self.assertRaises(common.Stop):diag.decode_trace(raw,image,b'b'*16)
 def test_lost_getstatus_is_unknown_then_collect_is_read_only(self):
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);f=Fake();f.leave_error=True
   with self.assertRaises(common.Stop):diag.launch(f,BUNDLE,self.images(),'entry',p,SHA)
   s=json.loads((p/'session.json').read_text());self.assertTrue(s['leave_attempted']);self.assertEqual(s['execution'],'UNKNOWN')
   f.calls=[];r=diag.collect(f,BUNDLE,s,p,SHA)
   self.assertEqual(r['last_committed_stage'],2);self.assertTrue(r['entry_evidence']);self.assertFalse(r['automatic_rom_return_verified']);self.assertFalse(r['identity_verified'])
   self.assertTrue(all(c['count'] is not None and c['data'] is None and not c['leave'] for c in f.calls))
 def test_readback_failure_never_leaves(self):
  with tempfile.TemporaryDirectory() as t:
   f=Fake();f.bad_read=True
   with self.assertRaises(common.Stop):diag.launch(f,BUNDLE,self.images(),'entry',Path(t),SHA)
   self.assertFalse(any(c['leave'] for c in f.calls))
 def test_progression_requires_collected_previous_stage(self):
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);f=Fake(mode=1)
   with self.assertRaises(common.Stop):diag.launch(f,BUNDLE,self.images(),'startup',p,SHA)
   self.assertEqual(f.calls,[])
   prev=dict(manifest_sha256=SHA,device=DEVICE,probe='entry',entry_evidence=True,fault_snapshot=None,partial_slots=[],last_committed_stage=2)
   diag.launch(f,BUNDLE,self.images(),'startup',p,SHA,previous=prev)
   report=diag.collect(f,BUNDLE,json.loads((p/'session.json').read_text()),p,SHA)
   self.assertTrue(report['native_record_valid']);self.assertFalse(report['identity_verified']);self.assertEqual(report['last_committed_stage'],17)
   with self.assertRaises(common.Stop):diag.launch(f,BUNDLE,self.images(),'reset',p,SHA,previous=report,allow_pg3=False)
 def test_fault_capture_and_safe_candidate_frame(self):
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);f=Fake(mode=1);prev=dict(manifest_sha256=SHA,device=DEVICE,probe='entry',entry_evidence=True,fault_snapshot=None,partial_slots=[],last_committed_stage=2)
   diag.launch(f,BUNDLE,self.images(),'startup',p,SHA,previous=prev)
   raw=bytearray(f.memory[diag.TRACE]);words=[0]*32;words[0]=0x4641554c;words[1]=3;words[2]=0xfffffff9;words[3]=diag.STACK+0x300;words[8]=(1<<15)|(1<<9);words[12]=0x5c001000;words[31]=0x46434d54;struct.pack_into('<32I',raw,896,*words);f.memory[diag.TRACE]=bytes(raw)
   stack=bytearray(1024);struct.pack_into('<8I',stack,0x300,0,0,0,0,0,0,diag.BASE+1000,1<<24);f.memory[diag.STACK]=bytes(stack)
   r=diag.collect(f,BUNDLE,json.loads((p/'session.json').read_text()),p,SHA)
   self.assertTrue(r['fault_snapshot']['bfar_valid']);self.assertFalse(r['fault_snapshot']['mmfar_valid']);self.assertEqual(r['candidate_core_frame']['pc'],diag.BASE+1000)
   words[8]|=1<<12;struct.pack_into('<32I',raw,896,*words);f.memory[diag.TRACE]=bytes(raw);f.calls=[]
   r=diag.collect(f,BUNDLE,json.loads((p/'session.json').read_text()),p,SHA);self.assertNotIn('candidate_core_frame',r);self.assertFalse(any(c['address']==diag.STACK for c in f.calls))
 def test_transport_restricts_all_writes_to_approved_ram(self):
  with tempfile.TemporaryDirectory() as t:
   f=diag.Transport.__new__(diag.Transport);f.output=Path(t);f.number=0;f.usb=mock.Mock();f.usb.wait_dfu.return_value=DEVICE;f.usb.selectors.return_value=['dfu-util'];f.usb.command.return_value='OK'
   for address in [0x08000000,0x52002000,0x1ff1e800,diag.STACK,diag.BASE+1]:
    with self.assertRaises(common.Stop):f.operation(DEVICE,address,data=b'x')
   f.usb.command.assert_not_called()
   f.operation(DEVICE,diag.BASE,data=b'x');a=f.usb.command.call_args[0][0];self.assertIn('0x24040000:force',a)
   f.operation(DEVICE,diag.BASE,leave=True);a=f.usb.command.call_args[0][0];self.assertNotIn('-D',a);self.assertIn('0x24040000:leave',a)
   f.operation(DEVICE,diag.BASE,data=b'x',leave=True);a=f.usb.command.call_args[0][0];self.assertIn('-D',a);self.assertIn('0x24040000:force:leave',a)
 def test_download_leave_is_explicit_and_never_automatic_retry(self):
  with tempfile.TemporaryDirectory() as t:
   f=Fake();diag.launch(f,BUNDLE,self.images(),'entry',Path(t),SHA,handoff='download')
   self.assertEqual(sum(c['leave'] for c in f.calls),1);self.assertIsNotNone(f.calls[-1]['data'])
 def test_python36_and_manual_collection_keeps_admission_false(self):
  ast.parse((HERE/'ram_startup_test.py').read_text(),feature_version=(3,6))
if __name__=='__main__':unittest.main()
