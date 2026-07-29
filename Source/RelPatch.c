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

RPLStatus RPLInstallPrologueHookEx(void* function, RPLHookFunc hook, RPLConvention callingConvention, uint32_t priority, void* userData)
{
    RPLStatus status = RPL_STATUS_SUCCESS;
    uint8_t* functionBytes = (uint8_t*)function;
    
    // TODO: Platform-agnostic method to check if dynamic code is allowed
    // if (!RPLIsDynamicCodeAvailable())
    //    return RPL_STATUS_DYNAMIC_CODE_PROHIBITED;

    // Validate calling convention
#if defined(_M_X64)
    if (callingConvention != RPL_CALL_X64)
        return RPL_STATUS_INVALID_CALLING_CONVENTION;
#elif defined(_M_IX86)
    switch (callingConvention)
    {
    case RPL_CALL_X86_CDECL:
    case RPL_CALL_X86_STDCALL:
        break;
    default:
        return RPL_STATUS_INVALID_CALLING_CONVENTION;
    }
#endif

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
                return RPLAppendHookToDispatch(codePage, hook, priority, userData);
        }

        // Follow the relative jump and hook the target function
        return RPLInstallPrologueHookEx(jumpTarget, hook, callingConvention, priority, userData);
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
    codePage->meta.dispatchTable = dispatchTable;
    dispatchTable->nDispatchEntries = 0;
    dispatchTable->nMaxDispatchEntries = RPL_DISPATCH_TABLE_ENTRIES;
    status = RPLAppendHookToDispatch(codePage, hook, priority, userData);
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
    // TODO: Move in the relocator from the old project
    //status = RPLRelocateCode(&relocator);
    if (status != RPL_STATUS_SUCCESS)
    {
        RPLPlatformVirtualFree(dispatchTable, dispatchTableSize);
        RPLPlatformVirtualFree(codePage, RPL_PAGE_SIZE);
        return status;
    }

    // Mark code page as executable
    RPLPageProtect oldProtect = 0;
    if (!RPLPlatformVirtualProtect(codePage, RPL_PAGE_SIZE, RPL_PAGE_READ_EXECUTE, &oldProtect))
        goto virtualprotect_failed;

    // Write the jump that goes to our code page
    if (!RPLPlatformVirtualProtect(function, relocator.inRequiredBytes, RPL_PAGE_READ_WRITE_EXECUTE, &oldProtect))
        goto virtualprotect_failed;
    RPLWriteJumpRel32(function, codePage->code, relocator.nBytesRelocated);

    if (!RPLPlatformVirtualProtect(function, relocator.inRequiredBytes, oldProtect, &oldProtect))
        goto virtualprotect_failed;

    return RPL_STATUS_SUCCESS;
virtualprotect_failed:
    RPLPlatformVirtualFree(dispatchTable, dispatchTableSize);
    RPLPlatformVirtualFree(codePage, RPL_PAGE_SIZE);
    return RPL_STATUS_VIRTUALPROTECT_FAILED;
}

RPLStatus RPLInstallPrologueHook(void* function, RPLHookFunc hook, RPLConvention callingConvention)
{
    return RPLInstallPrologueHookEx(function, hook, callingConvention, 0, NULL);
}

char* RPLDumpInstructions(void* address, int nInstructions, bool withAddresses)
{
    return RPLX86DumpInstructions(address, nInstructions, withAddresses);
}