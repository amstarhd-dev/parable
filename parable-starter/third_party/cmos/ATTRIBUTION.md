# SF-002 source and method attribution

`src/cmos_device.h` narrowly adapts the `mosfet` current/Jacobian implementation
from ACME.jl, commit `3832ccafb952ee2ca0a27286c091d613062c23fd`,
`src/elements.jl`. MIT Expat notice is preserved in ACME-LICENSE.md. Julia, the
general solver and dependencies are not imported. Adaptations: fixed published
CD4049 fit functions, positive PMOS magnitude convention, parameter freezing
outside the measured gate-voltage interval and a small native return type.

Scientific reference: Lasse Köper and Martin Holters, “Taming the Red Llama—Modeling
a CMOS-Based Overdrive Circuit,” DAFx2020, equations 9/17/18, Tables 1/3 and Figure
13. [Original article](https://dafx2020.mdw.ac.at/proceedings/papers/DAFx2020_paper_21.pdf)
states [CC-BY-3.0](https://creativecommons.org/licenses/by/3.0/). Authors and source
are credited for the reported equations, factual component values and measured-fit
data. No figures, text passages or recordings are redistributed. The present
fixed-rail reduced circuit, discretization, extrapolation and fault experiments
are changes; they do not inherit the paper's physical-unit or listening results.

All other evaluated sources in provenance.json are audit/reference-only. In
particular, no AGPL Obsidian code/fit, TI model, Tonepad layout or unlicensed
supplement listing is copied/adapted into Parable. Non-Git references use a
content SHA-1 in `revision` (explicitly labeled), with a SHA-256 of the inspected
bytes as well; these are not fabricated Git revisions. No remote check or legal
clearance is implied by the offline registry validator.

The remaining source is Parable-local; the repository still has no global
project LICENSE. These notices do not assign a license to the rest of Parable.
