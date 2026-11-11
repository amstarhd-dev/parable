# CRC32 narrow adaptation

Upstream: https://github.com/bakercp/CRC32
Revision: `60f89f3b78de2cad300710adc8351bfcf7dc8346` (2.0.1).
Files inspected: `src/CRC32.cpp`, `src/CRC32.h`, `LICENSE.md` (MIT).
The exact upstream license is retained as LICENSE.MIT.

`src/preset_crc.h` adapts only the 16-entry table and two-nibble update/finalize
algorithm. It uses fixed uint32 initialization, const table access and an explicit
byte span; Arduino.h/PROGMEM and generic native-object serialization are omitted.
No Arduino framework, filesystem or storage dependency is imported. The source
header retains the upstream 2013 copyright; the license file carries its 2017
notice. Independent tests use a bitwise evaluator, standard check vector, and
Python zlib/struct envelope bytes. CRC32/ISO-HDLC detects accidental corruption;
it provides neither authentication nor flash atomicity.
