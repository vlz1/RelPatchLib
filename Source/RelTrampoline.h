#ifndef RELTRAMPOLINE_H
#define RELTRAMPOLINE_H

#include "RelPatch.h"
#include <stdint.h>

typedef struct
{
    uint8_t* code;
    uint32_t codeLength;
    uint32_t callOffset;
    uint32_t callRel32Offset;
} RPLTrampoline;

RPLStatus RPLGetTrampoline(RPLConvention callingConvention, RPLTrampoline* trampolineOut);

#endif