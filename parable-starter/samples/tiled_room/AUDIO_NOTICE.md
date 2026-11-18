# Recording and implementation credits

Guitar-TECHS, ICASSP2025: Hegel Emmanuel Pedroza Villalobos, Termeh Taheri,
Wallace Abreu, Ryan M. Corey and Iran R. Roman.
https://zenodo.org/records/14963133 ; https://guitar-techs.github.io/ .
CC BY4.0: https://creativecommons.org/licenses/by/4.0/ . No endorsement implied.

Same Player3 directinput_03.wav used in SF023/024, original SHA256
84980b5468f3315171e4e546a1d38d54e4c368ccb5fee72c759e0915f346fad0.
Use channel0,9.28–29.28s;24bit→16bit, no resample/time stretch/fake notes;
input-receipt.json and prepared source preserve source identity. Original archive
P3_music.zip SHA033489e22600751fb5a1633e7d856b901c6782e0486fa02135e830780d9dbfe2.
Changes: calibrate to0.2 software peak, append3s tail, process through this native
room. Raw fullmix .65dry/.35wet; wet-only separately labeled. Matched copies use
reported constant scalar gains to−23LUFS; no loudnorm/limiter/compressor. Wet solos
use their fullmix listening gain. Stereo copies have separately measured gain.

Borrowed code: Mike Jarmy dsp-lib, MIT, exact pin and complete notice under
../../third_party/tiled_room. No proprietary Lexicon/Eos code, IR or recordings
included. This is an independent character study, no PCM70 sonic identity claim.
Preserve attribution/source/license/change notices when sharing these recordings.

Final fault changes exactly the right second tank-allpass recirculation coefficient
from default.5 to.94; no oscillator, extra delay line or separate resonator layer.
Original Room A comparison is existing Parable SF020, aperture0, with a disclosed
offline wet-RMS calibration. Existing source-code notices remain in third_party.
