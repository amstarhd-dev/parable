#!/usr/bin/env python3
"""RAM startup v0.2: launch once, manually recover ROM, collect READ-ONLY evidence.
No timeout workaround, automatic fallback, flash command or admission receipt.
"""
import argparse,json,secrets,struct,subprocess,sys,tempfile,zlib
from pathlib import Path
import seed3_runner as common
import ram_identity_test as identity_wire
BASE,MAIL,TRACE=0x24040000,0x24041000,0x24042000
STACK=0x2001fc00
CAPS={BASE:4096,MAIL:128,TRACE:1024,STACK:1024}
STAGES={1:'entry before any stack/CONTROL change (IRQs masked)',2:'entry sentinel halt / before VTOR',3:'VTOR installed / before MSP',4:'MSP corrected / before CONTROL',5:'CONTROL installed / before C++ prologue',6:'C++ reached / before core reads and IRQ cleanup',7:'before D-cache disable',8:'D-cache disabled / before I-cache disable',9:'I-cache disabled / before USB reset',10:'USB reset complete / before request validation',11:'before UID word0',12:'before UID word1',13:'before UID word2',14:'before DBGMCU IDCODE',15:'before factory capacity',16:'before response fields and CRC',17:'native response committed; startup probe halt',18:'before GPIOG clock enable',19:'before PG3 setup',20:'before charge delay',21:'before SYSRESETREQ'}

def load_bundle(path,expected):
 path=Path(path).resolve();raw=path.read_bytes();common.require(common.sha(raw)==expected,'Diagnostic manifest hash mismatch')
 m=json.loads(raw.decode());common.require(m['schema']=='parable.ram-startup.v2' and not m['flash_programming_permitted'],'Wrong diagnostic manifest')
 common.require((m['code_address'],m['code_limit'],m['mailbox_address'],m['mailbox_bytes'],m['trace_address'],m['trace_bytes'])==(BASE,4096,MAIL,128,TRACE,1024),'Unexpected diagnostic RAM layout')
 common.require(len(m['images'])==3 and {i['name'] for i in m['images']}=={'entry','startup','reset'},'Unexpected diagnostic set')
 blobs={}
 for image in m['images']:
  name=image['name'];common.require(image['bin_file']==name+'/diagnostic.bin','Unsafe image path')
  common.require(image['artifact_id']==common.sha(json.dumps(image['configuration'],sort_keys=True).encode()),'Wrong diagnostic identity')
  common.require(image['configuration']['pins']['libdaisy']['commit']=='1289a000bc29dae1bc0ed4129cfa2df40ef71574','Wrong SDK pin')
  common.require(image['mode']==['entry','startup','reset'].index(name) and image['configuration']['mode']==image['mode'],'Wrong mode binding')
  b=(path.parent/image['bin_file']).read_bytes();common.require(len(b)==image['bin_bytes'] and 664<=len(b)<=4096 and common.sha(b)==image['bin_sha256'],'Diagnostic BIN hash/size mismatch')
  sp,pc=struct.unpack('<II',b[:8]);common.require(sp==0x2001ffc0 and pc&1 and BASE+664<=pc<BASE+len(b),'Vector violates corrected MSP/Thumb entry bounds')
  blobs[name]=b
 return m,blobs

def trace_request(image,nonce):
 common.require(len(nonce)==16 and any(nonce),'Invalid nonce')
 return struct.pack('<4I',0x54524345,2,1024,image['mode'])+nonce+bytes.fromhex(image['artifact_id'])+bytes(960)

def decode_trace(raw,image,nonce):
 common.require(len(raw)==1024,'Short trace')
 common.require(raw[:64]==trace_request(image,nonce)[:64],'Stale/corrupt trace header: nonce or artifact differs')
 records=[];partial=[]
 for n in range(1,22):
  w=struct.unpack_from('<8I',raw,64+(n-1)*32)
  if not any(w):continue
  if w[0]!=0x53544745 or w[1]!=n or w[2]!=(~n&0xffffffff) or w[7]!=0x434d4954:
   partial.append(n);continue
  records.append(dict(stage=n,meaning=STAGES[n],ipsr=w[3],msp=w[4],control=w[5],primask=w[6]))
 numbers=[r['stage'] for r in records]
 contiguous=numbers==list(range(1,len(numbers)+1))
 f=struct.unpack_from('<32I',raw,896);fault=None
 if any(f):
  if f[0]==0x4641554c and f[31]==0x46434d54:
   names=['magic','ipsr','lr_or_exc_return','msp','psp','control','primask','basepri','cfsr','hfsr','dfsr','mmfar','bfar','shcsr','afsr','icsr','vtor','ccr']
   fault=dict(zip(names,f[:18]));fault['mmfar_valid']=bool(f[8]&(1<<7));fault['bfar_valid']=bool(f[8]&(1<<15))
  else:fault=dict(incomplete=True,raw_words=list(f))
 return dict(records=records,last_committed_stage=numbers[-1] if numbers and contiguous else None,trace_contiguous=contiguous,partial_slots=partial,fault_snapshot=fault,entry_evidence=bool(records) and contiguous,execution_complete=False,identity_verified=False,flash_programming_permitted=False)

