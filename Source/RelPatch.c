#include "RelPatch.h"
#include "RelX86Util.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

RPLStatus RPLDispatchCommon(RPLCodePage* codePage, RPLHookContext* context)
{
    RPLDispatchTable* dispatchTable = codePage->meta.dispatchTable;
    for (uint16_t i = 0; i < dispatchTable->nDispatchEntries; ++i)
    {
        RPLDispatchEntry* entry = &dispatchTable->entries[i];
        entry->function(context, entry->userData);
    }
    return RPL_STATUS_SUCCESS;
}

RPLStatus RPLAppendHookToDispatch(RPLCodePage* codePage, RPLHookFunc hook, uint32_t priority, void* userData)
{
    RPLDispatchTable* dispatchTable = codePage->meta.dispatchTable;
    if (dispatchTable->nDispatchEntries == 0)
    {
        ++dispatchTable->nDispatchEntries;
        dispatchTable->entries[0] = (RPLDispatchEntry){ 
            .function = hook,
            .userData = userData,
            .priority = priority
        };
        return RPL_STATUS_SUCCESS;
    }
    
    // TODO: Binary search for a place to put the new dispatch entry

    return RPL_STATUS_SUCCESS;
}

RPLStatus RPLInstallPrologueHookEx(void* function, RPLHookFunc hook, RPLConvention callingConvention, uint32_t priority, void* userData)
{
    RPLStatus status = RPL_STATUS_SUCCESS;
    uint8_t* functionBytes = (uint8_t*)function;
    if (!RPLIsDynamicCodeAvailable())
        return RPL_STATUS_DYNAMIC_CODE_PROHIBITED;

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
    RPLDispatchTable* dispatchTable = (RPLDispatchTable*)VirtualAlloc(NULL,
        RPL_PAGE_SIZE * RPL_DISPATCH_TABLE_PAGES,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);
    if (dispatchTable == NULL)
    {
        VirtualFree(codePage, 0, MEM_RELEASE);
        return RPL_STATUS_ALLOCATION_FAILED;
    }

    codePage->meta.signature = RPL_CODE_SIGNATURE;
    codePage->meta.dispatchTable = dispatchTable;
    dispatchTable->nDispatchEntries = 0;
    dispatchTable->nMaxDispatchEntries = RPL_DISPATCH_TABLE_ENTRIES;
    status = RPLAppendHookToDispatch(codePage, hook, priority, userData);
    if (status != RPL_STATUS_SUCCESS)
    {
        VirtualFree(dispatchTable, 0, MEM_RELEASE);
        VirtualFree(codePage, 0, MEM_RELEASE);
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
        VirtualFree(dispatchTable, 0, MEM_RELEASE);
        VirtualFree(codePage, 0, MEM_RELEASE);
        return status;
    }

    // Mark code page as executable
    DWORD oldProtect = 0;
    if (!VirtualProtect(codePage, RPL_PAGE_SIZE, PAGE_EXECUTE_READ, &oldProtect))
        goto virtualprotect_failed;

    // Write the jump that goes to our code page
    if (!VirtualProtect(function, relocator.inRequiredBytes, PAGE_EXECUTE_READWRITE, &oldProtect))
        goto virtualprotect_failed;
    RPLWriteJumpRel32(function, codePage->code, relocator.nBytesRelocated);

    if (!VirtualProtect(function, relocator.inRequiredBytes, oldProtect, &oldProtect))
        goto virtualprotect_failed;

    return RPL_STATUS_SUCCESS;
virtualprotect_failed:
    VirtualFree(dispatchTable, 0, MEM_RELEASE);
    VirtualFree(codePage, 0, MEM_RELEASE);
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