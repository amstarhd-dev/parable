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
    # Failed rebuilds must not retain a stale complete-kit receipt.
    (output / "report.json").unlink(missing_ok=True)
    kit = output / "first-boot"
    kit.mkdir(exist_ok=True)
    for name in ("manifest.json", "SHA256SUMS"):
        (kit / name).unlink(missing_ok=True)
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
    def image_args(directory, app_type):
        # Upstream's directory rule uses bare mkdir. -B must force source rebuilds
        # without re-running that rule on an existing directory. Never delete it.
        directory.mkdir(exist_ok=True)
        return [*common, "-o", directory, f"APP_TYPE={app_type}", f"BUILD_DIR={directory}"]
    run(image_args(minimal, "BOOT_NONE"), output / "firmware-build.log")
    flags = ["-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-d16", "-mfloat-abi=hard", "-std=gnu++17",
             "-O2", "-fno-exceptions", "-fno-rtti", "-fshort-enums", "-fno-fast-math", "-fno-finite-math-only", "-fstack-usage"]
    run([compiler, *flags, "-I", PROJECT / "include", "-c", PROJECT / "src/parable_prepare.cpp",
         "-o", output / "preparation.o"], output / "preparation-compile.log")
    run([toolbin / "arm-none-eabi-size", output / "preparation.o"], output / "preparation-size.txt")
    run([toolbin / "arm-none-eabi-readelf", "-h", "-A", output / "preparation.o"], output / "preparation-elf-header.txt")
    # Keep the standalone object receipt; a separate active image below proves linking.
    run([compiler, *flags, "-I", PROJECT / "include", "-c", PROJECT / "src/parable_rat.cpp",
         "-o", output / "rat.o"], output / "rat-compile.log")
    run([toolbin / "arm-none-eabi-size", output / "rat.o"], output / "rat-size.txt")
    run([toolbin / "arm-none-eabi-readelf", "-h", "-A", output / "rat.o"], output / "rat-elf-header.txt")
    run([toolbin / "arm-none-eabi-nm", "-C", "-u", output / "rat.o"], output / "rat-undefined.txt")
    if re.search(r"operator new|malloc|calloc|realloc|__cxa_throw|pthread|printf|fstream", (output / "rat-undefined.txt").read_text()):
        raise RuntimeError("Unexpected allocation/exception/I/O dependency in RAT object")
    layout = output / "rat-layout.cpp"
    layout.write_text('#include "parable_rat.h"\nextern "C" { char parable_rat_instance[sizeof(parable::dsp::Rat)]; }\n')
    run([compiler, *flags, "-I", PROJECT / "include", "-c", layout, "-o", output / "rat-layout.o"], output / "rat-layout.log")
    symbols = subprocess.check_output([toolbin / "arm-none-eabi-nm", "-S", output / "rat-layout.o"], text=True)
    instance = re.search(r"\b([0-9a-f]+) B parable_rat_instance\b", symbols)
    if not instance:
        raise RuntimeError("Missing ARM RAT instance measurement")
    # SF-002 remains standalone, with the same pins/flags and no firmware activation.
    run([compiler, *flags, "-I", PROJECT / "include", "-c", PROJECT / "src/parable_cmos.cpp",
         "-o", output / "cmos.o"], output / "cmos-compile.log")
    run([toolbin / "arm-none-eabi-size", output / "cmos.o"], output / "cmos-size.txt")
    run([toolbin / "arm-none-eabi-readelf", "-h", "-A", output / "cmos.o"], output / "cmos-elf-header.txt")
    run([toolbin / "arm-none-eabi-nm", "-C", "-u", output / "cmos.o"], output / "cmos-undefined.txt")
    if re.search(r"operator new|malloc|calloc|realloc|__cxa_throw|pthread|printf|fstream", (output / "cmos-undefined.txt").read_text()):
        raise RuntimeError("Unexpected allocation/exception/I/O dependency in CMOS object")
    cmos_layout = output / "cmos-layout.cpp"
    cmos_layout.write_text('#include "parable_cmos.h"\nextern "C" { char parable_cmos_instance[sizeof(parable::dsp::Cmos)]; }\n')
    run([compiler, *flags, "-I", PROJECT / "include", "-c", cmos_layout,
         "-o", output / "cmos-layout.o"], output / "cmos-layout.log")
    cmos_symbols = subprocess.check_output([toolbin / "arm-none-eabi-nm", "-S", output / "cmos-layout.o"], text=True)
    cmos_instance = re.search(r"\b([0-9a-f]+) B parable_cmos_instance\b", cmos_symbols)
    if not cmos_instance:
        raise RuntimeError("Missing ARM CMOS instance measurement")
    # SF003: compile the real standalone GP kernel, not a substitute firmware stub.
    run([compiler, *flags, "-I", PROJECT / "include", "-c", PROJECT / "src/parable_germanium.cpp",
         "-o", output / "germanium.o"], output / "germanium-compile.log")
    run([toolbin / "arm-none-eabi-size", output / "germanium.o"], output / "germanium-size.txt")
    run([toolbin / "arm-none-eabi-readelf", "-h", "-A", output / "germanium.o"], output / "germanium-elf-header.txt")
    run([toolbin / "arm-none-eabi-nm", "-C", "-u", output / "germanium.o"], output / "germanium-undefined.txt")
    if re.search(r"operator new|malloc|calloc|realloc|__cxa_throw|pthread|printf|fstream", (output / "germanium-undefined.txt").read_text()):
        raise RuntimeError("Unexpected allocation/exception/I/O dependency in germanium object")
    germanium_layout = output / "germanium-layout.cpp"
    germanium_layout.write_text('#include "parable_germanium.h"\nextern "C" { char parable_germanium_instance[sizeof(parable::dsp::Germanium)]; }\n')
    run([compiler, *flags, "-I", PROJECT / "include", "-c", germanium_layout,
         "-o", output / "germanium-layout.o"], output / "germanium-layout.log")
    germanium_symbols = subprocess.check_output([toolbin / "arm-none-eabi-nm", "-S", output / "germanium-layout.o"], text=True)
    germanium_instance = re.search(r"\b([0-9a-f]+) B parable_germanium_instance\b", germanium_symbols)
    if not germanium_instance:
        raise RuntimeError("Missing ARM germanium instance measurement")
    # A diagnostic, never-shipping full-codec image. Only its known FLASH overflow
    # is a non-gating outcome; unexpected compile/API/link failures still fail CI.
    probe = output / "codec-probe"
    if probe.exists():
        for pattern in ("*.elf", "*.hex", "*.bin"):
            for path in probe.glob(pattern):
                path.unlink()
    status = run([*image_args(probe, "BOOT_NONE"), "CODEC_LINK_PROBE=1", f"JSON_DIR={jsonroot}"],
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
        run([*image_args(directory, app_type), "CODEC_LINK_PROBE=1", f"JSON_DIR={jsonroot}"],
            output / (name + "-build.log"))
        images[name] = inspect(toolbin, directory, "parable_seed3_codec_probe", app_type)
    # Narrow production-shaped integration target: active fixed Gain/Rat/LP path,
    # the same callback/Engine/kernel, no JSON. Preserve the bypass startup image.
    rat_image = output / "rat_integration"
    rat_status = run([*image_args(rat_image, "BOOT_NONE"), "RAT_LINK_PROBE=1"],
                     output / "rat-integration-build.log", allow_failure=True)
    rat_log = (output / "rat-integration-build.log").read_text()
    rat_overflow = re.search(r"region `FLASH' overflowed by (\d+) bytes", rat_log)
    if rat_status:
        if not rat_overflow or "undefined reference" in rat_log or any(
                "error:" in line and not line.startswith("collect2: error: ld returned") for line in rat_log.splitlines()):
            raise RuntimeError("Unexpected integrated RAT link failure")
        rat_image = output / "rat_integration_sram"
        run([*image_args(rat_image, "BOOT_SRAM"), "RAT_LINK_PROBE=1"], output / "rat-integration-sram-build.log")
    images["rat_integration"] = inspect(toolbin, rat_image, "parable_seed3_rat_probe",
                                        "BOOT_SRAM" if rat_status else "BOOT_NONE")
    engine_layout = output / "engine-layout.cpp"
    engine_layout.write_text('#include "parable_core.h"\nextern "C" { char parable_engine_instance[sizeof(parable::Engine)]; '
                             'char parable_patch_instance[sizeof(parable::Patch)]; }\n')
    run([compiler, *flags, "-I", PROJECT / "include", "-c", engine_layout, "-o", output / "engine-layout.o"],
        output / "engine-layout.log")
    layout_symbols = subprocess.check_output([toolbin / "arm-none-eabi-nm", "-S", output / "engine-layout.o"], text=True)
    (output / "engine-layout-symbols.txt").write_text(layout_symbols)
    sizes = {}
    for label in ("engine", "patch"):
        match = re.search(r"\b([0-9a-f]+) B parable_" + label + r"_instance\b", layout_symbols)
        if not match:
            raise RuntimeError("Missing ARM layout measurement: " + label)
        sizes[label + "_bytes"] = int(match[1], 16)
    first_boot_targets = {"led_recovery": "parable_seed3_led", "mono_bypass": "parable_seed3_bypass",
                          "rat_composite": "parable_seed3_rat_composite"}
    for name, target in first_boot_targets.items():
        directory = kit / name
        # No automatic layout fallback in this kit. Any 128 KiB overflow FAILS.
        run([*image_args(directory, "BOOT_NONE"), f"FIRST_BOOT_IMAGE={name}"], output / (name + "-build.log"))
        shutil.copyfile(output / (name + "-build.log"), directory / "build.log")
        images["first_boot_" + name] = inspect(toolbin, directory, target, "BOOT_NONE")
    # Actual compiled integer-only hook audit, supplementary to the linked images.
    measurement = output / "measurement-probe.cpp"
    measurement.write_text('#include "runtime_measurement.h"\nextern "C" {\n'
        'char parable_capture_instance[sizeof(parable::first_boot::Capture)];\n'
        'void parable_measure_record(parable::first_boot::Capture* c, unsigned a, unsigned b, unsigned frames) '
        '{ c->observe(a,b,frames); }\n}\n')
    run([compiler, *flags, "-I", FIRMWARE, "-c", measurement, "-o", output / "measurement-probe.o"],
        output / "measurement-probe.log")
    run([toolbin / "arm-none-eabi-objdump", "-d", "-C", output / "measurement-probe.o"],
        output / "measurement-disassembly.txt")
    run([toolbin / "arm-none-eabi-nm", "-C", "-u", output / "measurement-probe.o"], output / "measurement-undefined.txt")
    # run() prefixes its command; strip that before checking actual nm output.
    if (output / "measurement-undefined.txt").read_text().splitlines()[1:]:
        raise RuntimeError("Callback integer accounting unexpectedly needs external helpers")
    if re.search(r"\tv[a-z0-9]+\.[a-z0-9]+\b", (output / "measurement-disassembly.txt").read_text()):
        raise RuntimeError("Unexpected floating instructions in callback accounting")
    measure_symbols = subprocess.check_output([toolbin / "arm-none-eabi-nm", "-S", output / "measurement-probe.o"], text=True)
    (output / "measurement-symbols.txt").write_text(measure_symbols)
    capture_size = re.search(r"\b([0-9a-f]+) B parable_capture_instance\b", measure_symbols)
    if not capture_size:
        raise RuntimeError("Missing actual ARM capture layout")
    report = dict(images=images)
    report["rat_kernel"] = dict(object_size=(output / "rat-size.txt").read_text().splitlines()[-1].split()[:4],
                              instance_bytes=int(instance[1],16), linked_into_audio_firmware=True,
                              hardware_execution="not performed", target_cpu_cost="not measured")
    report["rat_integration"] = dict(**sizes, native_rat_limit=1,
        boot_none_make_exit_code=rat_status, boot_none_overflow_bytes=int(rat_overflow[1]) if rat_overflow else 0,
        startup_patch="Gain(1.8) -> Rat(.35,.5,.7) -> OnePoleLowpass(1800 Hz); trim=.8",
        canonical_host_fixture="tests/fixtures/patch-v1/rat-composite.json",
        firmware_fixture="static native build-only probe, not editable/save authority",
        callback_deadline="not measured", hardware_execution="not performed")
    report["first_boot"] = dict(capture_bytes=int(capture_size[1], 16), capture_limit_callbacks=30000,
        counter="DWT CYCCNT; CM7 SystemCoreClockUpdate RCC readback", interval_contract="each callback <2^31 cycles",
        publication="one frozen summary, release/acquire completion, no ongoing mutable reader",
        retrieval="control-side existing USB CDC, initialized only after capture; immutable text repeated",
        callback_cpu="not measured", glitches="not observed/tested", hardware_execution="not performed")
    report["cmos_kernel"] = dict(object_size=(output / "cmos-size.txt").read_text().splitlines()[-1].split()[:4],
                               instance_bytes=int(cmos_instance[1],16), linked_into_audio_firmware=False,
                               hardware_execution="not performed", target_cpu_cost="not measured")
    report["germanium_kernel"] = dict(object_size=(output / "germanium-size.txt").read_text().splitlines()[-1].split()[:4],
                                    instance_bytes=int(germanium_instance[1],16), linked_into_audio_firmware=False,
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
    report["source_head_commit"] = os.environ.get("HEAD_COMMIT", report["source_commit"])
    report["source_worktree_dirty"] = bool(git(PROJECT, "status", "--porcelain"))
    inputs = ["firmware/daisy_pass_through.cpp", "firmware/Makefile", "firmware/build.py", "firmware/report.py",
              "firmware/dependencies.json", "include/parable_core.h", "include/parable_patch.h", "include/parable_prepare.h",
              "src/parable_prepare.cpp", "src/prepare_internal.h", "src/parable_patch.cpp", "src/patch_json.cpp", "src/patch_json.h",
              "include/parable_rat.h", "src/parable_rat.cpp", "src/rat_circuit_math.h"]
    inputs += ["include/parable_cmos.h", "src/parable_cmos.cpp", "src/cmos_device.h"]
    inputs += ["include/parable_germanium.h", "src/parable_germanium.cpp", "src/germanium_device.h"]
    inputs += ["tests/fixtures/patch-v1/rat-composite.json"]
    inputs += ["firmware/seed3_led.cpp", "firmware/first_boot_audio.h", "firmware/runtime_measurement.h",
               "docs/PAR009_GATE0.md", "docs/SEED3_FIRST_DAY.md", "tests/test_firmware_measurement.cpp"]
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
    shutil.copytree(PROJECT / "third_party/labrat", notices / "labrat", dirs_exist_ok=True)
    shutil.copytree(PROJECT / "third_party/cmos", notices / "cmos", dirs_exist_ok=True)
    shutil.copytree(PROJECT / "third_party/germanium", notices / "germanium", dirs_exist_ok=True)
    shutil.copytree(notices, kit / "licenses", dirs_exist_ok=True)
    for name in ("SEED3_FIRST_DAY.md", "PAR009_GATE0.md"):
        shutil.copyfile(PROJECT / "docs" / name, kit / name)
    rollback = kit / "rollback"
    rollback.mkdir(exist_ok=True)
    shutil.copyfile(minimal / "parable_seed3_minimal.bin", rollback / "parable_seed3_minimal.bin")
    manifest = {key: report[key] for key in ("source_commit", "source_head_commit", "source_worktree_dirty",
                                            "source_sha256", "linker_sha256", "compiler", "pins", "first_boot")}
    manifest.update(format="parable.seed3-first-boot", version=1, boot_mode="BOOT_NONE",
                    rom_dfu_address="0x08000000", software_bootloader_included=False,
                    runtime_evidence="PENDING PHYSICAL TEST", images={})
    for name, target in first_boot_targets.items():
        image = images["first_boot_" + name]
        entry = {key: image[key] for key in ("bin_sha256", "berkeley_size", "image_load_span_bytes",
            "execution_location", "static_placement_bytes", "static_region_span_bytes", "compiler_application_frames",
            "stack_top", "rat_enabled_at_startup", "unresolved_symbols")}
        entry["binary"] = f"{name}/{target}.bin"
        entry["map"] = f"{name}/{target}.map"
        entry["build_log"] = f"{name}/build.log"
        manifest["images"][name] = entry
    manifest["rollback"] = dict(binary="rollback/parable_seed3_minimal.bin", **images["minimal_internal_flash"])
    (kit / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    for name in ("measurement-disassembly.txt", "measurement-undefined.txt", "measurement-symbols.txt"):
        shutil.copyfile(output / name, kit / name)
    checksum_paths = sorted(path for path in kit.rglob("*") if path.is_file() and path.name != "SHA256SUMS")
    (kit / "SHA256SUMS").write_text("".join(hashlib.sha256(path.read_bytes()).hexdigest()
        + "  " + path.relative_to(kit).as_posix() + "\n" for path in checksum_paths))
    print(json.dumps({"images": {name: {k: image[k] for k in (
        "app_type", "execution_location", "berkeley_size", "image_load_span_bytes", "text_including_rodata_bytes",
        "included_input_rodata_bytes", "static_placement_bytes", "static_region_span_bytes")}
        for name, image in images.items()}, "rat_kernel": report["rat_kernel"], "cmos_kernel": report["cmos_kernel"], "germanium_kernel": report["germanium_kernel"],
        "internal_flash_codec_probe": report["codec_probe"]}, indent=2))
    print(f"Build-only evidence: {output}; no flashing or hardware execution performed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, default=PROJECT / "build/arm-cache")
    parser.add_argument("--output", type=Path, default=PROJECT / "build/arm")
    args = parser.parse_args()
    build(args.cache.resolve(), args.output.resolve())
