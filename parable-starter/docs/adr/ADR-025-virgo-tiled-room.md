# ADR025 — independent Tiled Room character study

Status: experimental, authorized revised SF025 brief, listening approval open.
Select MIT Mike Jarmy Dattorro figure8, not a literal bathroom IR or another new
FDN. Retain original four input allpasses, paired tank diffusion/delays, signed
stereo taps and one-pole input/tank filtering. No modulation. Fixed compact size
.30 relative to paper delays;4ms predelay; initial target decay.62s; input LP10kHz,
tank LP6.5kHz, common wet calibration.5. Reproduce source output at these controls
before calling the port faithful to that source. This does NOT establish PCM70
identity; original Rich Chamber's flat envelope/running-stopped behavior and
undocumented topology are not replicated by merely setting these numbers.

Caller-owned storage, bounded44100/48000/96000 preparation, supported decay.25–1.2s.
No callback memory or parameter mutation. Processor dry/wet configurable0–1;
principal listening mix.35wet/.65dry, separate actual wet-only files. Stereo wet
and mono averaging available; disclose mono cancellation/energy, no fake width.

Fault mechanism remains unimplemented until healthy listening files are delivered.
Investigate one existing diffuser recirculation attenuation, distinct from Leo's
scattering-plane focusing. Do not bolt on a resonator or oscillator. Any final
choice and stability/accuracy limits must be recorded before fault code.

## Fault decision after healthy milestone c5a0726 (PR80)

Healthy ZIP was published before fault development. Select exactly ONE existing
right-tank second-allpass recirculation coefficient: normal clamp(g+.15,.25,.5)
(.5 at default) → .94. This models an incorrect/lost attenuation coefficient in
an existing digital reflection/diffuser path, not displaced physical bathroom
panels. Same13 delays, source excitation, diffusion stages, tap readout, size,
filters, crossfeed gain and user controls. No oscillator/extra resonator/output
layer. The allpass's feedforward use of that same coefficient remains tied to its
feedback use by the original recurrence; it is one parameter, not two faults.

For integer delayD, internal W=X/(1-a*z^-D), output Y=(z^-D-a)W. Increasing a
reduces recirculation loss and lengthens its modal persistence; original output
taps already observe this internal history, so selective peaks can emerge.
|a|=.94<1; original tank cross gains remain below1; linear interpolation is
contractive. Validate whole network independently, not just this scalar bound.
No amplitude threshold invented: suitable notes/transients excite existing modes;
weak signals produce proportionally weaker ringing. This differs from Leo's
final scattering-plane H→I focusing; no scattering matrix is changed here.
Whether the result sounds ceramic, distinct and useful is open to listening.
