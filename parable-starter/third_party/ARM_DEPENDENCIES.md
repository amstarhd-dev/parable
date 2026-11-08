# ARM build dependencies and notices (PAR-005)

Exact source/toolchain/download pins are in
[`firmware/dependencies.json`](../firmware/dependencies.json). No branch tip or
unpinned master is used by the reproducible build. No DaisySP/effect library added.

| Component | Version/source identity | License / authoritative notice |
| --- | --- | --- |
| libDaisy | `1289a000bc29dae1bc0ed4129cfa2df40ef71574`, daisyaudio/libDaisy | MIT, root `LICENSE`. File-specific notices still apply. |
| CMSIS-DSP | `3a04f817a4380c00ff51f9bcdc6c4e4bb7a18b80` | Apache-2.0, `Drivers/CMSIS-DSP/LICENSE.txt`. |
| CMSIS 5 | `2b7495b8535bdcb306dac29b9ded4cfb679d7e5c` | Apache-2.0, `Drivers/CMSIS_5/LICENSE.txt`. |
| STM32H7 CMSIS device | `6dac8c24d7b38ab20806d27dd7d8285a6433b8f7` | Apache-2.0, submodule `LICENSE.md`. |
| STM32H7 HAL | `404a70d4c19c124636c78801ff7c0dbc87bc4256` | BSD-3-Clause, submodule `LICENSE.md`. |
| ST USB device middleware | `7b5e6886d2f11ad15d2b46364e8a96984358f639` | ST SLA0044 Rev5, `LICENSE.md`; includes ST-device-only use and redistribution conditions. Not simply MIT. |
| Bundled ST USB host / patched drivers | Included in the pinned libDaisy tree | Per-file ST notices; consult headers in `Middlewares/ST/STM32_USB_Host_Library` and `Middlewares/Patched`. ST SLA0044/BSD notices as specified in those files. |
| Bundled FatFs | Included in pinned libDaisy tree | ChaN permissive notice in `Middlewares/Third_Party/FatFs/src/ff.h` / `ff.c`. |
| Startup | libDaisy `core/startup_stm32h750xx.c` | No separate license in that file header; root MIT notice retained. |
| Linker scripts | libDaisy `core/STM32H750IB_{flash,sram,qspi}.lds`, unmodified | Headers say "provided under the BSD license" without a variant; retain headers and upstream root notice. Clarify upstream attribution before product redistribution. |
| Daisy bootloader audit | Bundled v6.4 intdfu/2000ms binary hash and official source audit commit pinned in lock file | Official DaisyBootloader root MIT notice; ST/file-specific notices still apply. Audited, not rebuilt/deployed; loader source has a different libDaisy gitlink. No claim of Seed3 hardware support or reproducible loader binary. |
| nlohmann/json | 3.11.3, same archive/hash as host CMake | MIT; existing [JSON dependency record](README.md). Used only by the control-side diagnostic images/probe. |
| xPack GNU Arm Embedded GCC | 14.2.1-1.1, `arm-none-eabi-g++` 14.2.1 (20241119) | Build tools include GPL licenses; GCC runtime libraries use their runtime exception, newlib has component permissive notices. Full distribution notices: extracted `distro-info/licenses/` (including COPYING.NEWLIB and GCC runtime exceptions). This is a build-tool/runtime dependency, not GPL effect source imported into Parable. |

The xPack archive is SHA-256 verified against the release checksum, copied into
the lock file. Arm's own 14.2.rel1 download redirected to an endpoint blocked in
this execution environment (HTTP 403); the build uses the explicitly identified
xPack distribution instead of claiming it is Arm's original binary package.

The script retains dependency license files alongside build artifacts, including
the distribution's toolchain/runtime notices. Full fetched sources preserve all
file-level notices. ST middleware is used for an ST STM32H750 target; its conditions
must be retained when distributing firmware. This record is provenance for the
selected build, not a statement that every bundled file shares libDaisy's MIT license.
Unused upstream googletest is not fetched or built. Required driver submodule
commits are verified against the lock and checked for local modifications.
