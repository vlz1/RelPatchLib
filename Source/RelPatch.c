#include "RelPatch.h"
#include "RelDispatch.h"
#include "RelX86Util.h"
#include "RelPlatform.h"
#include <stdio.h>

static const char* s_StatusStrings[] = {
    "Success",
    "Platform error",
    "Invalid argument",
    "Pattern mismatch",
    "Pattern malformed",
    "Allocation failed",
    "Instruction relocation failed",
    "Disassembly failed",
    "Page protection failed",
    "Dynamic code prohibited",
    "Invalid calling convention"
};

const char* RPLGetStatusString(RPLStatus status)
{
    if (status >= (sizeof(s_StatusStrings) / sizeof(const char*)))
        return "???";
    else if (status < 0)
        return "???";
    return s_StatusStrings[(int)status];
}

void* RPLAllocatePagesWithin2GB(void* address, size_t nPages)
{
    void* addressOut = NULL;
    RPLStatus status = RPLPlatformAllocPagesWithin2GB(address, nPages, &addressOut);
    if (status != RPL_STATUS_SUCCESS)
        return NULL;
    return addressOut;
}

RPLStatus RPLInstallHookEx(void* function, RPLHookFunc prologueHook, RPLHookFunc epilogueHook, RPLConvention callingConvention, uint32_t priority, void* userData)
{
    RPLStatus status = RPL_STATUS_SUCCESS;
    uint8_t* functionBytes = (uint8_t*)function;
    
    // TODO: Platform-agnostic method to check if dynamic code is allowed
    // if (!RPLIsDynamicCodeAvailable())
    //    return RPL_STATUS_DYNAMIC_CODE_PROHIBITED;

    // Validate calling convention
    switch (callingConvention)
    {
#if defined(_M_X64)
    case RPL_CALL_X64_MS_ABI:
    case RPL_CALL_X64_SYSV_ABI:
        break;
#elif defined(_M_IX86)
    case RPL_CALL_X86_CDECL:
    case RPL_CALL_X86_STDCALL:
        break;
#endif
    default:
        return RPL_STATUS_INVALID_CALLING_CONVENTION;
    }

    // Check if a hook is already installed here
    uint8_t* jumpTarget = RPLGetJumpTargetRel32(function);
    if (jumpTarget != NULL)
    {
        // Hook dispatch code is always aligned on a page boundary
        if (RPLIsAddressPageAligned(jumpTarget))
        {
            // If it's one of our code pages, just add the hook function to the existing dispatch table
            RPLCodePage* codePage = (RPLCodePage*)jumpTarget;
            if (codePage->meta.signature == RPL_CODE_SIGNATURE)
                return RPLAppendHookToDispatch(codePage, prologueHook, epilogueHook, priority, userData);
        }

        // Follow the relative jump and hook the target function
        return RPLInstallHookEx(jumpTarget, prologueHook, epilogueHook, callingConvention, priority, userData);
    }

    // Allocate the page for our dispatch code
    RPLCodePage* codePage = (RPLCodePage*)RPLAllocatePagesWithin2GB(function, 1);
    if (codePage == NULL)
        return RPL_STATUS_ALLOCATION_FAILED;

    // Allocate the table that'll hold our function pointers
    size_t dispatchTableSize = RPL_PAGE_SIZE * RPL_DISPATCH_TABLE_PAGES;
    RPLDispatchTable* dispatchTable;
    status = RPLPlatformVirtualAlloc(NULL, 
        dispatchTableSize, 
        RPL_PAGE_READ_WRITE, 
        (void**)&dispatchTable);
    if (status != RPL_STATUS_SUCCESS)
    {
        RPLPlatformVirtualFree(codePage, RPL_PAGE_SIZE);
        return RPL_STATUS_ALLOCATION_FAILED;
    }

    codePage->meta.signature = RPL_CODE_SIGNATURE;
    codePage->meta.callingConvention = callingConvention;
    codePage->meta.dispatchTable = dispatchTable;
    dispatchTable->nDispatchEntries = 0;
    dispatchTable->nMaxDispatchEntries = RPL_DISPATCH_TABLE_ENTRIES;
    status = RPLAppendHookToDispatch(codePage, prologueHook, epilogueHook, priority, userData);
    if (status != RPL_STATUS_SUCCESS)
    {
        RPLPlatformVirtualFree(dispatchTable, dispatchTableSize);
        RPLPlatformVirtualFree(codePage, RPL_PAGE_SIZE);
        return status;
    }

    // Set up the code page with the trampoline that calls RPLDispatchCommon
    RPLRelocatorState relocator = { 0 };
    relocator.inSrcCode = functionBytes;
    relocator.inDstPage = codePage;
    relocator.inCallingConvention = callingConvention;
    relocator.inRequiredBytes = 5;
    relocator.inMaxBytes = 16;
    status = RPLRelocateCode(&relocator);
    if (status != RPL_STATUS_SUCCESS)
    {
        RPLPlatformVirtualFree(dispatchTable, dispatchTableSize);
        RPLPlatformVirtualFree(codePage, RPL_PAGE_SIZE);
        return status;
    }

    // Mark code page as executable
    RPLPageProtect oldProtect = 0;
    if (!RPL_SUCCESSFUL(RPLPlatformVirtualProtect(codePage, RPL_PAGE_SIZE, RPL_PAGE_READ_EXECUTE, &oldProtect)))
        goto virtualprotect_failed;

    // Write the jump that goes to our code page
    if (!RPL_SUCCESSFUL(RPLPlatformVirtualProtect(function, relocator.inRequiredBytes, RPL_PAGE_READ_WRITE_EXECUTE, &oldProtect)))
        goto virtualprotect_failed;
    RPLWriteJumpRel32(function, codePage->code, relocator.nBytesRelocated);

    if (!RPL_SUCCESSFUL(RPLPlatformVirtualProtect(function, relocator.inRequiredBytes, oldProtect, &oldProtect)))
        goto virtualprotect_failed;

    return RPL_STATUS_SUCCESS;
virtualprotect_failed:
    RPLPlatformVirtualFree(dispatchTable, dispatchTableSize);
    RPLPlatformVirtualFree(codePage, RPL_PAGE_SIZE);
    return RPL_STATUS_VIRTUALPROTECT_FAILED;
}

RPLStatus RPLInstallHook(void* function, RPLHookFunc prologueHook, RPLHookFunc epilogueHook, RPLConvention callingConvention)
{
    return RPLInstallHookEx(function, prologueHook, epilogueHook, callingConvention, 0, NULL);
}

bool RPLIsFunctionHooked(void* function, RPLConvention* callingConventionOut)
{
    uint8_t* jumpTarget = RPLGetJumpTargetRel32(function);
    if (jumpTarget == NULL)
        return false;

    if (!RPLIsAddressPageAligned(jumpTarget))
        return false;

    RPLCodePage* codePage = (RPLCodePage*)jumpTarget;
    if (codePage->meta.signature != RPL_CODE_SIGNATURE)
        return false;

    *callingConventionOut = codePage->meta.callingConvention;
    return true;
}

char* RPLDumpInstructions(void* address, int nInstructions, bool withAddresses)
{
    return RPLX86DumpInstructions(address, nInstructions, withAddresses);
}