#include "RelPatch.h"
#include "../RelTrampoline.h"
#include "../RelDispatch.h"
#include "../RelX86Util.h"
#include <stdio.h>
#include <string.h>

RPLStatus RPLRelocateCode(RPLRelocatorState* state)
{
    ZydisDisassembledInstruction instruction = { 0 };
    RPLTrampoline trampolineCode = { 0 };
    RPLStatus result = RPL_STATUS_SUCCESS;
    uint32_t offset = 0;
    uint8_t* srcCode = state->inSrcCode;
    uint8_t* dstCode = state->inDstPage->code;
    state->constOffset = 0;

    // Get the correct trampoline code for the calling convention
    result = RPLGetTrampoline(state->inCallingConvention, &trampolineCode);
    if (result != RPL_STATUS_SUCCESS)
        return result;

    // Write the trampoline that saves registers and calls the actual dispatch code
    memcpy(dstCode, trampolineCode.code, trampolineCode.codeLength);
    dstCode += trampolineCode.codeLength;
    
    do
    {
        ZyanStatus status = ZydisDisassembleIntel(
            ZYDIS_MACHINE_MODE_LONG_64,
            (uintptr_t)srcCode,
            srcCode,
            state->inMaxBytes - offset,
            &instruction
        );
        if (ZYAN_FAILED(status))
        {
            return RPL_STATUS_DISASSEMBLY_FAILED;
        }
        else if (!RPLIsInstructionRelocatable(&instruction))
        {
            // TODO: Report which instruction caused the relocation to fail
            return RPL_STATUS_RELOCATION_FAILED;
        }

        RPLRelocation relocation;
        if (!RPLGetInstructionRelocation(srcCode, &instruction, &relocation))
        {
            // Instruction is position-independent, we can just copy it
            for (uint32_t i = 0; i < instruction.info.length; ++i)
                *dstCode++ = srcCode[i];
        }
        else
        {
            // TODO: Relocate RIP-relative instructions
        }

        printf("%p %s\n", srcCode, instruction.text);
        srcCode += instruction.info.length;
        offset += instruction.info.length;
        state->nBytesRelocated += instruction.info.length;
    } while (offset < state->inRequiredBytes);

    // Write the JMP that returns us to the original function
    uint8_t* returnAddr = state->inSrcCode + offset;
    int32_t returnRel32 = RPLCalculateRel32FromAddress(dstCode, 5, returnAddr);
    *dstCode++ = 0xE9;
    *(int32_t*)(dstCode) = returnRel32;
    dstCode += 4;

    // Write address of RPLDispatchCommon
    dstCode = (uint8_t*)RPL_ALIGN_UP(dstCode, 8);
    *(void**)(dstCode) = RPLDispatchCommon;

    // Point the CALL QWORD PTR [RIP + Rel32] to the address of RPLDispatchCommon
    int32_t dispatchRel32 = RPLCalculateRel32FromAddress(
        state->inDstPage->code + trampolineCode.callOffset,
        6,
        dstCode
    );
    *(int32_t*)(state->inDstPage->code + trampolineCode.callRel32Offset) = dispatchRel32;

    return RPL_STATUS_SUCCESS;
}