class Transport:
 def __init__(self,dfu,serial,output):self.usb=common.Mac(dfu,output,serial);self.output=Path(output);self.number=0
 def operation(self,device,address,data=None,count=None,leave=False):
  common.require(address in CAPS,'Refusing non-diagnostic RAM address')
  common.require((data is not None)+(count is not None)+(bool(leave) and data is None)==1,'Ambiguous RAM operation')
  common.require(not leave or address==BASE,'Leave must use code vector base')
  if data is not None:common.require(address!=STACK,'Stack capture is read-only')
  if data is not None:common.require(isinstance(data,(bytes,bytearray)) and 0<len(data)<=CAPS[address] and (address==BASE or len(data)==CAPS[address]),'RAM download size invalid')
  if count is not None:common.require(isinstance(count,int) and 0<count<=CAPS[address],'RAM upload size invalid')
  common.require(self.usb.wait_dfu(10)==device,'Selected ROM device changed')
  args=list(self.usb.selectors(device));self.number+=1
  with tempfile.TemporaryDirectory() as temp:
   f=Path(temp)/'data.bin'
   if data is not None:
    f.write_bytes(data);args+=['-s','0x%x:force%s'%(address,':leave' if leave else ''),'-D',str(f)]
   elif leave:args+=['-s','0x%x:leave'%address]
   else:args+=['-s','0x%x:%d:force'%(address,count),'-U',str(f)]
   common.atomic(self.output/('command-%02d.json'%self.number),common.json_bytes(args))
   try:log=self.usb.command(args,timeout=30)
   except (common.Stop,subprocess.SubprocessError) as e:
    common.atomic(self.output/('command-%02d-error.txt'%self.number),str(e).encode());raise
   common.atomic(self.output/('command-%02d.txt'%self.number),log.encode())
   if count is not None:
    raw=f.read_bytes();common.require(len(raw)==count,'Short RAM upload');return raw

def launch(t,bundle,blobs,probe,output,manifest_sha,handoff='command',previous=None,allow_pg3=False):
 image=next(i for i in bundle['images'] if i['name']==probe);device=t.usb.wait_dfu(20)
 common.require(handoff in ('command','download'),'Unknown handoff')
 if probe!='entry':
  common.require(isinstance(previous,dict) and previous.get('manifest_sha256')==manifest_sha and previous.get('device')==device,'Prior collected evidence for same kit/device required')
  expected='entry' if probe=='startup' else 'startup'
  common.require(previous.get('probe')==expected and previous.get('entry_evidence') and previous.get('fault_snapshot') is None and not previous.get('partial_slots') and not previous.get('collection_error'),'Prior stage incomplete/faulted')
  common.require(previous.get('last_committed_stage')==(2 if probe=='startup' else 17),'Prior probe did not reach its intended halt')
 if probe=='reset':common.require(allow_pg3 and previous.get('native_record_valid') is True,'Reset requires explicit PG3 consent and complete prior native record')
 nonce=secrets.token_bytes(16);trace=trace_request(image,nonce);request=identity_wire.request(image['artifact_id'],nonce)
 session=dict(schema='parable.ram-startup-session.v2',manifest_sha256=manifest_sha,probe=probe,artifact_id=image['artifact_id'],device=device,nonce=nonce.hex(),handoff=handoff,leave_attempted=False,execution='UNKNOWN',identity_verified=False,flash_programming_permitted=False)
 common.atomic(Path(output)/'session.json',common.json_bytes(session))
 for address,data in [(BASE,blobs[probe]),(MAIL,request),(TRACE,trace)]:
  t.operation(device,address,data=data)
  common.require(t.operation(device,address,count=len(data))==data,'RAM readback mismatch; leave forbidden')
 session['leave_attempted']=True;common.atomic(Path(output)/'session.json',common.json_bytes(session))
 try:t.operation(device,BASE,data=blobs[probe] if handoff=='download' else None,leave=True)
 except (common.Stop,subprocess.SubprocessError) as e:
  session['leave_error']=str(e);common.atomic(Path(output)/'session.json',common.json_bytes(session));raise
 session['leave_command_returned_zero']=True;common.atomic(Path(output)/'session.json',common.json_bytes(session))
 return session

