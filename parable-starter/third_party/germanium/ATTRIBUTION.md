# SF003 source and method attribution

`src/germanium_device.h` narrowly adapts the `bjt` internal Gummel–Poon
current/Jacobian implementation in ACME.jl, `src/elements.jl`, commit
`3832ccafb952ee2ca0a27286c091d613062c23fd`. Copyright2015,2016,2017 Martin Holters;
MIT Expat notice in ACME-LICENSE.md. Changes: fixed measured AC128 factors,
transport-Is convention,25.5mV thermal voltage, positive PNP junction differences,
independently checked derivatives with the appropriate leakage ideality factors,
and a bounded numerical evaluation domain. No Julia/framework/dependencies copied.

The parameter transcription in Greg Whitehead's `germaniumbjts.lib`, commit
`7838a40f939eaee39b37afef1dac031ef45c276a`, is MIT copyright2020 Greg Whitehead;
retain WHITEHEAD-LICENSE.md for the narrow adaptation/corroboration. Whole SPICE
model/network, plots and recordings are not imported. Native code and independent
behavioral stamps explicitly express the chosen model; no portable meaning is
assumed for that source's VT/CCB/CEB model-card keywords.

Scientific credit: Ben Holmes, Martin Holters and Maarten van Walstijn,
“Comparison of Germanium Bipolar Junction Transistor Models for Real-Time Circuit
Simulation,” DAFx2017, Table1 and equations1,4–7, available at
<https://www.dafx17.eca.ed.ac.uk/papers/DAFx17_paper_28.pdf>. Parameters originate
in their measured vintage sample, not new Parable measurements and not all AC128
or NKT275 parts. Its modified circuit is distinct from our selected classic nominal
network. No figures, text passages, MATLAB listing or recordings redistributed.

Other candidates are reference/audit only: no GPL/AGPL RT-WDF source, ElectroSmash/
Tayda schematic graphics, Overton MATLAB/LUT or third-party audio is imported.
Non-Git `revision` fields are explicitly content SHA-1, with SHA-256 as well,
not fabricated Git commits. Failed/unretrieved sources are disclosed in the report.

Remaining implementation/tools are Parable-local. Global project licensing remains
unspecified; these notices do not license the rest of Parable. Audio uses the
existing owned synthetic excitation or user-provided files, never copied recordings.
