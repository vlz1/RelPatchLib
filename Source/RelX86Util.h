#ifndef RELX86_UTIL_H
#define RELX86_UTIL_H

#include "RelPatch.h"
#include "Zydis/Zydis.h"

#ifdef _M_X64
    #define ZYDIS_MACHINE_MODE_NATIVE ZYDIS_MACHINE_MODE_LONG_64
#else
    #define ZYDIS_MACHINE_MODE_NATIVE ZYDIS_MACHINE_MODE_LEGACY_32
#endif

typedef struct
{
    uint8_t* code;
    uint32_t codeLength;
} RPLTrampoline;

typedef struct
{
    //
    // Inputs
    //

    uint8_t* inSrcCode;
    RPLCodePage* inDstPage;
    RPLConvention inCallingConvention;
    uint32_t inRequiredBytes;
    uint32_t inMaxBytes;

    //
    // State
    //
    
    // Offset of next instruction that will be written. Starts at 0 and grows upwards
    uint32_t codeOffset;
    uint32_t constOffset;

    //
    // Outputs
    //

    uint32_t nBytesRelocated;
} RPLRelocatorState;

typedef struct
{
    // Absolute target of the instruction
    uint8_t* target;
    // Size in bytes of the relocation (1, 2, 4, 8)
    uint16_t size;
    // Offset from the beginning of the instruction
    int16_t offset;
} RPLRelocation;

RPLStatus RPLRelocateCode(RPLRelocatorState* state);
void RPLWriteJumpRel32(uint8_t* dst, void* callTarget, uint32_t freeSpace);

bool RPLGetInstructionRelocation(uint8_t* address, const ZydisDisassembledInstruction* instruction, RPLRelocation* relocation);
bool RPLIsInstructionRelocatable(const ZydisDisassembledInstruction* instruction);

char* RPLX86DumpInstructions(void* address, int nInstructions, bool withAddresses);

#endif