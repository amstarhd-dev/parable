"""Generate stopped-reference traces for the reduced Muff fit."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import subprocess
import sys

import numpy as np

reference, training, guitar, destination = sys.argv[1:]
out = Path(destination)
out.mkdir(exist_ok=True)
train = np.fromfile(training, "<f4")
di = np.fromfile(guitar, "<f4")
# Separate split by source material, with lower and higher pick excursions.
train = np.r_[train[:48000] * .4, train[48000:] * .18].astype("<f4")
test = np.r_[di[96000:120000] * .3, di[120000:144000]].astype("<f4")
train.tofile(out / "train.f32")
test.tofile(out / "test.f32")

def capture(job):
    split, fault, sustain, tone, volume = job
    audio = train if split == "train" else test
    proc = subprocess.run([reference, str(sustain), str(tone), str(volume), str(fault)],
                          input=audio.tobytes(), capture_output=True, check=True)
    values = np.frombuffer(proc.stdout, "<f4").reshape(-1, 2)
    assert values.shape == (len(audio), 2) and np.isfinite(values).all()
    values[:, 1].tofile(out / f"{split}-{fault}-{sustain}-{tone}-{volume}.f32")
    return job

jobs = [(split, fault, sustain, tone, volume)
        for fault in range(2)
        for split, settings in [
            ("train", [(s, .5, .5) for s in (0., .25, .5, .75, 1.)]),
            ("test", [(s, t, v) for s in (.125, .625, .875)
                                  for t, v in ((.5, .5), (.15, .5), (.85, .5), (.5, .8))])]
        for sustain, tone, volume in settings]
with ThreadPoolExecutor(8) as pool:
    for job in pool.map(capture, jobs):
        print(job, flush=True)
