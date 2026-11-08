#!/usr/bin/env python3
"""Pinned Linux x86_64 ARM build only. Never flashes or runs target firmware."""
import argparse
import hashlib
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import tarfile
from pathlib import Path
from report import inspect

FIRMWARE = Path(__file__).resolve().parent
PROJECT = FIRMWARE.parent
PINS = json.loads((FIRMWARE / "dependencies.json").read_text())


def run(args, log, allow_failure=False, cwd=None):
    with log.open("w") as output:
        output.write("$ " + shlex.join(map(str, args)) + "\n")
        output.flush()
        result = subprocess.run(list(map(str, args)), cwd=cwd, stdout=output,
                                stderr=subprocess.STDOUT, timeout=600)
    if result.returncode and not allow_failure:
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}\n" + log.read_text()[-5000:])
    return result.returncode


def git(directory, *args):
    return subprocess.check_output(["git", "-C", str(directory), *args], text=True).strip()


def archive(cache, pin, filename):
    path = cache / filename
    if not path.exists():
        subprocess.run(["curl", "-fL", "--retry", "3", "--connect-timeout", "20", "--max-time", "180",
                        pin["url"], "-o", str(path)], check=True)
    with path.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest != pin["sha256"]:
        raise RuntimeError(f"Checksum mismatch: {path}")
    destination = cache / pin["directory"]
    if not destination.exists():
        with tarfile.open(path) as tar:
            tar.extractall(cache, filter="data")
    return destination


