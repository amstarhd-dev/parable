# Codex Assignment — SF-024
## The Wallace Experiment II: Parallel Guitar Architecture + Cabinet Simulation

**Repository:** `amstarhd-dev/parable`  
**Starting point:** SF-023 / PR #78  
**Status:** Standalone host experiment. Open an unmerged PR.

### Objective

Determine whether a proper guitar speaker/cabinet simulation and a pre-distortion signal split can transform our existing Big Muff, rotary, tape echo, and grand hall into a convincing, articulate, record-quality guitar sound.

The previous experiment used the Big Muff as the source for every branch. Its clean branch existed but was disabled.

Correct that experimental limitation and introduce a physically credible guitar cabinet stage.

**Do not modify the existing effects algorithms.**

### 1. Cabinet simulation: borrow real acoustic measurements

Find at least two suitable, legitimately redistributable guitar cabinet impulse responses, preferably representing contrasting speaker/cabinet configurations.

Candidates might include:

- A closed-back 4×12 guitar cabinet.
- An open-back 1×12 or 2×12 guitar cabinet.

Prefer documented microphone, speaker, and measurement details.

Check exact licensing and permitted redistribution before importing anything. Do not assume that a publicly downloadable commercial IR is open source.

Implement a modest, efficient convolution processor, borrowing an established implementation if appropriate.

The IRs should capture actual guitar speaker/microphone coloration—not merely approximate a cabinet with generic low-pass filtering.

Use a simple mono convolution path initially. Measure memory and processor cost.

### 2. Establish the cabinet's contribution independently

Before introducing parallel routing, render:

**A.** Big Muff direct, as previously heard.

**B.** Same Big Muff through Cabinet IR 1.

**C.** Same Big Muff through Cabinet IR 2.

**D.** Same Big Muff through Cabinet IR 1 with a modest, independently controllable amplifier stage, if an existing credible implementation is readily available.

Do not build a new physical amplifier simulator for D. Omit it if no suitable implementation is available.

These comparisons must use identical input, Muff settings, and loudness-matching methodology.

We want to hear what the cabinet itself contributes.

### 3. Rebuild the mixing console with a pre-Muff split

Use the actual four processors already established in SF-023.

```text
GUITAR INPUT
     │
     ├── BIG MUFF ──┐
     │              │
     ├── CLEAN ─────┤
     │              │
     ├── ROTARY ────┤
     │              │
     ├── TAPE ECHO ─┤
     │              │
     └── GRAND HALL ┤
                    │
              MIXING CONSOLE
                    │
                 CAB IR
                    │
                  OUTPUT
```

For this first configuration, the cabinet is downstream of the complete mix, approximating a single guitar amplifier/speaker system.

The pre-Muff split must preserve an independent clean branch. All other branches must receive clean guitar unless explicitly configured otherwise.

Use true wet-only returns for delay/reverb where appropriate. Avoid accidentally multiplying the direct signal.

Provide independently adjustable levels and basic high-pass filtering.

Compensate for fixed convolution latency and measure relevant phase interactions.

### 4. Explore the alternative: post-cabinet ambience

A conventional guitar recording often places the microphone/cabinet sound before studio delay and reverb.

Therefore test a second topology:

```text
                 ┌── Muff ───┐
Guitar ──────────┤           ├── Mix
                 └── Clean ──┘    │
                                  ▼
                              Cabinet IR
                                  │
                         ┌────────┼────────┐
                         │        │        │
                       Rotary    Tape     Hall
                         │        │        │
                         └────────┼────────┘
                                  ▼
                             Final Mix
```

The rotary placement here is deliberately a studio-effect experiment, not a physical Leslie-cabinet reconstruction.

Compare the pre-cabinet and post-cabinet placement of the ambient effects.

**Do not assume one architecture is universally superior.**

### 5. Listening deliverables

Use the identical licensed guitar performance from SF-023 for direct comparison.

Render:

1. Original Muff direct.
2. Muff + Cabinet 1.
3. Muff + Cabinet 2.
4. Original SF-023 serial mix.
5. New pre-Muff parallel mix + Cabinet 1.
6. Same as 5, clean branch disabled.
7. Same as 5, reduced Muff level and increased clean contribution.
8. Post-cabinet studio-effects configuration.
9. Same as 8, with restrained delay/reverb ducking.

Provide matched-loudness WAVs and document any separate raw-gain comparisons.

Keep all selected sound settings fixed except the dimension explicitly being tested.

For the main comparisons, avoid excessive or theatrical ambient levels. Afterward, include one deliberately expansive setting to demonstrate what the architecture can do under challenging conditions.

### 6. Sound targets

We are after:

- An aggressive, recognizable fuzz character.
- Clear pick attack and intelligible chords.
- A controllable distinction between clean and saturated components.
- Spaciousness that does not bury the central guitar.
- A convincing guitar-speaker spectral profile.
- A sound that could plausibly belong in a finished rock recording.

The references are broad production aesthetics associated with Nirvana, Andy Wallace's guitar mixes, and layered rock production—not an instruction to copy copyrighted recordings or pretend to reproduce a particular engineer's undocumented signal chain.

### 7. Technical constraints

- Keep the original Muff, Rotary, Tape, and Hall DSP implementations unchanged.
- Do not merge PR #77 or #78 into main.
- Do not expand the canonical Parable schema, firmware, or correspondence grammar.
- Use existing audio and measurement utilities.
- Measure convolution memory/CPU overhead.
- Maintain a deterministic offline renderer.
- Do not introduce a new generic mixing framework.
- Do not optimize the Muff's nonlinear solver in this assignment.
- No exhaustive revalidation of unrelated electrical models.

The cabinet IR should be implemented as a reusable, independent processor so it can be evaluated for hardware admission later.

Host success does not imply Daisy readiness.

### 8. Decision criteria

We need three separate answers:

**Question 1:** Does a real cabinet IR substantially improve the perceived authenticity and usefulness of our Big Muff?

**Question 2:** Does splitting the clean guitar before the Muff preserve articulation and allow a more convincing composite sound?

**Question 3:** Is it musically preferable to place the spatial effects before the simulated cabinet or after it?

Provide direct listening evidence for each question.

If an improvement is subtle or absent, report it without embellishment.

Do not equate numerical spectral differences with musical quality.

### Final instruction

Deliver the listening files as the primary output, followed by concise source, licensing, reproducibility, and resource evidence.

No new effect development, unrelated refactoring, or sprawling validation campaigns.

**We are making records now, not proving transistor equations again.**

Open one experimental PR and leave it unmerged.