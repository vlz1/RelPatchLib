#include "RelPatch.h"
#include "RelDispatch.h"
#include "RelPlatform.h"
#include <stdio.h>

RPLThreadContext* RPLDispatchCommon(RPLCodePage* codePage, RPLHookContext* context, void* returnAddress)
{
    RPLDispatchTable* dispatchTable = codePage->meta.dispatchTable;
    RPLThreadContext* threadContext = NULL;

    printf("DISPATCH: %p\n", threadContext);
    if (RPLIsEpilogueContext(context))
    {
        for (uint16_t i = 0; i < dispatchTable->nDispatchEntries; ++i)
        {
            RPLDispatchEntry* entry = &dispatchTable->entries[i];
            if (!entry->epilogue)
                continue;
            entry->epilogue(context, entry->userData);
        }
        RPLPlatformGetThreadContext(&threadContext);
        printf("EPILOGUE: return to %p\n", returnAddress);
        fflush(stdout);
        return threadContext;
    }

    for (uint16_t i = 0; i < dispatchTable->nDispatchEntries; ++i)
    {
        RPLDispatchEntry* entry = &dispatchTable->entries[i];
        if (!entry->prologue)
            continue;
        entry->prologue(context, entry->userData);
    }
    printf("PROLOGUE: return to %p\n", returnAddress);
    fflush(stdout);
    if (dispatchTable->flags & RPL_DISPATCH_HAS_EPILOGUE)
    {
        RPLPlatformGetThreadContext(&threadContext);
        threadContext->epilogueReturnAddress = returnAddress;
    }
    return NULL;
}

RPLStatus RPLAppendHookToDispatch(RPLCodePage* codePage, RPLHookFunc prologueHook, RPLHookFunc epilogueHook, uint32_t priority, void* userData)
{
    RPLDispatchTable* dispatchTable = codePage->meta.dispatchTable;
    if (epilogueHook != NULL)
        dispatchTable->flags |= RPL_DISPATCH_HAS_EPILOGUE;

    if (dispatchTable->nDispatchEntries == 0)
    {
        ++dispatchTable->nDispatchEntries;
        dispatchTable->entries[0] = (RPLDispatchEntry){ 
            .prologue = prologueHook,
            .epilogue = epilogueHook,
            .userData = userData,
            .priority = priority
        };
        return RPL_STATUS_SUCCESS;
    }

    // TODO: Binary search for a place to put the new dispatch entry

    return RPL_STATUS_SUCCESS;
}