def build(cache, output):
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise RuntimeError("This pinned binary toolchain supports Linux x86_64 only")
    layouts = {"BOOT_NONE": "core/STM32H750IB_flash.lds", "BOOT_SRAM": "core/STM32H750IB_sram.lds",
               "BOOT_QSPI": "core/STM32H750IB_qspi.lds"}
    if PINS["target"] != dict(cpu="cortex-m7", fpu="fpv5-d16", float_abi="hard", layouts=layouts):
        raise RuntimeError("Only the three audited upstream Cortex-M7 layouts are supported")
    os.environ["GIT_TERMINAL_PROMPT"] = "0"
    cache.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    toolroot = archive(cache, PINS["toolchain"], "arm-toolchain.tar.gz")
    toolbin = toolroot / "bin"
    compiler = toolbin / "arm-none-eabi-g++"
    version = subprocess.check_output([compiler, "-dumpfullversion"], text=True).strip()
    if version != PINS["toolchain"]["version"]:
        raise RuntimeError("Unexpected compiler version")
    lib = cache / "libDaisy"
    pin = PINS["libdaisy"]
    if not lib.exists():
        run(["git", "init", lib], output / "git-init.log")
        run(["git", "-C", lib, "remote", "add", "origin", pin["url"]], output / "git-remote.log")
        run(["git", "-C", lib, "fetch", "--depth", "1", "origin", pin["commit"]], output / "git-fetch.log")
        run(["git", "-C", lib, "checkout", "--detach", pin["commit"]], output / "git-checkout.log")
    if git(lib, "rev-parse", "HEAD") != pin["commit"]:
        raise RuntimeError("Cached libDaisy has the wrong commit; use a fresh managed cache")
    run(["git", "-C", lib, "submodule", "update", "--init", "--depth", "1", "--", *pin["submodules"]], output / "submodules.log")
    for path, commit in pin["submodules"].items():
        if git(lib / path, "rev-parse", "HEAD") != commit or git(lib / path, "status", "--porcelain"):
            raise RuntimeError(f"Submodule pin/cleanliness failure: {path}")
    if git(lib, "status", "--porcelain"):
        raise RuntimeError("Cached libDaisy sources are modified")
    loader = PINS["bootloader_audit"]
    if hashlib.sha256((lib / loader["bundled_binary"]).read_bytes()).hexdigest() != loader["sha256"]:
        raise RuntimeError("Bundled bootloader audit hash differs")
    # Host and ARM probes must use exactly the same JSON dependency.
    cmake = (PROJECT / "CMakeLists.txt").read_text()
    if PINS["json"]["url"] not in cmake or PINS["json"]["sha256"] not in cmake:
        raise RuntimeError("Host/ARM JSON pins differ")
    jsonroot = archive(cache, PINS["json"], "json.tar.xz")
    run(["make", "-C", lib, "clean"], output / "libdaisy-clean.log")
    run(["make", "-C", lib, "-j2", f"GCC_PATH={toolbin}", "OPT=-O3 -fstack-usage"], output / "libdaisy-build.log")
    minimal = output / "minimal"
    common = ["make", "-C", FIRMWARE, "-B", "-j2", f"LIBDAISY_DIR={lib}", f"GCC_PATH={toolbin}"]
    run([*common, "APP_TYPE=BOOT_NONE", f"BUILD_DIR={minimal}"], output / "firmware-build.log")
    flags = ["-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-d16", "-mfloat-abi=hard", "-std=gnu++17",
             "-O2", "-fno-exceptions", "-fno-rtti", "-fshort-enums", "-fno-fast-math", "-fno-finite-math-only", "-fstack-usage"]
    run([compiler, *flags, "-I", PROJECT / "include", "-c", PROJECT / "src/parable_prepare.cpp",
         "-o", output / "preparation.o"], output / "preparation-compile.log")
    run([toolbin / "arm-none-eabi-size", output / "preparation.o"], output / "preparation-size.txt")
    run([toolbin / "arm-none-eabi-readelf", "-h", "-A", output / "preparation.o"], output / "preparation-elf-header.txt")
    # Compile the shared experimental drive without linking it into the unchanged
    # minimal/codec images. This is ARM object evidence, never a CPU/boot claim.
    run([compiler, *flags, "-I", PROJECT / "include", "-c", PROJECT / "src/parable_drive.cpp",
         "-o", output / "drive.o"], output / "drive-compile.log")
    run([toolbin / "arm-none-eabi-size", output / "drive.o"], output / "drive-size.txt")
    run([toolbin / "arm-none-eabi-readelf", "-h", "-A", output / "drive.o"], output / "drive-elf-header.txt")
    run([toolbin / "arm-none-eabi-nm", "-C", "-u", output / "drive.o"], output / "drive-undefined.txt")
    if re.search(r"operator new|malloc|calloc|realloc|__cxa_throw|pthread|printf|fstream", (output / "drive-undefined.txt").read_text()):
        raise RuntimeError("Unexpected allocation/exception/I/O dependency in drive object")
    layout = output / "drive-layout.cpp"
    layout.write_text('#include "parable_drive.h"\nextern "C" { char parable_drive_instance[sizeof(parable::dsp::Drive)]; }\n')
    run([compiler, *flags, "-I", PROJECT / "include", "-c", layout, "-o", output / "drive-layout.o"], output / "drive-layout.log")
    symbols = subprocess.check_output([toolbin / "arm-none-eabi-nm", "-S", output / "drive-layout.o"], text=True)
    instance = re.search(r"\b([0-9a-f]+) B parable_drive_instance\b", symbols)
    if not instance:
        raise RuntimeError("Missing ARM instance layout measurement")
    # A diagnostic, never-shipping full-codec image. Only its known FLASH overflow
    # is a non-gating outcome; unexpected compile/API/link failures still fail CI.
    probe = output / "codec-probe"
    if probe.exists():
        for pattern in ("*.elf", "*.hex", "*.bin"):
            for path in probe.glob(pattern):
                path.unlink()
    status = run([*common, "APP_TYPE=BOOT_NONE", "CODEC_LINK_PROBE=1", f"JSON_DIR={jsonroot}", f"BUILD_DIR={probe}"],
                 output / "codec-probe.log", allow_failure=True)
    diagnostic = (output / "codec-probe.log").read_text()
    overflow = re.search(r"region `FLASH' overflowed by (\d+) bytes", diagnostic)
    unexpected = any("error:" in line and not line.startswith("collect2: error: ld returned")
                     for line in diagnostic.splitlines()) or "undefined reference" in diagnostic
    if status and (not overflow or unexpected):
        raise RuntimeError("Unexpected codec probe failure; see codec-probe.log")
    images = {"minimal_internal_flash": inspect(toolbin, minimal)}
    for app_type, name in (("BOOT_SRAM", "diagnostic_sram"), ("BOOT_QSPI", "diagnostic_qspi")):
        directory = output / name
        run([*common, f"APP_TYPE={app_type}", "CODEC_LINK_PROBE=1", f"JSON_DIR={jsonroot}",
             f"BUILD_DIR={directory}"], output / (name + "-build.log"))
        images[name] = inspect(toolbin, directory, "parable_seed3_codec_probe", app_type)
    report = dict(images=images)
    report["drive_kernel"] = dict(object_size=(output / "drive-size.txt").read_text().splitlines()[-1].split()[:4],
                                instance_bytes=int(instance[1], 16), linked_into_firmware=False,
                                hardware_execution="not performed", target_cpu_cost="not measured")
    report["codec_probe"] = dict(status="flash_overflow" if status else "linked",
                                  make_exit_code=status, flash_overflow_bytes=int(overflow[1]) if overflow else 0,
                                  hardware_execution="not performed")
    report["pins"] = PINS
    report["compiler"] = subprocess.check_output([compiler, "--version"], text=True).splitlines()[0]
    report["host_tools"] = dict(python=platform.python_version(),
                                make=subprocess.check_output(["make", "--version"], text=True).splitlines()[0],
                                git=subprocess.check_output(["git", "--version"], text=True).strip())
    report["source_commit"] = git(PROJECT, "rev-parse", "HEAD")
    report["source_worktree_dirty"] = bool(git(PROJECT, "status", "--porcelain"))
    inputs = ["firmware/daisy_pass_through.cpp", "firmware/Makefile", "firmware/build.py", "firmware/report.py",
              "firmware/dependencies.json", "include/parable_core.h", "include/parable_patch.h", "include/parable_prepare.h",
              "src/parable_prepare.cpp", "src/prepare_internal.h", "src/parable_patch.cpp", "src/patch_json.cpp", "src/patch_json.h",
              "include/parable_drive.h", "src/parable_drive.cpp"]
    report["source_sha256"] = {name: hashlib.sha256((PROJECT / name).read_bytes()).hexdigest() for name in inputs}
    report["linker_sha256"] = {mode: hashlib.sha256((lib / path).read_bytes()).hexdigest()
                               for mode, path in layouts.items()}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    # Keep notices with redistributed build artifacts, not only links to licenses.
    notices = output / "licenses"
    for path in ["LICENSE", "Drivers/CMSIS-DSP/LICENSE.txt", "Drivers/CMSIS_5/LICENSE.txt",
                 "Drivers/CMSIS-Device/ST/STM32H7xx/LICENSE.md", "Drivers/STM32H7xx_HAL_Driver/LICENSE.md",
                 "Middlewares/ST/STM32_USB_Device_Library/LICENSE.md"]:
        destination = notices / "libDaisy" / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(lib / path, destination)
    shutil.copytree(toolroot / "distro-info/licenses", notices / "toolchain", dirs_exist_ok=True)
    shutil.copyfile(jsonroot / "LICENSE.MIT", notices / "nlohmann-json-LICENSE.MIT")
    shutil.copyfile(PROJECT / "third_party/ARM_DEPENDENCIES.md", notices / "ARM_DEPENDENCIES.md")
    print(json.dumps({"images": {name: {k: image[k] for k in (
        "app_type", "execution_location", "berkeley_size", "image_load_span_bytes", "text_including_rodata_bytes",
        "included_input_rodata_bytes", "static_placement_bytes", "static_region_span_bytes")}
        for name, image in images.items()}, "drive_kernel": report["drive_kernel"],
        "internal_flash_codec_probe": report["codec_probe"]}, indent=2))
    print(f"Build-only evidence: {output}; no flashing or hardware execution performed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, default=PROJECT / "build/arm-cache")
    parser.add_argument("--output", type=Path, default=PROJECT / "build/arm")
    args = parser.parse_args()
    build(args.cache.resolve(), args.output.resolve())
