#ifndef RELPLATFORM_H
#define RELPLATFORM_H

#include "RelPatch.h"

typedef enum {
    RPL_PAGE_READ = (1 << 0),
    RPL_PAGE_WRITE = (1 << 1),
    RPL_PAGE_EXECUTE = (1 << 2)
} RPLPageProtect;

RPLStatus RPLPlatformVirtualAlloc(void* address, size_t bytes, RPLPageProtect protection);
RPLStatus RPLPlatformVirtualFree(void* address, size_t bytes);

#endif