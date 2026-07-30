// 8 bytes to align the stack
// 32 bytes for RCX, RDX, R8, and R9
// 64 bytes for XMM0-3
// 32 bytes for RPLHookContext
// 32 bytes for RPLDispatchCommon shadow space
.set TRAMPOLINE_StackSize, (8 + 32 + 64 + 32 + 32)

.set TRAMPOLINE_StackGPRBase, (TRAMPOLINE_StackSize - (32 + 8))
.set TRAMPOLINE_StackXMMBase, (TRAMPOLINE_StackGPRBase - 64)
.set TRAMPOLINE_StackContextBase, (TRAMPOLINE_StackXMMBase - 32)

.global TRAMPOLINE_Prologue_Win64
TRAMPOLINE_Prologue_Win64:
    // Get address of current code page
    lea -7(%rip), %rax

    // Align the stack and reserve space for registers
    sub $TRAMPOLINE_StackSize, %rsp

    // Save argument registers
    movaps %xmm0, TRAMPOLINE_StackXMMBase(%rsp)
    movaps %xmm1, TRAMPOLINE_StackXMMBase + 16(%rsp)
    movaps %xmm2, TRAMPOLINE_StackXMMBase + 32(%rsp)
    movaps %xmm3, TRAMPOLINE_StackXMMBase + 48(%rsp)
    movq %rcx, TRAMPOLINE_StackGPRBase(%rsp)
    movq %rdx, TRAMPOLINE_StackGPRBase + 8(%rsp)
    movq %r8, TRAMPOLINE_StackGPRBase + 16(%rsp)
    movq %r9, TRAMPOLINE_StackGPRBase + 24(%rsp)

    // Code page address -> RPLDispatchCommon first arg
    mov %rax, %rcx

    // Build x64 RPLHookContext
    leaq TRAMPOLINE_StackContextBase(%rsp), %rdx
    movq $0, (%rdx) // RPL_CALL_X64_MS_ABI
    leaq TRAMPOLINE_StackSize + 40(%rsp), %rax
    movq %rax, 8(%rdx)
    leaq TRAMPOLINE_StackGPRBase(%rsp), %rax
    movq %rax, 16(%rdx)
    leaq TRAMPOLINE_StackXMMBase(%rsp), %rax
    movq %rax, 24(%rdx)

    // CALL QWORD PTR [RIP + REL32]
.set _TRAMPOLINE_Prologue_CallOffset, . - TRAMPOLINE_Prologue_Win64
    .byte 0xFF
    .byte 0x15
.set _TRAMPOLINE_Prologue_CallRel32, . - TRAMPOLINE_Prologue_Win64
    .long 0x00000000

    // Restore registers
    movaps TRAMPOLINE_StackXMMBase(%rsp), %xmm0
    movaps TRAMPOLINE_StackXMMBase + 16(%rsp), %xmm1
    movaps TRAMPOLINE_StackXMMBase + 32(%rsp), %xmm2
    movaps TRAMPOLINE_StackXMMBase + 48(%rsp), %xmm3
    movq TRAMPOLINE_StackGPRBase(%rsp), %rcx
    movq TRAMPOLINE_StackGPRBase + 8(%rsp), %rdx
    movq TRAMPOLINE_StackGPRBase + 16(%rsp), %r8
    movq TRAMPOLINE_StackGPRBase + 24(%rsp), %r9

    add $TRAMPOLINE_StackSize, %rsp

.global TRAMPOLINE_Prologue_Win64_Length
TRAMPOLINE_Prologue_Win64_Length:
    .long . - TRAMPOLINE_Prologue_Win64

.global TRAMPOLINE_Prologue_Win64_CallOffset
TRAMPOLINE_Prologue_Win64_CallOffset:
    .long _TRAMPOLINE_Prologue_CallOffset

.global TRAMPOLINE_Prologue_Win64_CallRel32Offset
TRAMPOLINE_Prologue_Win64_CallRel32Offset:
    .long _TRAMPOLINE_Prologue_CallRel32