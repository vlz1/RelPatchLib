#include "RelPatch.h"
#include "RelDispatch.h"
#include "RelPlatform.h"
#include <stdio.h>

RPLThreadContext* RPLDispatchCommon(RPLCodePage* codePage, RPLHookContext* context, void* returnAddress)
{
    RPLDispatchTable* dispatchTable = codePage->meta.dispatchTable;
    RPLThreadContext* threadContext = NULL;
    RPLPlatformGetThreadContext(&threadContext);

    printf("DISPATCH: %p\n", threadContext);
    if (!RPLIsEpilogueContext(context))
    {
        for (uint16_t i = 0; i < dispatchTable->nDispatchEntries; ++i)
        {
            RPLDispatchEntry* entry = &dispatchTable->entries[i];
            entry->function(context, entry->userData);
        }
        printf("PROLOGUE: return to %p\n", returnAddress);
        fflush(stdout);
        threadContext->epilogueReturnAddress = returnAddress;
        return NULL;
    }
    else
    {
        printf("EPILOGUE: return to %p\n", threadContext->epilogueReturnAddress);
        fflush(stdout);
        return threadContext;
    }
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