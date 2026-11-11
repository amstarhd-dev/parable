# SF004 desk experiment: predeclared scope and gates

Written after clean main440a992 Debug/Release16/16, before coupled experiment
results. No production, firmware, existing model test/reference or schematic edit.
PR21 is open: read as research context at b2b1bf4, do not assume it merged.

Compare A four-node force/current-sense querying with B a continuously settling
PNP island containing the local bias/feedback/reactive network. Also evaluate
an analog companion-conductance network with digital histories, without building
it. Distinguish current vectors, device evaluations, derivative probes and lanes.
No physical transistor, converter, amplifier or board measurement is available.

Small independent ngspice experiment: reuse the original physical-sign GP stamps
and circuit, partition only the source resistance into a physical R0 plus a
virtual remainder. A sampled current measurement closes that electrical boundary:
Vdrive[n]=U[n]-(Rs-R0)*Imeasure[n], delivered after specified transport delay,
DAC quantization and finite driver settling. R0 remains physically in series;
Cin, local transistor bias/feedback, Fuzz/bypass, collector loads, Cbc, Cout,
Volume and load are continuous analog. This tests ONE meaningful bidirectional
port; it is neither a direct replacement of the device API nor validation of
arbitrary virtual passives or a two-port interface. Fixed-analog Rs=R0 is a
separate baseline, not presented as a solution to virtual component substitution.

Sample/hold is explicit (not continuous feedback mislabeled digital). Model
16-bit ADC current range +/-1mA,16-bit DAC +/-1V; no bit-depth claims about
real ENOB. Current gain error100ppm, current offset1nA; driver extra series1ohm;
measurement pole0.1us and drive pole0.2us are ASSUMPTIONS, not measured devices.
Try192kHz/1us and48kHz/300us transport, and an optimistic continuous interface
(no sample/quant/delay/error). Existing seven cases cover healthy/Fuzz endpoints,
50k source, weak input and both faults. Never change physical GP constants.

Gates before results: no saturation, no nonfinite samples, no solver failure;
DC node deviation<=1mV. Against original exact reduced-model SPICE fixtures use
unchanged node RMS<.02V/peak<.2V, jack RMS<.0015V/peak<.01V and input RMS<1uA.
Additional boundary-error gate relative to continuous reference: jack RMS<=.2mV,
peak<=1mV and input RMS<=.1uA. No amplitude normalization or fitted delay shift.
These are research fidelity budgets, not listening thresholds. Passing finite
traces cannot prove global closed-loop stability. Record bounded saturation/
limit cycles and integration/interface failures as failures, without gate edits.

If an experiment cannot be made credible, report the exact modeling/blocking
parameter rather than inventing a passing stand-in. Wave-variable/passivity and
analog-companion alternatives need separate proofs; one delayed voltage-type
interface failure does not disprove those architectures. No hardware build follows
from a passing offline experiment.

Before any coupled run, clarify latency comparison: raw end-to-end error is
always reported against the existing undelayed fixture. For isolating feedback
boundary error, also run the full continuous circuit driven through the SAME
known sampled/quantized/delayed forward-source chain, with its full Rs physical.
This is an independently simulated reference stimulus, not post-hoc time fitting.
The .2mV/1mV/.1uA boundary gate applies to that same-stimulus reference.
A known feed-forward delay alone is not classified as a feedback instability.

Numerical implementation note (gates unchanged): the first analog-switch/rounding/
transmission-line prototype aborts in ngspice at a code transition. This is NOT
evidence of physical instability. Retain that rejected deck/log. Replace only
the simulator's converter machinery with exact scheduled stop/read/quantize/alter/
resume events. The plant remains continuous SPICE; DAC words hold between actual
delivery times and capacitor states persist. Check exact event times and a simple
RC pause/resume self-test before accepting traces. No smooth quantizer or relaxed
physical tolerance is used to manufacture convergence.

The paused converter model also encounters an integration failure at a delivery
event with trapezoidal ngspice integration. One numerical diagnostic is permitted:
Gear2 with the SAME reltol/abstol/vntol and physical/interface parameters. If it
runs, require timestep refinement before any stability conclusion; artificial
numerical damping must not be sold as a hardware fix. Preserve abort records.
