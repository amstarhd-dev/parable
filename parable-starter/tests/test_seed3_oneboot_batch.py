"""One-BOOT batch tests: physical acceptance requires a real Seed3, never these fakes."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock
ROOT=Path(__file__).resolve().parents[1]/'tools/resources'
sys.path.insert(0,str(ROOT))
import seed3_oneboot_batch as one
import seed3_runner as r
from test_seed3_runner import fixture,FakeBackend,IDENTITY

class Fake(FakeBackend):
    def __init__(self,manifest,output):
        super().__init__(manifest,output)
        self.identity=mock.Mock(side_effect=AssertionError('ROM silicon reads must never occur'))
        self.fail_after=0
    def flash_selected(self,device,binary):
        # Exact 128KiB images already verified by load_kit.
        self.flash(device,binary,IDENTITY)

class Batch(unittest.TestCase):
    def test_all_15_reports_15_rom_returns_and_first_native_identity(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);manifest,digest=fixture(root)
            backend=Fake(manifest,root/'out')
            one.batch(backend,manifest,digest,root,root/'out',30)
            self.assertEqual(len(backend.flashes),15)
            self.assertEqual(backend.flashes[0],'rat_1x_cost_only')
            self.assertEqual(set(backend.flashes),{a['profile'] for a in manifest['artifacts']})
            self.assertEqual(json.loads((root/'out/batch-complete.json').read_text())['rom_returns'],15)
            self.assertEqual(json.loads((root/'out/results.json').read_text())['full_15_image_acceptance'],True)
            session=json.loads((root/'out/session.json').read_text())
            self.assertEqual(session['initial_silicon_read'],'NOT AVAILABLE THROUGH ROM DFU')
            self.assertEqual(json.loads((root/'out/first-native-silicon.json').read_text())['uid'],IDENTITY['uid'])
            backend.identity.assert_not_called()
    def test_first_handoff_failure_stops_before_second_flash(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);manifest,digest=fixture(root)
            backend=Fake(manifest,root/'out');backend.fail_transition=1
            with self.assertRaises(r.Stop):
                one.batch(backend,manifest,digest,root,root/'out',30)
            self.assertEqual(backend.flashes,['rat_1x_cost_only'])
            self.assertFalse((root/'out/batch-complete.json').exists())
            self.assertEqual(len(list((root/'out').glob('*.txt'))),1)
            backend.identity.assert_not_called()
    def test_changed_native_identity_stops_before_third_flash(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);manifest,digest=fixture(root);backend=Fake(manifest,root/'out')
            original=backend.flash_selected
            def changed(device,binary):
                original(device,binary)
                if len(backend.flashes)==2:backend.control_uid='aaaaaaaaaaaaaaaaaaaaaaaa'
            backend.flash_selected=changed
            with self.assertRaisesRegex(r.Stop,'Changed silicon'):
                one.batch(backend,manifest,digest,root,root/'out',30)
            self.assertEqual(len(backend.flashes),2)
            self.assertFalse((root/'out/batch-complete.json').exists())
    def test_wrong_cdc_identity_rejected_without_authorizing_reset(self):
        class FakePort:
            def __init__(self,chip):
                self.lines=['PARABLE_CONTROL_V1 artifact_id=A nonce=N uid=123456789012345678901234 chip_id='+str(chip)+' flash_kib=128 board=seed3 handoff='+r.HANDOFF+'\n']
            def send(self,v):self.nonce=v.split()[1]
            def line(self,end):
                return self.lines.pop(0).replace('artifact_id=A','artifact_id='+'a'*64).replace('nonce=N','nonce='+self.nonce)
        for chip in (0,0x451,0x8):
            with self.assertRaises(r.Stop):
                one.cdc_identity(FakePort(chip),dict(artifact_id='a'*64))
    def test_flash_selected_does_not_call_rom_silicon_and_never_retries(self):
        with tempfile.TemporaryDirectory() as t:
            backend=one.Device.__new__(one.Device)
            backend.output=Path(t);backend.sequence=0;backend.wait_dfu=mock.Mock(return_value=dict(path='20-1',serial='S',layout='flash'))
            backend.selectors=mock.Mock(return_value=['dfu-util','-a','0'])
            backend.command=mock.Mock(return_value='Uploaded and left')
            binary=Path(t)/'test.bin';binary.write_bytes(b'abc')
            backend.flash_selected(backend.wait_dfu.return_value,binary)
            self.assertEqual(backend.command.call_count,1)
            self.assertIn('-D',backend.command.call_args[0][0])
            backend.command.side_effect=r.Stop('flash failure')
            with self.assertRaises(r.Stop):backend.flash_selected(backend.wait_dfu.return_value,binary)
            self.assertEqual(backend.command.call_count,2)
            self.assertFalse((Path(t)/'flash-02.log').exists())
    def test_runner_has_no_direct_rom_register_access(self):
        source=(ROOT/'seed3_oneboot_batch.py').read_text()
        for forbidden in ('0x1ff1e800','0x5c001000','0x1ff1e880','backend.identity('):
            self.assertNotIn(forbidden,source)

if __name__=='__main__':unittest.main()
