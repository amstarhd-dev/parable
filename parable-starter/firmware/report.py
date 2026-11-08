"""Read actual ARM ELF/map output; no execution or hardware inference."""
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path


def inspect(toolbin, build, target="parable_seed3_minimal", app_type="BOOT_NONE"):
    elf = build / (target + ".elf")
    prefix = str(toolbin / "arm-none-eabi-")
    outputs = {}
    for name, command in {
        "size.txt": [prefix + "size", str(elf)],
        "sections.txt": [prefix + "size", "-A", str(elf)],
        "objdump.txt": [prefix + "objdump", "-h", str(elf)],
        "elf-header.txt": [prefix + "readelf", "-h", "-A", str(elf)],
        "symbols.txt": [prefix + "nm", "-C", "-S", str(elf)],
        "disassembly.txt": [prefix + "objdump", "-d", "-C", str(elf)],
    }.items():
        outputs[name] = subprocess.check_output(command, text=True)
        (build / name).write_text(outputs[name])
    if not re.search(r"Machine:\s+ARM\b", outputs["elf-header.txt"]):
        raise RuntimeError("Expected an actual ARM ELF")
    if "Tag_CPU_arch: v7E-M" not in outputs["elf-header.txt"] or "VFP registers" not in outputs["elf-header.txt"]:
        raise RuntimeError("Expected Cortex-M7-compatible hard-float ARM attributes")
    has_codec = "Codec::decode" in outputs["symbols.txt"] and "preparation::prepare" in outputs["symbols.txt"]
    if target.endswith("minimal") and ("Codec::" in outputs["symbols.txt"] or "nlohmann::" in outputs["symbols.txt"]):
        raise RuntimeError("JSON unexpectedly linked into minimal image")
    if target.endswith("codec_probe") and not has_codec:
        raise RuntimeError("Diagnostic must retain JSON decoding and native preparation")
    sections = {}
    lines = outputs["objdump.txt"].splitlines()
    for i, line in enumerate(lines):
        match = re.match(r"\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)", line)
        if match:
            name, size, vma, lma = match.groups()
            sections[name] = dict(bytes=int(size, 16), vma=int(vma, 16), lma=int(lma, 16), flags=lines[i + 1].strip())
    origin, capacity = {"BOOT_NONE": (0x08000000, 128 * 1024),
                        "BOOT_SRAM": (0x24000000, 480 * 1024),
                        "BOOT_QSPI": (0x90040000, 7936 * 1024)}[app_type]
    loaded = [s for s in sections.values() if s["bytes"] and "LOAD" in s["flags"]]
    if any(not origin <= s["lma"] < s["lma"] + s["bytes"] <= origin + capacity for s in loaded):
        raise RuntimeError("Unexpected load placement outside selected upstream layout")
    load_span = max(s["lma"] + s["bytes"] for s in loaded) - origin
    if sections[".isr_vector"]["vma"] != origin or sections[".text"]["vma"] < origin:
        raise RuntimeError("Unexpected vector/code execution location")
    binary = build / (target + ".bin")
    if binary.stat().st_size != load_span:
        raise RuntimeError("Binary length disagrees with selected load addresses")
    initial_sp, reset_vector = struct.unpack("<II", binary.read_bytes()[:8])
    text = sections[".text"]
    if initial_sp != 0x20020000 or not reset_vector & 1 or not text["vma"] <= (reset_vector & ~1) < text["vma"] + text["bytes"]:
        raise RuntimeError("Unexpected initial stack or Thumb reset-vector placement")
    regions = {"ITCM": (0, 64 * 1024), "DTCM": (0x20000000, 128 * 1024), "AXI_SRAM": (0x24000000, 512 * 1024),
               "D2_SRAM": (0x30000000, 288 * 1024), "D3_SRAM": (0x38000000, 64 * 1024),
               "BACKUP_SRAM": (0x38800000, 4096), "SDRAM": (0xc0000000, 64 * 1024 * 1024),
               "QSPI": (0x90000000, 8 * 1024 * 1024)}
    placement = {name: sum(s["bytes"] for s in sections.values()
                          if "ALLOC" in s["flags"] and start <= s["vma"] < start + size)
                 for name, (start, size) in regions.items()}
    allocated = {name: [s for s in sections.values() if s["bytes"] and "ALLOC" in s["flags"]
                       and start <= s["vma"] < start + size]
                 for name, (start, size) in regions.items()}
    first = {name: min((s["vma"] for s in items), default=None) for name, items in allocated.items()}
    spans = {name: max(s["vma"] + s["bytes"] for s in items) - first[name] if items else 0
             for name, items in allocated.items()}
    # The upstream linker merges .rodata into .text. Count included input .rodata
    # only inside the output .text, excluding the discarded-section listing.
    rodata = 0
    inside = pending = False
    for line in (build / (target + ".map")).read_text().splitlines():
        if line.startswith(".text "):
            inside = True
        elif inside and re.match(r"^\.\S", line):
            inside = False
        if not inside:
            continue
        if re.match(r"\s+\.rodata\S*", line):
            pending = True
        if pending:
            match = re.search(r"0x[0-9a-f]+\s+(0x[0-9a-f]+)", line)
            if match:
                rodata += int(match[1], 16)
                pending = False
    if not 0 < rodata <= sections[".text"]["bytes"]:
        raise RuntimeError("Map .rodata accounting disagrees with output .text")
    frames = []
    for line in (build / "daisy_pass_through.su").read_text().splitlines():
        function, size, kind = line.rsplit("\t", 2)
        frames.append(dict(function=function, bytes=int(size), qualifier=kind))
    (build / "application-stack-usage.json").write_text(json.dumps(frames, indent=2) + "\n")
    return dict(app_type=app_type, sections=sections, image_load_origin=origin,
                initial_sp=initial_sp, reset_vector=reset_vector,
                image_load_span_bytes=load_span, image_load_capacity_bytes=capacity,
                image_load_section_bytes=sum(s["bytes"] for s in loaded),
                berkeley_size=outputs["size.txt"].splitlines()[-1].split()[:4],
                execution_location={"BOOT_NONE": "internal FLASH", "BOOT_SRAM": "AXI SRAM", "BOOT_QSPI": "external QSPI (XIP)"}[app_type],
                nonvolatile_storage="internal FLASH" if app_type == "BOOT_NONE" else "QSPI at 0x90040000; loader not installed/tested",
                text_including_rodata_bytes=sections[".text"]["bytes"],
                included_input_rodata_bytes=rodata, static_placement_bytes=placement,
                static_region_first_address=first, static_region_span_bytes=spans,
                compiler_application_frames=frames, stack_top="0x20020000 (DTCM)",
                runtime_stack_high_water="not measured", runtime_heap_high_water="not measured",
                bin_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
