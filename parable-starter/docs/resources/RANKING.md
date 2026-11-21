# Optimization priority ranking

Priority combines correctness, memory/lifetime and algorithmic risk. Host order below is a screening result, **not a Cortex-M7 speed prediction**. Physical candidate/whole-chain measurements must decide target admission.

| Engineering order | Scope | Reason / next bounded action |
|---|---|---|
| P0 | Cloud | Two prepared cores alone exceed64MiB. Prove used-buffer bounds and lifetime before an unchanged target admission. |
| P0 | Blender / CMOS stress | Preserve reported solver failures and resolve acceptance/convergence before speed promotion. |
| P0 | Plate | Resolve control-side allocation/exception policy under pinned SDK flags; current target compile fails. |
| P1 | TS808, Klon, Muff, FZ1, germanium, tape saturation | Measure supplied M7 images; identify nonlinear evaluation versus solve/filter cost. Existing mathematical candidates remain unselected. |
| P1 | RAT and common FIR family | Try exact-order contiguous/mirrored traversal before shortening filters; validate extra memory and full-processor target benefit. |
| P2 | Spring / Hall | Modal workload and signal-dependent math/64-frame bursts need specific target measurements. |
| P2 | Scorpio or Wavefolder + Rooms/Tiled + tape + rotary | Prove an early full playable combination on target while expensive reference families remain held. |

## Same-host screening order of the21 primary voices

Worst case median is the largest per-case callback median divided by48. It includes nominal/fault/high-control and adversarial inputs. It is not a worst-case bound, and the source units/control domains differ.

| Rank | Voice | Worst case median µs/frame | Reported native failure maximum |
|---:|---|---:|---:|
| 1 | blender | 110.281 | 8293 |
| 2 | ts808 | 37.681 | 0 |
| 3 | tape_saturation | 19.263 | 0 |
| 4 | fz1 | 18.700 | 0 |
| 5 | klon | 16.939 | 0 |
| 6 | muff | 12.298 | 0 |
| 7 | germanium | 5.399 | 0 |
| 8 | spring | 2.387 | 0 |
| 9 | cmos | 1.433 | 280 |
| 10 | ds1 | 1.374 | 0 |
| 11 | cancer_cloud | 1.022 | 0 |
| 12 | scorpio | 0.603 | 0 |
| 13 | wavefolder | 0.584 | 0 |
| 14 | rat | 0.516 | 0 |
| 15 | leo_hall | 0.382 | 0 |
| 16 | libra | 0.337 | 0 |
| 17 | taurus_plate | 0.219 | 0 |
| 18 | virgo_tiled | 0.170 | 0 |
| 19 | rotary | 0.104 | 0 |
| 20 | gemini_rooms | 0.040 | 0 |
| 21 | tape_medium | 0.032 | 0 |

## Shared versus specific

| Opportunity | Shared scope | Required distinction |
|---|---|---|
| FIR history traversal | RAT/CMOS/germanium and later oversampled drives | Full/symmetric coefficient layouts differ. Preserve arithmetic order, tap count, factor and latency; measure memory tradeoff. |
| Nonlinear math evaluators | Gummel–Poon/BJT/CMOS equation families | Preserve value AND derivative/domain behavior; actual Newton iteration counts can erase arithmetic savings. |
| Precision | Common measurement/differential tooling | Which state/section can use FP32 is effect-specific. M7 hardware double and software transcendental calls are separate. |
| Solver structure | Reusable measurement and matrix primitives | Topology/conditioning and failure policies differ. No speculative Jacobian caching or reduced convergence budget. |
| Delay storage/lifetimes | Shared capacity/accounting tools | Actual maximum indices, modulation, reset behavior and overlapping preparation belong to each model. |
| Parameter work | Shared stopped-preparation pattern | Hoist only proven parameter-only work. Hall sin/pow includes signal-dependent behavior. |

No savings are claimed for unimplemented proposals. No new optimization is adopted here. Existing RAT physical observations and unselected germanium experiments remain separately identified. Source-byte reuse does not let separate processor instances share mutable audio state.
