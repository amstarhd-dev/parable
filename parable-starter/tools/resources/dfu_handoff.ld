/* Supplement the exact SDK BOOT_NONE script; do not expand or replace FLASH.
   Startup zeroes .bss only. This cache-line-sized mailbox survives SYSRESETREQ. */
SECTIONS {
  .parable_handoff (NOLOAD) : ALIGN(32) {
    KEEP(*(.parable_handoff))
  } > SRAM
} INSERT BEFORE .bss;
ASSERT(SIZEOF(.parable_handoff) == 32, "benchmark handoff mailbox size")
