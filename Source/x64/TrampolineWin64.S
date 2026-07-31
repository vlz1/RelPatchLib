// 8 bytes to align the stack
// 48 bytes for RCX, RDX, R8, R9, RAX, and RBP
// 64 bytes for XMM0-3
// 32 bytes for RPLHookContext
// 32 bytes for RPLDispatchCommon shadow space
.set TRAMPOLINE_StackSize, (8 + 48 + 64 + 32 + 32)

.set TRAMPOLINE_StackGPRBase, (TRAMPOLINE_StackSize - (48 + 8))
.set TRAMPOLINE_StackRAX, (TRAMPOLINE_StackGPRBase + 32)
.set TRAMPOLINE_StackRBP, (TRAMPOLINE_StackGPRBase + 40)
.set TRAMPOLINE_StackXMMBase, (TRAMPOLINE_StackGPRBase - 64)
.set TRAMPOLINE_StackContextBase, (TRAMPOLINE_StackXMMBase - 32)

.global TRAMPOLINE_Prologue_Win64
TRAMPOLINE_Prologue_Win64:
    // Align the stack and reserve space for registers
    subq $TRAMPOLINE_StackSize, %rsp

    // Save arg registers
    movq %rcx, TRAMPOLINE_StackGPRBase(%rsp)
    movq %rdx, TRAMPOLINE_StackGPRBase + 8(%rsp)
    movq %r8, TRAMPOLINE_StackGPRBase + 16(%rsp)
    movq %r9, TRAMPOLINE_StackGPRBase + 24(%rsp)
    xorq %r8, %r8

    // Linker doesn't like this JMP REL8 for some reason
    .byte 0xEB
    .byte (_TRAMPOLINE_SaveRegs - _TRAMPOLINE_EpilogueEntry)
_TRAMPOLINE_EpilogueEntry:
    subq $TRAMPOLINE_StackSize + 8, %rsp
    movl $1, %r8d
_TRAMPOLINE_SaveRegs:
    movaps %xmm0, TRAMPOLINE_StackXMMBase(%rsp)
    movaps %xmm1, TRAMPOLINE_StackXMMBase + 16(%rsp)
    movaps %xmm2, TRAMPOLINE_StackXMMBase + 32(%rsp)
    movaps %xmm3, TRAMPOLINE_StackXMMBase + 48(%rsp)
    movq %rax, TRAMPOLINE_StackRAX(%rsp)
    movq %rbp, TRAMPOLINE_StackRBP(%rsp)

    // Get address of current code page for RPLDispatchCommon first arg
.set _TRAMPOLINE_Prologue_PageOffset, . - TRAMPOLINE_Prologue_Win64
    lea -7 - _TRAMPOLINE_Prologue_PageOffset(%rip), %rcx

    // Build x64 RPLHookContext
    leaq TRAMPOLINE_StackContextBase(%rsp), %rdx
    movl $0, (%rdx) // RPL_CALL_X64_MS_ABI
    movl %r8d, 4(%rdx)
    leaq TRAMPOLINE_StackSize + 40(%rsp), %rax
    movq %rax, 8(%rdx)
    leaq TRAMPOLINE_StackGPRBase(%rsp), %rax
    movq %rax, 16(%rdx)
    leaq TRAMPOLINE_StackXMMBase(%rsp), %rax
    movq %rax, 24(%rdx)

    // Get return address for RPLDispatchCommon third arg
    leaq (_TRAMPOLINE_EpilogueEntry - . - 7)(%rip), %r8
    xchgq TRAMPOLINE_StackSize(%rsp), %r8

    // CALL QWORD PTR [RIP + REL32] -> RPLDispatchCommon
.set _TRAMPOLINE_Prologue_CallOffset, . - TRAMPOLINE_Prologue_Win64
    .byte 0xFF
    .byte 0x15
.set _TRAMPOLINE_Prologue_CallRel32, . - TRAMPOLINE_Prologue_Win64
    .long 0x00000000

    // Restore float registers
    movaps TRAMPOLINE_StackXMMBase(%rsp), %xmm0
    movaps TRAMPOLINE_StackXMMBase + 16(%rsp), %xmm1
    movaps TRAMPOLINE_StackXMMBase + 32(%rsp), %xmm2
    movaps TRAMPOLINE_StackXMMBase + 48(%rsp), %xmm3

    // If RPLDispatchCommon returned NULL, we're leaving a prologue hook and entering the actual function.
    // Otherwise, we're returning from an epilogue hook and we need to get the original return address.
    movq %rax, %rcx
    testq %rcx, %rcx
    jz 3f
    2:
        // Exit epilogue hook
        // Don't bother restoring volatile arg registers
        movq TRAMPOLINE_StackRAX(%rsp), %rax
        movq TRAMPOLINE_StackRBP(%rsp), %rbp
        addq $TRAMPOLINE_StackSize + 8, %rsp

        // Get epilogueReturnAddress from RPLThreadContext
        xorq %rdx, %rdx
        xchgq (%rcx), %rdx
        jmp *%rdx
    3:
        // Restore all GP registers and exit prologue hook
        movq TRAMPOLINE_StackGPRBase(%rsp), %rcx
        movq TRAMPOLINE_StackGPRBase + 8(%rsp), %rdx
        movq TRAMPOLINE_StackGPRBase + 16(%rsp), %r8
        movq TRAMPOLINE_StackGPRBase + 24(%rsp), %r9
        movq TRAMPOLINE_StackRAX(%rsp), %rax
        movq TRAMPOLINE_StackRBP(%rsp), %rbp
        addq $TRAMPOLINE_StackSize, %rsp
    // Continue to relocated code...

.global TRAMPOLINE_Prologue_Win64_Length
TRAMPOLINE_Prologue_Win64_Length:
    .long . - TRAMPOLINE_Prologue_Win64

.global TRAMPOLINE_Prologue_Win64_CallOffset
TRAMPOLINE_Prologue_Win64_CallOffset:
    .long _TRAMPOLINE_Prologue_CallOffset

.global TRAMPOLINE_Prologue_Win64_CallRel32Offset
TRAMPOLINE_Prologue_Win64_CallRel32Offset:
    .long _TRAMPOLINE_Prologue_CallRel32