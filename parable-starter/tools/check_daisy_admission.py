#!/usr/bin/env python3
"""Seed3 first-boot DWT timing screen. TIMING SCREEN ONLY; never production admission."""
import argparse, json, re, sys
from pathlib import Path
LEDGER = Path(__file__).resolve().parent.parent / "docs" / "daisy_effect_inventory.json"
REQUIRED = ("core_hz_rcc", "configured_rate_millihz", "block_frames", "estimated_deadline_cycles",
            "counter_enabled", "blocks", "valid_blocks", "max_valid_callback_body_cycles",
            "at_or_above_estimated_deadline", "invalid_intervals", "frame_mismatches")
def parse(raw):
    banner=None
    fields={}
    for line in raw.splitlines():
        line=line.strip()
        if line.startswith("PAR009 "):
            banner=line[7:]
            fields={}
        if banner is not None:
            for m in re.finditer(r"\\b([a-z_]+)=([0-9]+)(?=\\s|$)",line):
                fields[m[1]]=int(m[2])
    if not banner: raise ValueError("No PAR009 banner")
    missing=set(REQUIRED)-set(fields)
    if missing: raise ValueError("Missing "+str(sorted(missing)))
    return banner,fields
def check(raw,voice,stimulus="unconnected"):
    ledger=json.loads(LEDGER.read_text())
    voices={x["id"]:x for x in ledger["effects"]}
    if voice not in ("bypass","rat"): raise ValueError("No tagged firmware capture for "+voice)
    banner,v=parse(raw)
    if voice=="bypass" and not banner.startswith("MONO_BYPASS"): raise ValueError("Wrong bypass firmware")
    if voice=="rat" and "RAT_COMPOSITE" not in banner: raise ValueError("Wrong RAT firmware")
    problems=[]
    if (v["core_hz_rcc"],v["configured_rate_millihz"],v["block_frames"],v["estimated_deadline_cycles"]) != (480000000,48000000,48,480000):
        problems.append("clock/rate/frame/budget mismatch")
    if not v["counter_enabled"] or v["blocks"]!=30000 or v["valid_blocks"]!=30000: problems.append("incomplete counter capture")
    if v["invalid_intervals"] or v["frame_mismatches"] or v["at_or_above_estimated_deadline"]: problems.append("invalid intervals, frame mismatches or deadline overrun")
    utilization=100*v["max_valid_callback_body_cycles"]/v["estimated_deadline_cycles"]
    if utilization > ledger["full_chain_limit_percent"]: problems.append("exceeds 70% full-chain CPU target")
    if voice!="bypass" and utilization>ledger["isolated_target_percent"][voices[voice]["category"]]:
        problems.append("composite exceeds provisional 20% drive target; cannot attribute RAT-only cycles")
    return dict(voice=voice,stimulus=stimulus,banner=banner,cycles=v["max_valid_callback_body_cycles"],
                deadline=v["estimated_deadline_cycles"],utilization_percent=round(utilization,4),
                timing_screen="FAIL" if problems else "PASS_TIMING_ONLY",problems=problems,
                production_admitted=False, limitations=["callback body only, SDK/ISR tail excluded",
                "input and control worst-case not certified","stack, SDRAM, actual analog I/O, listening and combined patch pending"])
def test():
    def raw(header,cycles):
        return (f"PAR009 {header}\\ncore_hz_rcc=480000000 configured_rate_millihz=48000000 block_frames=48 estimated_deadline_cycles=480000\\n"
                f"counter_enabled=1 blocks=30000 valid_blocks=30000 max_valid_callback_body_cycles={cycles} last_valid_cycles=3246\\n"
                "at_or_above_estimated_deadline=0 invalid_intervals=0 frame_mismatches=0 max_valid_ratio_ppm=lu\\n")
    a=check(raw("RAT_COMPOSITE Gain(1.8) Rat",370915),"rat")
    b=check(raw("MONO_BYPASS trim=1",3357),"bypass")
    assert a["timing_screen"]=="FAIL" and abs(a["utilization_percent"]-77.274)<.01
    assert b["timing_screen"]=="PASS_TIMING_ONLY" and b["production_admitted"] is False
    ledger=json.loads(LEDGER.read_text())
    ids=[x["id"] for x in ledger["effects"]]
    assert len(ids)==len(set(ids))>=21
    assert all(x["production_admitted"] is False for x in ledger["effects"])
    assert next(x for x in ledger["effects"] if x["id"]=="cancer_cloud")["disposition"]=="memory_blocked"
    print("Daisy resource ledger and historical DWT fixtures PASS (not production proof)")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    g=p.add_mutually_exclusive_group(required=True)
    g.add_argument("--list",action="store_true")
    g.add_argument("--selftest",action="store_true")
    g.add_argument("--report",type=Path)
    p.add_argument("--voice",default="rat")
    p.add_argument("--stimulus",choices=["unconnected","synthetic","instrument"],default="unconnected")
    a=p.parse_args()
    try:
        if a.selftest: test();return 0
        if a.list:
            ledger=json.loads(LEDGER.read_text())
            for x in sorted(ledger["effects"],key=lambda x:(x["priority"],x["id"])):
                print(f'P{x["priority"]} {x["id"]:18} {x["disposition"]:19} {x["source"]}')
            return 0
        result=check(a.report.read_text(),a.voice,a.stimulus)
        print(json.dumps(result,indent=2))
        return 2 if result["timing_screen"]=="FAIL" else 0
    except (OSError,ValueError,AssertionError) as e:
        print("Invalid resource evidence: "+str(e),file=sys.stderr);return 3
if __name__=="__main__": sys.exit(main())
