# Independent SF016 fixtures

`nominal.cir`/`D1-open.cir` are literal runnable circuits. `netlists/` contains
all80 DC and48 transient decks with built-in ngspice44.2 devices, no native
output/stamps. `blender_spice_reference.py` regenerates/checks all DC/metadata/
binary hashes. Last fresh replay matched all three byte hashes.

SF016F32:8-byte magic, uint32 little-endian row count/50 columns, then each row
uint32 case/frame plus50 IEEE754 binary32 values. `spice-manifest.json` names
every column, records quantization/mesh/receipts. Node reference labels are not
capacitor designators: c1 means Q1 collector. Cases supply scientific sample rate.
Use whole-history checkpoints plus dense cycles; not continuous audio RMS. Float
rounding peaks below0.5uV; native gates use the original explicit tolerances.

Raw Gate0 candidate JSON retains the original196Hz fit columns. Only held196Hz
rows' h1/h2 are actual fundamental/octave metrics; non-held pluck/chord rows use
RMS/waveform behavior, not those off-frequency fits. D2-open is research-only;
the only native/audition fault is D1-open. No original drawings are redistributed.

Current verdict is FAILED:21 field checks in two8kHz cases miss.46 other transient
conditions and80 DC conditions pass. Finer0.1/0.05us8k export confirms the reference;
no tolerance was changed. Do not refresh data or reinterpret that as acceptance.