def collect(t,bundle,session,output,manifest_sha):
 common.require(session.get('schema')=='parable.ram-startup-session.v2' and session.get('manifest_sha256')==manifest_sha and session.get('leave_attempted') is True,'No matching recorded leave attempt')
 image=next(i for i in bundle['images'] if i['name']==session['probe'])
 common.require(image['artifact_id']==session['artifact_id'],'Session artifact mismatch')
 device=t.usb.wait_dfu(20);common.require(device==session['device'],'Different ROM device after recovery')
 # Collector is strictly READ-ONLY: never reload or erase evidence before reading it.
 raw=t.operation(device,TRACE,count=1024);common.atomic(Path(output)/'trace.raw',raw)
 mail=t.operation(device,MAIL,count=128);common.atomic(Path(output)/'mailbox.raw',mail)
 result=decode_trace(raw,image,bytes.fromhex(session['nonce']))
 result.update(schema='parable.ram-startup-observation.v2',manifest_sha256=manifest_sha,probe=image['name'],device=device,return_method='MANUAL_OR_UNOBSERVED',automatic_rom_return_verified=False,native_record_valid=False)
 try:
  w=struct.unpack('<32I',mail)
  common.require(w[:4]==(identity_wire.RESPONSE,1,32,0) and mail[16:32]==bytes.fromhex(session['nonce']) and mail[32:64]==bytes.fromhex(image['artifact_id']),'No complete nonce-bound native response')
  common.require(w[24]==BASE and w[25]==(3 if image['mode']==2 else 0) and w[27]==0 and w[28]==1 and not any(w[29:31]),'Unexpected native startup fields')
  common.require(w[31]==zlib.crc32(mail[4:124])&0xffffffff and w[20]==128,'Bad CRC or factory capacity')
  result['native_identity']=common.silicon(mail[64:76],mail[76:80],struct.pack('<H',w[20]));result['native_record_valid']=True
 except (common.Stop,ValueError,struct.error) as e:result['native_record_error']=str(e)
 fault=result['fault_snapshot']
 if fault and not fault.get('incomplete'):
  lr=fault['lr_or_exc_return'];sp=fault['psp'] if lr&4 else fault['msp']
  errors=fault['cfsr']&((1<<3)|(1<<4)|(1<<5)|(1<<11)|(1<<12)|(1<<13))
  offset=sp-STACK+(0 if lr&16 else 72)
  if lr in (0xfffffff1,0xfffffff9,0xfffffffd,0xffffffe1,0xffffffe9,0xffffffed) and not errors and 0<=offset<=1024-32:
   try:
    stack=t.operation(device,STACK,count=1024);common.atomic(Path(output)/'stack.raw',stack)
    words=struct.unpack_from('<8I',stack,offset)
    result['candidate_core_frame']=dict(zip(['r0','r1','r2','r3','r12','lr','pc','xpsr'],words))
    result['candidate_frame_note']='Recovered RAM after reset; freshness not independently guaranteed. Check xPSR Thumb bit and disassembly.'
   except (common.Stop,OSError,subprocess.SubprocessError) as e:result['collection_error']=str(e)
  else:result['frame_note']='No safe frame decode: invalid EXC_RETURN, stacking error or frame outside fixed DTCM capture range.'
 common.atomic(Path(output)/'collected.json',common.json_bytes(result))
 return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('action',choices=['launch','collect']);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--manifest-sha256',required=True);p.add_argument('--dfu-util',type=Path,required=True);p.add_argument('--serial',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--probe',choices=['entry','startup','reset'],default='entry');p.add_argument('--session',type=Path);p.add_argument('--previous-report',type=Path);p.add_argument('--handoff',choices=['command','download'],default='command');p.add_argument('--authorize-ram-diagnostic',action='store_true');p.add_argument('--authorize-pg3-reset',action='store_true');a=p.parse_args()
 try:
  bundle,blobs=load_bundle(a.manifest,a.manifest_sha256)
  common.require(a.action=='collect' or a.authorize_ram_diagnostic,'RAM launch authorization required')
  common.require(not a.output.exists(),'Use a fresh output directory');a.output.mkdir(parents=True)
  t=Transport(a.dfu_util,a.serial,a.output)
  if a.action=='launch':
   previous=json.loads(a.previous_report.read_text()) if a.previous_report else None
   launch(t,bundle,blobs,a.probe,a.output,a.manifest_sha256,a.handoff,previous,a.authorize_pg3_reset)
   print('Leave attempt recorded; EXECUTION UNKNOWN. Recover using BOOT/RESET, then collect. No automatic retry or wait for return.')
  else:
   common.require(a.session is not None,'Supply the launch session.json')
   result=collect(t,bundle,json.loads(a.session.read_text()),a.output,a.manifest_sha256)
   print(json.dumps(result,indent=2))
   if result.get('collection_error'):return 1
  return 0
 except (common.Stop,OSError,ValueError,KeyError,TypeError,subprocess.SubprocessError) as e:
  print('STOP: '+str(e)+'\nIf leave was attempted: execution is UNKNOWN. Preserve session, manually recover ROM and collect; do not retry automatically.',file=sys.stderr);return 1
if __name__=='__main__':sys.exit(main())
