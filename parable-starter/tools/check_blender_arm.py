#!/usr/bin/env python3
"""Pinned actual M7 object/layout/complete reachable audio graph; never execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--compiler", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
compiler = a.compiler.resolve()
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
pins = json.loads((ROOT / "firmware/dependencies.json").read_text())
commands = []


def run(args):
    args = list(map(str, args))
    commands.append(args)
    return subprocess.check_output(args, text=True)


assert run([compiler, "-dumpfullversion"]).strip() == pins["toolchain"]["version"]
tool = lambda name: compiler.parent / ("arm-none-eabi-" + name)
flags = ["-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-d16", "-mfloat-abi=hard", "-fno-exceptions", "-fno-rtti", "-fno-fast-math", "-fno-finite-math-only", "-fstack-usage", "-I" + str(ROOT / "include")]
obj = out / "parable_blender.o"
run([compiler, *flags, "-c", ROOT / "src/parable_blender.cpp", "-o", obj])
undefined = run([tool("nm"), "-u", obj])
external = {line.split()[-1] for line in undefined.splitlines()}
assert external <= {"sin", "cos", "exp", "pow", "expm1", "memcpy", "memset"}, external
assembly = run([tool("objdump"), "-drC", obj])
(out / "blender.disassembly.txt").write_text(assembly)
(out / "blender.undefined.txt").write_text(undefined)
headers = list(re.finditer(r"^[0-9a-f]+ <(.+)>:\n", assembly, re.M))
graph = {}
indirect = set()
ops = {}
for i, h in enumerate(headers):
    name = h.group(1)
    body = assembly[h.end() : headers[i + 1].start() if i + 1 < len(headers) else len(assembly)]
    calls = set(re.findall(r"R_ARM_THM_(?:CALL|JUMP24)\s+(.+)", body))
    # Include resolved local calls/tail branches too, not relocations alone.
    for target in re.findall(r"\b(?:bl|b\.w|b\.n)\s+[0-9a-f]+ <(.+)>", body):
        target = re.sub(r"\+0x[0-9a-f]+$", "", target)
        if target != name:
            calls.add(target)
    if re.search(r"\bblx\s+r\d+\b", body):
        indirect.add(name)
    graph[name] = sorted(calls)
    ops[name] = {op: len(re.findall(r"\b" + op + r"\.", body)) for op in ["vdiv", "vmul", "vadd", "vsub", "vcvt"]}

root = "parable::dsp::Blender::process(float)"
assert root in graph
reachable = set()


def visit(name):
    if name in reachable:
        return
    reachable.add(name)
    if name in external:
        assert name in {"expm1", "memcpy", "memset"}, ("control-side math in audio", name)
        return
    assert name in graph, ("unresolved local target", name)
    assert name not in indirect, ("indirect call", name)
    assert name.startswith("parable::dsp::Blender::"), name
    assert not any("::" + method + "(" in name for method in ["prepare", "matrices", "reset"]), name
    for child in graph[name]:
        visit(child)


visit(root)
usage = (out / "parable_blender.su").read_text().splitlines()
frames = {}
for line in usage:
    m = re.search(r"Blender::(\w+)\(.*\t(\d+)\tstatic$", line)
    if m and "lambda" not in line:
        frames[m.group(1)] = int(m.group(2))


def local_frame_chain(name, parents=()):
    if name in external:
        return 0
    assert name not in parents, ("unexpected recursion", name)
    method = re.search(r"Blender::(\w+)\(", name).group(1)
    assert method in frames
    return frames[method] + max([local_frame_chain(c, (*parents, name)) for c in graph[name]] or [0])


source = out / "layout.cpp"
source.write_text('#include "parable_blender.h"\n#include "parable_activation.h"\nextern "C" const uint32_t layout[]={sizeof(parable::dsp::Blender),sizeof(parable::Engine),parable::activation::Runtime::bundleBytes(),sizeof(parable::activation::Runtime)};\n')
run([compiler, *flags, "-c", source, "-o", out / "layout.o"])
run([tool("objcopy"), "--dump-section", ".rodata=" + str(out / "layout.bin"), out / "layout.o"])
layout = struct.unpack("<4I", (out / "layout.bin").read_bytes())
assert layout[0] <= 32768 and layout[1:] == (11040, 11288, 45224), layout
size = run([tool("size"), obj])
text, data, bss = map(int, size.splitlines()[-1].split()[:3])
assert text <= 24576 and data == bss == 0
report = dict(compiler=pins["toolchain"], flags=flags, object=dict(text=text, data=data, bss=bss), layout=dict(zip(["processor", "unchanged_engine", "unchanged_bundle", "unchanged_runtime"], layout)), whole_object_undefined=sorted(external), reachable_audio_graph={n: graph.get(n, []) for n in sorted(reachable)}, audio_instruction_occurrences={n: ops[n] for n in sorted(reachable) if n in ops}, compiler_static_frames=frames, known_local_audio_frame_chain_bytes=local_frame_chain(root), stack_usage=usage, stack_limitations="Compiler local frame chain only, before libm/caller/IRQ/ABI/cache effects; not measured stack high-water. Stopped prepare additionally calls matrices.", normal_work_bound="8 internal samples, <=32 Newton updates+final evaluation each; fixed14x14 elimination;14 exponentials/evaluation healthy,13 D1-open;385 interpolation+385 decimation taps;31-node reconstruction/30 capacitor histories plus nonlinear diode charge.", max_expm1_calls_per_base_frame_healthy=8*33*14, target_cycles="not measured", future_admission_ceiling_cycles_per_sample=2000, firmware_linked=False, physical_tested=False, source_sha256={name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in ["include/parable_blender.h", "src/parable_blender.cpp", "src/blender_device.h"]}, commands=commands)
(out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps({k: v for k, v in report.items() if k not in ["commands", "flags", "reachable_audio_graph", "audio_instruction_occurrences", "stack_usage"]}, indent=2))
