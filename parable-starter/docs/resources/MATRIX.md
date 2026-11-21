# Complete resource matrix

47 measured profiles; 1738 input/control scenarios; 48kHz/48-frame routines. **No effect admitted.**

Host timing column is the largest case median in microseconds/frame; wall-clock measurements on the recorded host CPU, never M7 utilization. Memory is requested live/reprepare-peak host bytes. ARM text excludes the probe; ARM instance includes the laboratory adapter. Frame is the largest individual compiler frame, including stopped preparation, not total stack.

| Profile | Source | Host µs/frame | Host live / reprepare peak B | ARM instance / text B | Max function frame B | Hold |
|---|---|---:|---:|---:|---:|---|
| rat | main | 0.516 | 10760 / 10760 | 10752 / 6412 | 424 | target/audio pending |
| cmos | main | 1.433 | 2136 / 2136 | 2128 / 5580 | 344 | native solver reported failures in recorded stress cases; retain separate numerical hold |
| germanium | main | 5.399 | 2488 / 2488 | 2480 / 7080 | 2664 | target/audio pending |
| muff | main | 12.298 | 8832 / 8832 | 8832 / 7032 | 9328 | target/audio pending |
| ts808 | main | 37.681 | 8104 / 8104 | 8104 / 9092 | 8432 | target/audio pending |
| klon | main | 16.939 | 11808 / 11808 | 11808 / 9784 | 12184 | target/audio pending |
| wavefolder | main | 0.584 | 9704 / 9704 | 9704 / 3892 | 9936 | target/audio pending |
| scorpio | main | 0.603 | 9224 / 9224 | 9224 / 7928 | 10888 | target/audio pending |
| libra | PR70 | 0.337 | 968 / 968 | 968 / 2856 | 1120 | target/audio pending |
| ds1 | PR69 | 1.374 | 8832 / 8832 | 8832 / 11448 | 11296 | target/audio pending |
| fz1 | PR61 | 18.700 | 5136 / 5136 | 5136 / 7696 | 5456 | target/audio pending |
| blender | PR63 | 110.281 | 19880 / 19880 | 19880 / 10656 | 20944 | Existing high-note scientific gates fail; native solver reported failures in recorded stress cases; retain separate numerical hold |
| tape_saturation | PR56 | 19.263 | 11880 / 11880 | 11880 / 6020 | 20216 | target/audio pending |
| tape_medium | main | 0.032 | 230552 / 230552 | 128 / 1296 | 56 | target/audio pending |
| rotary | main | 0.104 | 4392 / 4392 | 4392 / 3144 | 80 | target/audio pending |
| spring | PR48 | 2.387 | 18736 / 18736 | 18736 / 3364 | 152 | target/audio pending |
| taurus_plate | PR72 | 0.219 | 148224 / 294824 | None / None | None | Pinned SDK ARM compile not passed; see raw build failure |
| gemini_rooms | PR74 | 0.040 | 43596 / 43596 | 212 / 1588 | 128 | target/audio pending |
| cancer_cloud | PR76 | 1.022 | 67028776 / 134070296 | 1584 / 18346 | 1360 | active-plus-prepare memory exceeds64MiB in host representation; target placement/lifetime requires redesign; 67011904-byte core near64MiB limit; reprepare peak exceedsSDRAM |
| leo_hall | PR77 | 0.382 | 475480 / 475480 | 475480 / 9396 | 408 | target/audio pending |
| virgo_tiled | PR80 | 0.170 | 48900 / 48900 | 340 / 2588 | 88 | target/audio pending |
| generic_drive_prototype | PR11 | 0.376 | 5920 / 5920 | 5912 / 2556 | 96 | Unmerged PAR006 prototype; superseded as general drive direction |
| gemini_cmos_candidate | PR42 | 1.616 | 2160 / 2160 | 2152 / 5532 | 344 | native solver reported failures in recorded stress cases; retain separate numerical hold |
| rat_pr81_reference | PR81 | 0.508 | 10760 / 10760 | 10752 / 6412 | 424 | target/audio pending |
| rat_fir97 | PR81 | 0.370 | 9896 / 9896 | 9888 / 6416 | 424 | target/audio pending |
| rat_fir97_fp32 | PR81 | 0.351 | 9824 / 9824 | 9816 / 6232 | 424 | target/audio pending |
| rat_1x_cost_only | PR81 | 0.050 | 10760 / 10760 | 10752 / 6412 | 424 | CPU-only lower bound; anti-aliasing removed, never a fidelity candidate |
| germanium_libm | PR21 | 5.313 | 2488 / 2488 | 2480 / 7080 | 2664 | target/audio pending |
| germanium_hermite32 | PR21 | 6.098 | 2488 / 2488 | 2480 / 9792 | 2664 | target/audio pending |
| germanium_hermite64 | PR21 | 6.129 | 2488 / 2488 | 2480 / 11840 | 2664 | target/audio pending |
| germanium_polynomial8 | PR21 | 5.443 | 2488 / 2488 | 2480 / 7644 | 2664 | target/audio pending |
| germanium_polynomial12 | PR21 | 6.076 | 2488 / 2488 | 2480 / 7676 | 2664 | target/audio pending |
| bypass | main | 0.008 | 11056 / 11056 | 11048 / 6412 | 424 | target/audio pending |
| gain | main | 0.010 | 11056 / 11056 | 11048 / 6412 | 424 | target/audio pending |
| lowpass | main | 0.015 | 11056 / 11056 | 11048 / 6412 | 424 | target/audio pending |
| chain_scorpio_gemini_rooms_0 | pinned mixed chain | 0.696 | 287772 / 287772 | 13968 / 13956 | 10888 | target/audio pending |
| chain_scorpio_gemini_rooms_1 | pinned mixed chain | 0.737 | 287772 / 287772 | 13968 / 13956 | 10888 | target/audio pending |
| chain_scorpio_gemini_rooms_2 | pinned mixed chain | 0.729 | 287772 / 287772 | 13968 / 13956 | 10888 | target/audio pending |
| chain_wavefolder_virgo_tiled_0 | pinned mixed chain | 0.673 | 293556 / 293556 | 14576 / 10920 | 9936 | target/audio pending |
| chain_wavefolder_virgo_tiled_1 | pinned mixed chain | 0.885 | 293556 / 293556 | 14576 / 10920 | 9936 | target/audio pending |
| chain_wavefolder_virgo_tiled_2 | pinned mixed chain | 0.672 | 293556 / 293556 | 14576 / 10920 | 9936 | target/audio pending |
| chain_rat_leo_hall_0 | pinned mixed chain | 0.867 | 721192 / 721192 | 490760 / 20248 | 424 | target/audio pending |
| chain_rat_leo_hall_1 | pinned mixed chain | 0.873 | 721192 / 721192 | 490760 / 20248 | 424 | target/audio pending |
| chain_rat_leo_hall_2 | pinned mixed chain | 0.873 | 721192 / 721192 | 490760 / 20248 | 424 | target/audio pending |
| chain_muff_virgo_tiled_0 | pinned mixed chain | 13.011 | 292684 / 292684 | 13704 / 14060 | 9328 | target/audio pending |
| chain_muff_virgo_tiled_1 | pinned mixed chain | 12.518 | 292684 / 292684 | 13704 / 14060 | 9328 | target/audio pending |
| chain_muff_virgo_tiled_2 | pinned mixed chain | 12.557 | 292684 / 292684 | 13704 / 14060 | 9328 | target/audio pending |

All raw scenarios, startup/reprepare costs, output hashes, counter values, callback quantiles, compiler frames, math dependencies and source/command fingerprints are in the compressed host/ARM ledgers beside this table. Native source is unchanged.

Observed solver failure counts remain visible even where the output is finite. A successful infrastructure test does not certify those models.
