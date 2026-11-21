# SF004 architecture research tools

Offline research only: no processor, converter driver or validated HIL system.

- `budget.py`: rates, precision and companion-current arithmetic from pinned
  native workload and independent SPICE bias fixtures.
- `linear_port.py`: exact continuous two-pole evolution between ADC/DAC events
  for a **resistor surrogate**, with quantization, offset/gain, finite burden,
  transport, ZOH and settling. This is not the nonlinear GP circuit.
- `coupled_spice.py`: original independent GP/circuit stamps with one electrical
  source-resistance boundary. Continuous mode matches all7 original cases.
  Sampled modes currently abort at delivery-event integration: **unvalidated**,
  exit2 with failure record. Numerical aborts are not physical instability.
- `PLAN.md`: gates recorded before results; numerical-method attempts do not
  relax physical tolerances.
- `check.py`: verifies arithmetic/reference evidence and disclosure of the
  blocker, never declares mixed-signal hardware valid.

From repository root:

```bash
python3 parable-starter/tools/sf004/budget.py --output /tmp/sf004-budget.json
python3 parable-starter/tools/sf004/linear_port.py --output /tmp/sf004-linear.json
PYTHONDONTWRITEBYTECODE=1 python3 parable-starter/tools/sf004/check.py
python3 parable-starter/tools/sf004/coupled_spice.py --ngspice /path/to/ngspice44.2 --output /tmp/sf004-continuous --mode continuous
PYTHONDONTWRITEBYTECODE=1 python3 parable-starter/tools/sf004/check.py --continuous /tmp/sf004-continuous/results.json
```

Reproduce the **known numerical blocker**, not a fidelity pass:

```bash
python3 parable-starter/tools/sf004/coupled_spice.py --ngspice /path/to/ngspice44.2 --output /tmp/sf004-blocked --case healthy_mid --mode fast_192k
# Expected exit2: explicit incomplete/aborted-transient record.
```

Use a fresh output directory. The generator never removes existing output
directories; large raw tables are hashed then removed to bound disk use. It
retains sampled comparisons, generated decks and logs. A short aborted trace
cannot pass. Failed fidelity and numerical execution errors are distinct.

Use the existing ngspice44.2 package pin in
`third_party/germanium/provenance.json`. No new external code/model is imported;
existing GP stamps and process wrapper are reused, not a duplicate engine.
Ngspice remains an external offline tool, never a DSP/firmware dependency.
Source PDF hashes/versions are in `tests/fixtures/sf004/sources.json`; PDFs and
external illustrations are not redistributed. Original notices remain unchanged.

See [the report](../../docs/PHYSICAL_BJT_FEASIBILITY.md). Passing research CI
means calculations/disclosures reproduce, not that physical HIL works.
