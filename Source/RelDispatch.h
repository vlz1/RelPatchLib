#ifndef RELDISPATCH_H
#define RELDISPATCH_H

#include "RelPatch.h"

typedef struct
{
    RPLHookFunc function;
    void* userData;
    int priority;
} RPLDispatchEntry;

typedef struct
{
    uint32_t nDispatchEntries;
    uint32_t nMaxDispatchEntries;
    RPLDispatchEntry entries[RPL_DISPATCH_TABLE_ENTRIES];
} RPLDispatchTable;

typedef struct
{
    uint32_t signature;
    RPLDispatchTable* dispatchTable;
} RPLCodePageMeta;

typedef struct
{
    uint8_t code[1024];
    uint8_t constData[RPL_PAGE_SIZE - 1024 - sizeof(RPLCodePageMeta)];
    RPLCodePageMeta meta;
} RPLCodePage;

RPLStatus RPLDispatchCommon(RPLCodePage* codePage, RPLHookContext* context);
RPLStatus RPLAppendHookToDispatch(RPLCodePage* codePage, RPLHookFunc hook, uint32_t priority, void* userData);

#endif