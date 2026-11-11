# Control-side JSON dependency

- Project: JSON for Modern C++ (`nlohmann/json`).
- Version: **3.11.3**, unmodified release archive.
- Source: https://github.com/nlohmann/json/releases/tag/v3.11.3
- Exact archive: https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
- SHA-256: `d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d`
- License: MIT; the release's exact notice is retained in
  [nlohmann-json-LICENSE.MIT](nlohmann-json-LICENSE.MIT) and in the fetched source.
- Rationale: established C++17-compatible strict JSON/UTF-8 ingress and public SAX
  interface. Parable rejects raw NUL bytes (upstream treats them as end-of-input)
  and adds duplicate detection, input limits, schema/graph checks,
  direct binary32 token conversion and deterministic output. No GPL source added.

CMake FetchContent downloads and verifies the pinned archive at configure time.
Network access to the release is required on a fresh build. For an offline build,
pre-extract that same verified archive and configure with
`-DFETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON=/absolute/path/to/json`.
That override is the caller's responsibility; CMake does not reverify its contents.
No dependency update is implicit. No JSON library is linked to the audio targets,
and no MCU footprint/compatibility claim is made.

PAR-010 narrowly reuses the MIT CRC32 evaluator with pinned source/notice and
adaptation details in [crc32/README.md](crc32/README.md). Its journal remains a
control-side library; no storage/JSON dependency enters existing audio targets.
