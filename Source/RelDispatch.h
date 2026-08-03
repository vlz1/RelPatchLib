#ifndef RELDISPATCH_H
#define RELDISPATCH_H

#include "RelPatch.h"
#include "RelPlatform.h"

typedef struct
{
    RPLHookFunc prologue;
    RPLHookFunc epilogue;
    void* userData;
    int priority;
} RPLDispatchEntry;

#define RPL_DISPATCH_HAS_EPILOGUE (1 << 0)

typedef struct
{
    uint16_t flags;
    uint16_t nDispatchEntries;
    uint16_t nMaxDispatchEntries;
    RPLDispatchEntry entries[RPL_DISPATCH_TABLE_ENTRIES];
} RPLDispatchTable;

typedef struct
{
    uint32_t signature;
    RPLConvention callingConvention;
    RPLDispatchTable* dispatchTable;
} RPLCodePageMeta;

typedef struct
{
    uint8_t code[1024];
    uint8_t constData[RPL_PAGE_SIZE - 1024 - sizeof(RPLCodePageMeta)];
    RPLCodePageMeta meta;
} RPLCodePage;

RPLThreadContext* RPLDispatchCommon(RPLCodePage* codePage, RPLHookContext* context, void* returnAddress);
RPLStatus RPLAppendHookToDispatch(RPLCodePage* codePage, RPLHookFunc prologueHook, RPLHookFunc epilogueHook, uint32_t priority, void* userData);

#endif