ENTRY(Ram_Entry)
MEMORY {
 CODE (rx) : ORIGIN = 0x24040000, LENGTH = 4K
 MAIL (rw) : ORIGIN = 0x24041000, LENGTH = 128
 STACK (rw) : ORIGIN = 0x2001fc00, LENGTH = 1K
}
PHDRS { code PT_LOAD FLAGS(5); mailbox PT_LOAD FLAGS(6); stack PT_LOAD FLAGS(6); }
SECTIONS {
 .vectors : { KEEP(*(.vectors)) } > CODE :code
 .text : { *(.text*) *(.rodata*) } > CODE :code
 .ARM.exidx : { *(.ARM.exidx*) } > CODE :code
 .data : { *(.data*) } > CODE :code
 .bss (NOLOAD) : { *(.bss*) *(COMMON) } > CODE :code
 .mailbox (NOLOAD) : { _mailbox = .; . += 128; } > MAIL :mailbox
 .stack (NOLOAD) : { . += 1024; _stack_top = .; } > STACK :stack
 .ARM.attributes 0 : { *(.ARM.attributes) } :NONE
 /DISCARD/ : { *(.comment) *(.note*) }
}
ASSERT(SIZEOF(.data) == 0 && SIZEOF(.bss) == 0, "No C runtime initialization allowed")
ASSERT(SIZEOF(.vectors) == 664, "Complete H750 vector table required")
ASSERT(ADDR(.vectors) == 0x24040000, "ROM RAM execution address")
