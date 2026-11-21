"""RAM bootstrap simulations and actual portable wire code; no hardware claims."""
import ast
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
from unittest import mock
import zlib
HERE=Path(__file__).resolve().parents[1]/'tools/resources'
sys.path.insert(0,str(HERE))
import ram_identity_test as ram
import seed3_runner as common
META=HERE.parents[1]/'docs/resources/ram-identity/manifest.json'
MANIFEST=json.loads(META.read_text())
BLOB=META.with_name('ram-identity.bin').read_bytes()
DEVICE=dict(path='1-2',serial='SYNTHETIC',layout='@Internal Flash /0x08000000/16*128Kg')
def answer(challenge,uid=(1,2,3)):
 w=list(struct.unpack('<32I',challenge));w[0]=ram.RESPONSE;w[16:19]=uid;w[19]=0x10030450;w[20]=128;w[24]=ram.BASE;w[25]=3;w[28]=1
 raw=struct.pack('<32I',*w);w[31]=zlib.crc32(raw[4:124])&0xffffffff
 return struct.pack('<32I',*w)
class Fake:
 def __init__(self):
  self.usb=mock.Mock();self.usb.wait_dfu.return_value=DEVICE
  self.memory={};self.events=[];self.change_uid=False;self.bad_readback=False;self.fail_return=False;self.round=0
 def command(self,device,address,length=None,download=None,leave=False):
  assert device==DEVICE and address in (ram.BASE,ram.MAIL)
  self.events.append((address,length,download is not None,leave))
  if download is not None:self.memory[address]=Path(download).read_bytes()
  elif leave:
   self.round+=1;self.memory[ram.MAIL]=answer(self.memory[ram.MAIL],(1,2,4) if self.change_uid and self.round==2 else (1,2,3))
  else:return b'bad' if self.bad_readback else self.memory[address]
 def enumeration(self,device):return str(self.round)
 def returned(self,device,generation):
  if self.fail_return:raise common.Stop('No ROM return')
