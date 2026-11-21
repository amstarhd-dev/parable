# Actual linked firmware sections

All15 images use the unchanged official BOOT_NONE128KiB layout; code executes from internal flash. Data/BSS use AXI SRAM at0x24000000, DMA SRAM at0x30000000, and explicitly reserved SDRAM at0xc0000000. DTCM static allocation is zero in these images. Heap/stack high-water is not measured. Images have not been booted.

| Profile | Flash binary B | AXI data+BSS B | DMA B | SDRAM reserved B | Execution |
|---|---:|---:|---:|---:|---|
| rat | 104360 | 42132 | 16384 | 0 | foreground kernel |
| rat_fir97 | 104376 | 41268 | 16384 | 0 | foreground kernel |
| rat_fir97_fp32 | 104208 | 41196 | 16384 | 0 | foreground kernel |
| rat_1x_cost_only | 104376 | 42132 | 16384 | 0 | foreground kernel |
| cmos | 94504 | 33508 | 16384 | 0 | foreground kernel |
| germanium | 97168 | 33860 | 16384 | 0 | foreground kernel |
| muff | 97176 | 40212 | 16384 | 0 | foreground kernel |
| ts808 | 107208 | 39484 | 16384 | 0 | foreground kernel |
| klon | 99976 | 43188 | 16384 | 0 | foreground kernel |
| blender | 109400 | 51260 | 16384 | 0 | foreground kernel |
| fz1 | 97952 | 36516 | 16384 | 0 | foreground kernel |
| tape_saturation | 101064 | 43260 | 16384 | 0 | foreground kernel |
| spring | 95592 | 50116 | 16384 | 0 | foreground kernel |
| leo_hall | 106640 | 31380 | 16384 | 475480 | foreground kernel |
| chain_scorpio_gemini_rooms_0 | 109592 | 45356 | 16384 | 866304 | foreground kernel |

Backup SRAM is12B per image. All exact section addresses/sizes, binary/ELF hashes, source pins, compiler/SDK identities and selected controls are in artifacts.json and the ZIP. The ZIP includes original ELF.gz, map, symbols, compiler-frame records and build logs. The g0 application flag bounds compiler-debug output; the pinned SDK archive retains its original debug information. No production build flag changes.
