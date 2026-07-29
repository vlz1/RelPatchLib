#include "RelPatch.h"
#include "TestCommon.h"
#include <stdint.h>

static const uint8_t s_ExactMatchData[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78
};

bool TEST_ExactPatternMatch()
{
    RPLCompiledPattern* pattern = NULL;
    RPLStatus status = RPLCompilePattern("DE AD BE EF 12 34 56 78", &pattern);
    if (status != RPL_STATUS_SUCCESS)
    {
        PrintError("RPLCompilePattern: %s", RPLGetStatusString(status));
        return false;
    }

    status = RPLPatternCompare(pattern, s_ExactMatchData, sizeof(s_ExactMatchData));
    if (status != RPL_STATUS_SUCCESS)
    {
        PrintError("RPLPatternCompare: %s", RPLGetStatusString(status));
        return false;
    }

    return true;
}