# Observed SF-022 evidence

Baseline clean current-main Release40/40 before editing; final complete Release43/43.
Debug6/6 focused; complete old Debug run intentionally interrupted after31 finished
without failures. Poisoned ASan+UBSan3/3. Exact logs retain compiler warnings and
partial run; no partial run is called a full pass. Release/generic test elapsed
seconds reflect concurrent workloads; only `performance.json` uses sequential
quiet benchmark passes. No current GitHub CI success inferred from these files.

`arm/report.json` records exact executed compiler/make argv, SDK/submodule pin,
archive/linker hashes, actual object/full-image sections, static frames and limits.
Gzipped map/symbol/disassembly are lossless. ELF remains local and is reproducible;
CI's existing ARM artifact publishes newly linked output. Nothing was flashed.

`metrics.json`: actual original/native impulse analysis +independent5x5 SVD;
`decay-rates.json`: corrected actual native3rate transient captures.
`rejected-phase0-noise-floor-decay.json` is an **invalid initial96k decay estimate**:
phase0-only impulse was dropped by upstream input subsampling. Its96k numbers
measure numerical noise accumulation, not the hall. Do not use them as RT60.
The corrected probe drives both phases, preserving original engine behavior.

Exact optional rate analysis, with NumPy2.3.5 after compiling/running hall_response
as in SF022_REPORT:

```bash
python3 - <<'PY'
import numpy as np
from pathlib import Path
for p in sorted(Path('build/hall-response-data').glob('*.f32')):
    rate,decay=map(int,p.stem.split('-'))
    x=np.fromfile(p,dtype='<f4').astype(float)
    e=np.cumsum((x*x)[::-1])[::-1]
    db=10*np.log10(np.maximum(e,1e-300)/e[0])
    i=np.flatnonzero((db<=-5)&(db>=-25))
    slope=np.polyfit(i/rate,db[i],1)[0]
    print(rate,decay,-60/slope)
PY
```

`preservation.json`: all452 pre-existing source/header/firmware/fixture hashes,
not a refreshed expected sound baseline. Old LabRAT sixteen-file preservation test
also passes in complete Release. Cancer worktree unchanged at249ff079, clean.
Audio/source identities and package hashes live beside downloadable recordings.
