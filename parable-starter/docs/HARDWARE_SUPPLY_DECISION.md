# Parable hardware supply decision — 2026-11-06

## Decision

Prototype on **Daisy Seed3 65 MB** with the official pedal development kit, then build a Parable carrier PCB around the module. Do **not** design a clone of the Seed module before April 2027. Keep the portable C++ DSP and patch contract decoupled from the board support package; preserve a backup option to migrate to Seed2 DFM or a later fully integrated PCB after real demand and component quotes.

## Evidence, with limitations

1. Manufacturer announces Seed3 July 2026, $29.99 for 1 MB and **$34.99 for 65 MB** in the selected US storefront (currently showing out of stock/restock en route), pin-compatible with older Seed and backward-compatible firmware. https://community.daisy.audio/t/seed3-is-here/9440 ; https://daisy.audio/products/seed3
2. Manufacturer inventory snapshot shows regional distribution is uneven; do not rely on a general "In stock" label. The stock page's ETA fields include old dates and are not reliable delivery promises. https://daisy.audio/pages/seed3-global-inventory
3. Manufacturer responded in July 2026 that the original Seed and Pod will remain available, and indicated commercial-oriented form factors are intended to remain stable. https://community.daisy.audio/t/stock-update-new-stuff-coming/9295
4. Seed2 DFM is explicitly described as a mass-production-oriented alternative, with differential audio, 64 MB SDRAM, more GPIO, and STM32H7-class DSP. It is not mechanically drop-in compatible with Seed3; migration requires its own carrier/interface check. https://docs.daisy.audio/hardware/Seed2-DFM/
5. Seed3 pedal dev kit has open-source schematics, KiCad design and a BOM. These are for the **carrier**, not proof that every file for reproducing the Seed3 module is provided. License terms vary by repository; inspect exact files before incorporating. https://docs.daisy.audio/product/Seed3-Pedal-Dev-Kit/
6. Electronics supply itself is nontrivial: individual distributors report widely different inventory for STM32H750IBK6 (Mouser 0 and long lead, DigiKey some stock, LCSC more) and TI TAC5242 (DigiKey temporarily 0, Mouser shows stock). A custom module still relies on those supply chains. Manufacturer build availability may differ from distributor snippets. https://www.mouser.com/en/ProductDetail/STMicroelectronics/STM32H750IBK6 ; https://www.digikey.com/en/products/detail/stmicroelectronics/STM32H750IBK6/9453370 ; https://www.ti.com/product/TAC5242

## Supply risk controls

- Request written lead-time / volume / support terms directly from Daisy for 10, 30, 100 and 300 pieces; confirm 65 MB SKU and future availability, rather than interpreting a storefront quantity as a commercial commitment.
- Purchase 2–3 65 MB modules now for development once verified available. Establish a batch-reserve policy only after a realistic pilot production schedule.
- Put the module on the carrier with serviceable connections in early builds; avoid a costly redesign before the hardware performance test is complete.
- Keep board IO / switch hardware and control transport separate from compiled audio algorithms.
- Add one alternate processor-module feasibility review after hardware timing, analog noise and available memory are measured.
- Do not assume component clone economics improve until assembled-module cost, RF/EMC and signal integrity, QA and yield have been quoted.

## Hardware gates

1. Native audio callback on Seed3: compile, flash, no underruns, line/instrument input electronics, latency measurement.
2. Carrier proof: proper input buffer/pickup loading; safe bypass; 9 V supply conversion; control scans; output protection; measured noise floor.
3. Four nonvolatile patches and Register/Volume operations without phone or internet.
4. Phone/ESP32 transport proof only after reliable offline use.
5. Pilot supply purchase contingent on measured hardware pass and written delivery agreement.

## Frozen scope / future composer support

Release-track: chart compiler, manual Workshop editor, one patch format, four hardware patches, a limited **high quality** effect selection. Later input grammars (biometric and creature/type-mapping experiments) consume the same patch contract; they should not add separate runtime effects engines before pilot. Any commercial use of third-party creature names/art must receive IP clearance; do not presume renaming alone solves it. A creature-battle mode is not in the pilot scope.

## Independent core status

The `Parable_Core_Brick_0.1` folder is native C++ with host-side tests and an **unverified** libDaisy audio callback adapter. It is a deliberate plumbing proof, not a finished SDK, finished guitar effect, or evidence of hardware sound quality.
