# Codex Assignment — SF-023
## The Wallace Experiment: Serial Effects vs. Parallel Mixing Console

**Repository:** `amstarhd-dev/parable`  
**Type:** Experimental host DSP / listening study  
**Disposition:** New branch, separate PR, unmerged.

### Objective

Determine whether parallel signal routing, frequency-selective processing, and dynamic wet-effect ducking can produce a substantially more articulate, powerful, and spacious guitar tone than conventional serial effects.

Use exactly four existing Parable processors:

1. **Big Muff** — SF-009, merged on main.
2. **Rotary speaker** — SF-007, merged on main.
3. **Tape echo** — SF-006, merged on main.
4. **Grand Hall** — SF-022, experimental branch `sf-022`, PR #77.

Do not develop new versions of these effects. Do not substitute generic placeholder algorithms.

This experiment tests **the mixing architecture**, not the quality of newly invented effects.

### Phase 1 — Assemble the laboratory

Create a standalone host-only renderer using the four actual DSP implementations.

Import the existing hall implementation from its isolated experimental branch without merging that PR or modifying its source.

Ensure each algorithm's required sample rate, block size, buffers, and lifecycle constraints are respected.

Use the established permitted direct-input guitar recording. Render at 48 kHz, mono output initially. Keep the original source and settings identical across comparisons.

Include adequate decay tails.

### Phase 2 — Three configurations

**A. Conventional serial pedalboard**

```text
Guitar
  ↓
Big Muff
  ↓
Rotary
  ↓
Tape Echo
  ↓
Grand Hall
  ↓
Output
```

This is our control.

Choose sensible musical settings, not deliberately terrible ones. Keep the wet/dry mix moderate enough that the serial configuration remains credible.

**B. Parallel console — static**

```text
                  ┌─ Dry/attack ─────────┐
                  │                      │
Guitar ─ Big Muff ┼─ Rotary ─────────────┤
                  │                      │
                  ├─ Tape Echo ──────────┼─ MIX → Output
                  │                      │
                  └─ Grand Hall ─────────┘
```

Use independently adjustable send/return gains.

Apply appropriate high-pass filtering to the delay and reverb branches, and complementary EQ only where it demonstrably helps.

Keep a clear driven guitar core. The dry/attack branch is optional and independently adjustable; do not assume that clean guitar blended with fuzz must always improve the result.

Do not inadvertently double-count the direct signal embedded in wet effects. Account for latency and phase alignment.

**C. Parallel console — dynamic**

Use exactly the same routing and initial gains as B, adding:

- Guitar-input envelope follower.
- Automatic ducking of delay and reverb returns during strong playing.
- Configurable attack and release.
- A 0–6 dB ducking range, initially testing approximately 4 dB.
- Recovery as the played signal decays, allowing ambience to expand between notes.

The modulation branch should remain independently adjustable.

Do not add an automatic mastering compressor, limiter for artistic loudness, or other processing that obscures the comparison.

### Phase 3 — Controlled listening

Generate these comparisons:

| Recording | Configuration |
|---|---|
| 01 | Dry guitar reference |
| 02 | Big Muff alone |
| 03 | A — Serial chain |
| 04 | B — Parallel, static |
| 05 | C — Parallel, dynamic |
| 06 | B — Wet returns isolated |
| 07 | C — Wet returns isolated |
| 08 | C — No spectral carving |
| 09 | C — No ducking |

Deliver the three principal mixes at both identical output gain and matched perceived loudness, documenting the matching procedure.

Use one common input performance.

Include at least one aggressive rhythmic passage with gaps between chords and one sustained passage that allows the rotary, tape, and hall to develop.

If the existing permitted sample lacks useful gaps or sustain, select an additional appropriately licensed recording. Preserve complete source attribution.

### Phase 4 — What actually improves?

Measure and document:

- Peak level, RMS, and integrated loudness where meaningful.
- Spectral balance and low-mid buildup.
- Wet-return gain reduction versus time.
- Decay audibility between notes.
- Latency and any relevant phase cancellation.
- Full-graph CPU time and persistent memory, including all four effects.

Do not confuse measurable differences with musical superiority.

The founder will judge musical quality by listening.

### Phase 5 — Resource feasibility

The experiment may run entirely on the host initially.

Record the incremental cost of routing, filters, gain controls, and envelope followers separately from the four existing DSP engines.

Determine whether the mixing infrastructure is plausibly inexpensive enough for Daisy.

Do not claim that the entire four-effect graph fits or meets the audio deadline without corresponding target evidence.

If the full graph is too expensive, identify the actual bottleneck rather than optimizing everything indiscriminately.

### Hard boundaries

- No changes to canonical correspondence rules.
- No changes to the internal DSP algorithms.
- No automatic merging of outstanding PRs.
- No new general-purpose audio framework unless a minimal host graph is demonstrably insufficient.
- No extensive SPICE verification or redundant algorithm-level test campaigns.
- Reuse existing waveform, PCM, measurement, and test utilities.
- Keep new validation proportional to the changes.
- Do not spend the experiment building a UI.

### Success criterion

Answer one question with actual guitar audio:

**Does the same set of four effects become significantly more musical when treated as a miniature mixing console rather than a serial pedalboard?**

Deliver reproducible recordings and focused technical evidence.

If the parallel approach provides little benefit—or sounds worse—report that plainly.

**Priority: listening evidence first, implementation simplicity second, resource measurement third.**

Open an independent PR with the audio files, exact configuration values, implementation, and conclusions. Leave unmerged.

Do not proceed into planetary correspondence integration. That decision follows the listening experiment.