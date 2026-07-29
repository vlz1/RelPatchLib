#include "RelPatch.h"
#include "RelDispatch.h"

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