class Ram(unittest.TestCase):
 def test_packaged_executable_hashes_vectors_and_real_elf_segments(self):
  folder=META.parent;receipt=json.loads((folder/'kit.json').read_text());archive=folder/receipt['file']
  self.assertEqual(common.sha(archive.read_bytes()),receipt['sha256']);self.assertFalse(receipt['hardware_tested']);self.assertFalse(receipt['flash_programming_permitted'])
  with zipfile.ZipFile(archive) as z:
   for line in z.read('SHA256SUMS').decode().splitlines():
    digest,name=line.split('  ',1);self.assertEqual(common.sha(z.read(name)),digest,name)
   self.assertEqual(z.read('ram_identity_test.py'),(HERE/'ram_identity_test.py').read_bytes())
   self.assertEqual(z.read('seed3_runner.py'),(HERE/'seed3_runner.py').read_bytes())
   elf=z.read('ram-identity.elf');self.assertEqual(common.sha(elf),MANIFEST['elf_sha256'])
   self.assertEqual(z.read('ram-identity.bin'),BLOB)
   self.assertEqual({n for n in z.namelist() if n.endswith('.bin')},{'ram-identity.bin'})
   self.assertEqual(struct.unpack_from('<H',elf,18)[0],40) # ELF32 ARM
   offset=struct.unpack_from('<I',elf,28)[0];stride,count=struct.unpack_from('<HH',elf,42)
   for i in range(count):
    segment=struct.unpack_from('<8I',elf,offset+i*stride)
    if segment[0]==1:
     _,fileoff,addr,physical,filesize,memsize,flags,alignment=segment
     self.assertEqual(addr,physical)
     self.assertTrue(any(lo<=addr and addr+memsize<=hi for lo,hi in [(ram.BASE,ram.MAIL+128),(0x2001fc00,0x20020000)]))
     if filesize:self.assertEqual(elf[fileoff:fileoff+filesize],BLOB)
   vectors=struct.unpack('<166I',BLOB[:664]);self.assertEqual(vectors[0],0x20020000)
   for index,value in enumerate(vectors[1:],1):
    if index in (7,8,9,10,13):self.assertEqual(value,0)
    else:self.assertTrue(value&1 and ram.BASE+664<=value<ram.BASE+len(BLOB))
 def test_rom_return_requires_new_enumeration_and_same_device(self):
  f=ram.RamDfu.__new__(ram.RamDfu);f.usb=mock.Mock()
  def listing(number=3,serial='SYNTHETIC'):
   return 'Found DFU: [0483:df11] devnum=%s, cfg=1, intf=0, path="1-2", alt=0, name="%s", serial="%s"\n'%(number,DEVICE['layout'],serial)
  f.usb.command.return_value=listing();f.returned(DEVICE,'2')
  f.usb.command.return_value=listing(serial='OTHER')
  with self.assertRaises(common.Stop):f.returned(DEVICE,'2')
  f.usb.command.return_value=listing(number=2)
  with mock.patch.object(ram.time,'monotonic',side_effect=[0,0,26]),mock.patch.object(ram.time,'sleep'):
   with self.assertRaises(common.Stop):f.returned(DEVICE,'2')
  f.usb.command.side_effect=['',listing(number=2)]
  with mock.patch.object(ram.time,'sleep'):f.returned(DEVICE,'2')
 def test_tampered_layout_size_and_vectors_rejected(self):
  for mode in ['layout','size','vector']:
   with tempfile.TemporaryDirectory() as t:
    p=Path(t);m=copy.deepcopy(MANIFEST);blob=bytearray(BLOB)
    if mode=='layout':m['code_address']=0x08000000
    if mode=='size':blob+=bytes(4097-len(blob))
    if mode=='vector':blob[4:8]=struct.pack('<I',0x08000001)
    m['bin_bytes']=len(blob);m['bin_sha256']=common.sha(blob)
    (p/'ram-identity.bin').write_bytes(blob);raw=common.json_bytes(m);(p/'manifest.json').write_bytes(raw)
    with self.assertRaises(common.Stop):ram.validate_kit(p/'manifest.json',common.sha(raw))
 def test_actual_manifest_vectors_and_source_bindings(self):
  m,blob=ram.validate_kit(META,common.sha(META.read_bytes()));self.assertEqual(blob,BLOB)
  for name,digest in m['configuration']['sources'].items():self.assertEqual(common.sha((HERE/'ram_identity'/name).read_bytes()),digest)
  self.assertLessEqual(len(blob),4096);self.assertEqual(struct.unpack('<I',blob[:4])[0],0x20020000)
  with self.assertRaises(common.Stop):ram.validate_kit(META,'0'*64)
 def test_protocol_rejects_stale_corrupt_and_invalid_silicon(self):
  nonce=bytes(range(16));q=ram.request(MANIFEST['artifact_id'],nonce);raw=answer(q)
  self.assertEqual(ram.response(raw,MANIFEST,nonce)['identity']['flash_kib'],128)
  for bad in [raw[:-1],q,raw[:72]+bytes([raw[72]^1])+raw[73:]]:
   with self.assertRaises(common.Stop):ram.response(bad,MANIFEST,nonce)
  with self.assertRaises(common.Stop):ram.response(raw,MANIFEST,b'x'*16)
  for index,value in [(19,0x451),(20,2048),(24,0x08000000),(30,1)]:
   w=list(struct.unpack('<32I',raw));w[index]=value
   changed=struct.pack('<32I',*w);w[31]=zlib.crc32(changed[4:124])&0xffffffff
   with self.assertRaises(common.Stop):ram.response(struct.pack('<32I',*w),MANIFEST,nonce)
  with self.assertRaises(common.Stop):ram.request(MANIFEST['artifact_id'],bytes(16))
 def test_two_rounds_no_flash_admission(self):
  with tempfile.TemporaryDirectory() as t:
   f=Fake();result=ram.execute(f,MANIFEST,BLOB,Path(t))
   self.assertEqual(result['rom_returns_observed'],2);self.assertFalse(result['flash_programming_permitted']);self.assertFalse(result['full_runner_integrated'])
   self.assertEqual(sum(e[3] for e in f.events),2)
   self.assertTrue((Path(t)/'identity-observation.json').exists())
   self.assertFalse((Path(t)/'transition-receipt.json').exists())
 def test_readback_return_and_identity_failure_stop(self):
  for mode in ['bad_readback','fail_return','change_uid']:
   with tempfile.TemporaryDirectory() as t:
    f=Fake();setattr(f,mode,True)
    with self.assertRaises(common.Stop):ram.execute(f,MANIFEST,BLOB,Path(t))
    self.assertFalse((Path(t)/'identity-observation.json').exists())
    if mode=='bad_readback':self.assertFalse(any(e[3] for e in f.events))
 def test_transport_refuses_flash_option_bytes_and_bad_ranges(self):
  with tempfile.TemporaryDirectory() as t:
   f=ram.RamDfu.__new__(ram.RamDfu);f.usb=mock.Mock();f.output=Path(t);f.command_number=0
   p=Path(t)/'input.bin';p.write_bytes(bytes(4097))
   for address in [0x08000000,0x1ff1e800,0x52002000,0x24000000,ram.BASE+1]:
    with self.assertRaises(common.Stop):f.command(DEVICE,address,download=p)
   for kwargs in [dict(address=ram.BASE,download=p),dict(address=ram.MAIL,length=129),dict(address=ram.MAIL,leave=True),dict(address=ram.BASE,length=1,download=p),dict(address=ram.BASE,length=-1)]:
    with self.assertRaises(common.Stop):f.command(DEVICE,**kwargs)
   f.usb.command.assert_not_called();f.usb.wait_dfu.assert_not_called()
 def test_real_command_construction_and_no_flash_selectors(self):
  with tempfile.TemporaryDirectory() as t:
   f=ram.RamDfu.__new__(ram.RamDfu);f.output=Path(t);f.command_number=0;f.usb=mock.Mock();f.usb.wait_dfu.return_value=DEVICE;f.usb.selectors.return_value=['dfu-util','-a','0'];f.usb.command.return_value='OK'
   p=Path(t)/'ram.bin';p.write_bytes(BLOB)
   f.command(DEVICE,ram.BASE,download=p);args=f.usb.command.call_args[0][0];self.assertIn('-D',args);self.assertIn('0x24040000:force',args)
   f.command(DEVICE,ram.BASE,leave=True);args=f.usb.command.call_args[0][0];self.assertIn('0x24040000:leave',args);self.assertNotIn('-D',args)
 def test_actual_cpp_request_and_crc_under_sanitizers(self):
  # Exercise the SAME portable functions compiled into the ARM executable.
  source='''#include "protocol.h"
#include <cstdio>
int main(){uint32_t w[32]={};if(std::fread(w,1,128,stdin)!=128)return 2;uint32_t id[8];for(unsigned i=0;i<8;++i)id[i]=0xaaaaaaaa;if(!ram_identity::validRequest(w,id))return 3;w[0]=0x50494452;w[16]=1;w[17]=2;w[18]=3;w[19]=0x10030450;w[20]=128;w[24]=0x24040000;w[25]=3;w[28]=1;w[31]=ram_identity::checksum(w);return std::fwrite(w,1,128,stdout)!=128;}'''
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'wire.cpp').write_text(source)
   subprocess.run(['c++','-O1','-std=c++17','-fsanitize=address,undefined','-fno-pie','-no-pie','-I'+str(HERE/'ram_identity'),str(p/'wire.cpp'),'-o',str(p/'wire')],check=True)
   q=ram.request('a'*64,bytes(range(16)));raw=subprocess.check_output([str(p/'wire')],input=q)
   m=dict(artifact_id='a'*64);self.assertEqual(ram.response(raw,m,bytes(range(16)))['identity']['uid'],'000000010000000200000003')
   for offset in [0,4,8,12,32,64,124]:
    bad=bytearray(q);bad[offset]^=1
    self.assertEqual(subprocess.run([str(p/'wire')],input=bad,stdout=subprocess.PIPE).returncode,3)
 def test_python36_and_no_legacy_identity_calls(self):
  source=(HERE/'ram_identity_test.py').read_text();ast.parse(source,feature_version=(3,6))
  self.assertNotIn('.flash(',source);self.assertNotIn('.identity(',source)
if __name__=='__main__':unittest.main()
