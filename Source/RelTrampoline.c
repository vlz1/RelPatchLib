#include "RelPatch.h"
#include "RelTrampoline.h"

#if defined(RPL_X86_64)
    extern void TRAMPOLINE_Prologue_Win64();
    extern uint32_t TRAMPOLINE_Prologue_Win64_Length;
    extern uint32_t TRAMPOLINE_Prologue_Win64_CallOffset;
    extern uint32_t TRAMPOLINE_Prologue_Win64_CallRel32Offset;
#elif defined(RPL_X86_32)
#endif

RPLStatus RPLGetTrampoline(RPLConvention callingConvention, RPLTrampoline* trampolineOut)
{
#if defined(RPL_X86_64)
    if (callingConvention == RPL_CALL_X64_MS_ABI)
    {
        void* function = TRAMPOLINE_Prologue_Win64;
        trampolineOut->code = RPLGetJumpTargetRel32(function);
        if (trampolineOut->code == NULL)
            trampolineOut->code = function;
        trampolineOut->codeLength = TRAMPOLINE_Prologue_Win64_Length;
        trampolineOut->callOffset = TRAMPOLINE_Prologue_Win64_CallOffset;
        trampolineOut->callRel32Offset = TRAMPOLINE_Prologue_Win64_CallRel32Offset;
        return RPL_STATUS_SUCCESS;
    }
    else if (callingConvention == RPL_CALL_X64_SYSV_ABI)
    {
        // TODO: SysV x64
        return RPL_STATUS_SUCCESS;
    }
#elif defined(RPL_X86_32)
#endif
    return RPL_STATUS_INVALID_CALLING_CONVENTION;
}