.syntax unified
.cpu cortex-m7
.thumb
#include "diag_config.h"
// One independent 32-byte cache line per marker. No stack or function calls.
.macro MARK stage
 ldr r0, =(0x24042040 + (\stage-1)*32)
 ldr r1, =0x53544745
 str r1, [r0]
 movs r1, #\stage
 str r1, [r0,#4]
 mvns r1, r1
 str r1, [r0,#8]
 mrs r1, ipsr
 str r1, [r0,#12]
 mrs r1, msp
 str r1, [r0,#16]
 mrs r1, control
 str r1, [r0,#20]
 mrs r1, primask
 str r1, [r0,#24]
 ldr r1, =0x434d4954
 str r1, [r0,#28]
 dsb sy // drain prior normal-memory writes before cache maintenance
 ldr r2, =0xe000ef68 // DCCMVAC: clean this cache line to PoC
 str r0, [r2]
 dsb sy
 isb sy
.endm
.section .text.Diag_Entry,"ax",%progbits
.global Diag_Entry
.type Diag_Entry,%function
.thumb_func
Diag_Entry:
 cpsid i // mask inherited USB IRQs before any instrumentation
 MARK 1 // first observable entry, before stack or CONTROL modification
 MARK 2
#if DIAG_MODE == 0
1: b 1b // intentionally halt; recover manually and collect markers
#else
 ldr r0, =0xe000ed08
 ldr r1, =0x24040000
 str r1, [r0]
 dsb sy
 isb sy
 MARK 3 // fault vectors installed before changing MSP
 ldr r0, =0x2001ffc0
 msr msp, r0
 MARK 4
 movs r0, #0
 msr control, r0
 isb sy
 MARK 5
 b Diag_Main
#endif
.size Diag_Entry,.-Diag_Entry
.ltorg

.section .text.Diag_Fault,"ax",%progbits
.global Diag_Fault
.type Diag_Fault,%function
.thumb_func
Diag_Fault:
 cpsid i
 // Do not dereference a possibly broken stack or call C/C++.
 ldr r0, =0x24042380
 mrs r1, ipsr
 str r1,[r0,#4]
 str lr,[r0,#8]
 mrs r1,msp
 str r1,[r0,#12]
 mrs r1,psp
 str r1,[r0,#16]
 mrs r1,control
 str r1,[r0,#20]
 mrs r1,primask
 str r1,[r0,#24]
 mrs r1,basepri
 str r1,[r0,#28]
 ldr r2,=0xe000ed00
 ldr r1,[r2,#0x28]
 str r1,[r0,#32] // CFSR
 ldr r1,[r2,#0x2c]
 str r1,[r0,#36] // HFSR
 ldr r1,[r2,#0x30]
 str r1,[r0,#40] // DFSR
 ldr r1,[r2,#0x34]
 str r1,[r0,#44] // MMFAR: meaningful only when MMARVALID
 ldr r1,[r2,#0x38]
 str r1,[r0,#48] // BFAR: meaningful only when BFARVALID
 ldr r1,[r2,#0x24]
 str r1,[r0,#52] // SHCSR
 ldr r1,[r2,#0x3c]
 str r1,[r0,#56] // AFSR
 ldr r1,[r2,#4]
 str r1,[r0,#60] // ICSR
 ldr r1,[r2,#8]
 str r1,[r0,#64] // VTOR
 ldr r1,[r2,#0x14]
 str r1,[r0,#68] // CCR
 ldr r1,=0x4641554c
 str r1,[r0]
 ldr r1,=0x46434d54
 str r1,[r0,#124]
 dsb sy
 ldr r2,=0xe000ef68
 str r0,[r2]
 adds r0,#32
 str r0,[r2]
 adds r0,#32
 str r0,[r2]
 adds r0,#32
 str r0,[r2]
 dsb sy
2: b 2b
.size Diag_Fault,.-Diag_Fault
.ltorg
