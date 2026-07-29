#include "RelX86Util.h"
#include <stdio.h>

bool RPLGetInstructionRelocation(uint8_t* address, const ZydisDisassembledInstruction* instruction, RPLRelocation* relocation)
{
    for (int i = 0; i < instruction->info.operand_count_visible; ++i)
    {
        const ZydisDecodedOperand* operand = &instruction->operands[i];
        if (operand->type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            if (operand->mem.base == ZYDIS_REGISTER_RIP || operand->mem.base == ZYDIS_REGISTER_EIP)
            {
                relocation->target = address + (operand->mem.disp.value + instruction->info.length);
                relocation->size = 4;
                relocation->offset = instruction->info.length - 4;
                return true;
            }
        }
    }
    return false;
}

bool RPLIsInstructionRelocatable(const ZydisDisassembledInstruction* instruction)
{
    switch (instruction->info.mnemonic)
    {
        // We don't try to relocate conditional branches, because they indicate more
        // complex control flow than the typical function prologue.
        // There's already a risk that a branch within the function body may jump
        // backwards into the middle of our patch, and the presence of complex
        // control flow makes that even more likely.
    case ZYDIS_MNEMONIC_JB:
    case ZYDIS_MNEMONIC_JBE:
    case ZYDIS_MNEMONIC_JCXZ:
    case ZYDIS_MNEMONIC_JECXZ:
    case ZYDIS_MNEMONIC_JKNZD:
    case ZYDIS_MNEMONIC_JKZD:
    case ZYDIS_MNEMONIC_JL:
    case ZYDIS_MNEMONIC_JLE:
    case ZYDIS_MNEMONIC_JNB:
    case ZYDIS_MNEMONIC_JNBE:
    case ZYDIS_MNEMONIC_JNL:
    case ZYDIS_MNEMONIC_JNLE:
    case ZYDIS_MNEMONIC_JNO:
    case ZYDIS_MNEMONIC_JNP:
    case ZYDIS_MNEMONIC_JNS:
    case ZYDIS_MNEMONIC_JNZ:
    case ZYDIS_MNEMONIC_JO:
    case ZYDIS_MNEMONIC_JP:
    case ZYDIS_MNEMONIC_JRCXZ:
    case ZYDIS_MNEMONIC_JS:
    case ZYDIS_MNEMONIC_JZ:
        return false;
    default:
        return true;
    }
}

void RPLWriteJumpRel32(uint8_t* dst, void* callTarget, uint32_t freeSpace)
{
    int32_t rel32 = RPLCalculateRel32FromAddress(dst, 5, callTarget);
    dst[0] = 0xE9;
    *(int32_t*)(dst + 1) = rel32;
    for (uint32_t i = 5; i < freeSpace; ++i)
        dst[i] = 0x90; // Pad with NOP
}

char* RPLX86DumpInstructions(void* address, int nInstructions, bool withAddresses)
{
    ZydisDisassembledInstruction instruction = { 0 };
    size_t size = (size_t)nInstructions * sizeof(instruction.text);

    // Add space for newlines and NULL terminator
    size += ((size_t)nInstructions) + 1;

    if (withAddresses)
    {
        // Add space for addresses
        size += ((size_t)nInstructions * (sizeof(void*) * 2 + 3));
    }

    char* buffer = malloc(size);
    if (buffer == NULL)
        return NULL;

    char* writePtr = buffer;
    int count = 0;
    while (count < nInstructions)
    {
        ZyanStatus status = ZydisDisassembleIntel(
            ZYDIS_MACHINE_MODE_NATIVE, (uintptr_t)address, address, 16, &instruction);
        if (ZYAN_FAILED(status))
        {
            // Something broke in the disassembler
            free(buffer);
            return NULL;
        }

        size_t remaining = size - (writePtr - buffer);
        int written = 0;
        if (withAddresses)
            written = sprintf_s(writePtr, remaining, "%p: %s", address, instruction.text);
        else
            written = sprintf_s(writePtr, remaining, "%s", instruction.text);
        if (written < 0)
        {
            // Ran out of space in the buffer for our text
            free(buffer);
            return NULL;
        }
        writePtr += written;
        address = (uint8_t*)address + instruction.info.length;

        if (count < (nInstructions - 1))
            *(writePtr++) = '\n';
        ++count;
    }
    *(writePtr++) = '\0';
    return buffer;
}