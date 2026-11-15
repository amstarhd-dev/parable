#!/usr/bin/env python3
"""Draw our own connectivity schematic from our literal board-facts netlist.

Graphviz is an offline documentation tool, never a DSP/runtime dependency. No
historical drawing or restricted simulator artwork is copied. Model parasitics
remain explicit in the independently executable netlist, separate from board BOM.
"""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
q = json.dumps
lines = ["graph blender {", 'graph [rankdir=LR, overlap=false, splines=polyline, bgcolor="white", label="SF016 documented original-style Blender — own connectivity schematic\nS1/S2 are SPDT: choose one throw; normal Tone connection shown by switch label.\nSU/SL Sustain10k; TH/TL Tone25k; LU/LL Level50k; BU/BL Blend100k: electrical midpoints.\nIntrinsic BJT/Ge charge and leakage are in the netlist, not extra board parts.", labelloc=t, fontsize=18];', 'node [fontname="sans-serif", fontsize=11]; edge [fontname="sans-serif", fontsize=10];']
nets = set()


def wire(net, part, port=""):
    nets.add(net)
    lines.append(f'{q("net_" + net)} -- {q(part)}' + (":" + port if port else "") + ";")


for line in (ROOT / "tests/fixtures/blender/nominal.cir").read_text().splitlines()[1:]:
    if line.startswith(".control"):
        break
    fields = line.split()
    if not fields or line[0] not in "RCDQ":
        continue
    name = fields[0]
    if name.startswith(("RL", "CJ", "CBC", "CBE")) and name != "Rload":
        continue
    if name == "C17":
        fields[2] = "effect_out"
    if name.startswith("Q"):
        part = "2N3391A" if name in ["Q1", "Q2"] else "2N3391"
        lines.append(f'{q(name)} [shape=record, color="#285c92", label="{name} {part}|{{<c>C|<b>B|<e>E}}"];')
        for net, port in zip(fields[1:4], ["c", "b", "e"]):
            wire(net, name, port)
    elif name.startswith("D"):
        lines.append(f'{q(name)} [shape=record, color="#a04b24", label="{name} 1N276|{{<a>anode|<k>cathode}}"];')
        wire(fields[1], name, "a")
        wire(fields[2], name, "k")
    else:
        color = "#28664d" if name.startswith("C") else "#505050"
        lines.append(f'{q(name)} [shape=box, color={q(color)}, label={q(name + " " + fields[3])}];')
        wire(fields[1], name)
        wire(fields[2], name)
lines += ['S1 [shape=record, label="S1 effect/bypass SPDT|{<c>common|<a>effect|<b>bypass}"];', 'S2 [shape=record, label="S2 Tone SPDT|{<c>Tone wiper|<a>normal|<b>boost}"];', 'C16 [shape=box, color=gray, label="C16 10uF supply\nconstant9V, zero AC"];']
for net, part, port in [("jack", "S1", "c"), ("effect_out", "S1", "a"), ("input", "S1", "b"), ("tone", "S2", "c"), ("level", "S2", "a"), ("toneunused", "S2", "b"), ("rail", "C16", ""), ("0", "C16", "")]:
    wire(net, part, port)
for net in sorted(nets):
    label = "ground0V" if net == "0" else "+9V ideal rail" if net == "rail" else net
    lines.append(f'{q("net_" + net)} [shape=ellipse, label={q(label)}, style=filled, fillcolor="#f1f3f5"];')
lines.append("}")
dot = a.output / "schematic.dot"
dot.write_text("\n".join(lines) + "\n")
subprocess.run(["dot", "-Tsvg", dot, "-o", a.output / "schematic.svg"], check=True)
print("Own board-facts connectivity SVG; literal netlist remains authority")
