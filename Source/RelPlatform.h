#ifndef RELPLATFORM_H
#define RELPLATFORM_H

#include "RelPatch.h"

typedef enum {
    RPL_PAGE_READ = (1 << 0),
    RPL_PAGE_WRITE = (1 << 1),
    RPL_PAGE_EXECUTE = (1 << 2),
    RPL_PAGE_READ_WRITE = RPL_PAGE_READ | RPL_PAGE_WRITE,
    RPL_PAGE_READ_EXECUTE = RPL_PAGE_READ | RPL_PAGE_EXECUTE,
    RPL_PAGE_READ_WRITE_EXECUTE = RPL_PAGE_READ | RPL_PAGE_WRITE | RPL_PAGE_EXECUTE
} RPLPageProtect;

typedef struct
{
    void* epilogueReturnAddress;
} RPLThreadContext;

RPLStatus RPLPlatformVirtualAlloc(void* baseAddress, size_t bytes, RPLPageProtect protection, void** addressOut);
RPLStatus RPLPlatformVirtualFree(void* baseAddress, size_t bytes);
RPLStatus RPLPlatformVirtualProtect(void* baseAddress, size_t bytes, RPLPageProtect protection, RPLPageProtect* oldProtectionOut);
RPLStatus RPLPlatformAllocPagesWithin2GB(void* baseAddress, size_t nPages, void** addressOut);
RPLStatus RPLPlatformGetThreadContext(RPLThreadContext** threadContextOut);
void RPLPlatformCleanup();

#endif