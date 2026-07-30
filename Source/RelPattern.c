#include "RelPatch.h"
#include <stdlib.h>

#define PATTERN_WILDCARD_BIT 0x80

struct RPLCompiledPattern
{
    uint32_t nibbleCount;
    uint8_t nibbleData[];
};

static inline int32_t HexDigitToNibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return 0x0A + (c - 'A');
    else if (c >= 'a' && c <= 'f')
        return 0x0A + (c - 'a');
    return -1;
}

RPLStatus RPLCompilePatternEx(const char* pattern, RPLCompiledPattern** compiledPatternOut, char** errorOut, int* errorIndexOut)
{
    const char* patternSrc = pattern;
    uint32_t nibbleCount = 0;
    char* errorInternal = NULL;
    int errorIndexInternal = 0;

    if (compiledPatternOut == NULL)
        return RPL_STATUS_INVALID_ARGUMENT;
    if (errorOut == NULL)
        errorOut = &errorInternal;
    if (errorIndexOut == NULL)
        errorIndexOut = &errorIndexInternal;
    *compiledPatternOut = NULL;

    // Find out many nibbles the pattern requires (just count non-space characters)
    while (*patternSrc != '\0')
    {
        char c = *patternSrc++;
        if (c != ' ')
            ++nibbleCount;
    }

    // Allocate space for pattern data
    size_t patternSize = sizeof(RPLCompiledPattern) + nibbleCount;
    RPLCompiledPattern* compiledPattern = malloc(patternSize);
    if (compiledPattern == NULL)
    {
        *errorOut = "Memory allocation failed";
        *errorIndexOut = -1;
        return RPL_STATUS_ALLOCATION_FAILED;
    }
    compiledPattern->nibbleCount = nibbleCount;

    // Parse and compile
    uint32_t charIndex = 0;
    uint32_t nibbleIndex = 0;
    while (*pattern != '\0')
    {
        char c = *pattern;
        if (c == ' ')
            goto next;

        if (c == '?')
        {
            compiledPattern->nibbleData[nibbleIndex++] = PATTERN_WILDCARD_BIT;
            goto next;
        }

        int8_t expected = HexDigitToNibble(c);
        if (expected < 0)
        {
            *errorOut = "Invalid hex digit";
            *errorIndexOut = charIndex;
            return RPL_STATUS_PATTERN_MALFORMED;
        }

        compiledPattern->nibbleData[nibbleIndex++] = expected & 0x0F;
    next:
        ++pattern;
        ++charIndex;
    }

    *compiledPatternOut = compiledPattern;
    return RPL_STATUS_SUCCESS;
}

RPLStatus RPLCompilePattern(const char* pattern, RPLCompiledPattern** compiledPatternOut)
{
    return RPLCompilePatternEx(pattern, compiledPatternOut, NULL, NULL);
}

RPLStatus RPLPatternCompareEx(RPLCompiledPattern* compiledPattern, const void* buffer, uint32_t bufferSize, uint32_t* firstByteMismatchOut)
{
    if (compiledPattern == NULL)
        return RPL_STATUS_INVALID_ARGUMENT;

    uint32_t byteCount = (compiledPattern->nibbleCount >> 1) + (compiledPattern->nibbleCount & 1);
    if (byteCount > bufferSize)
    {
        *firstByteMismatchOut = 0;
        return RPL_STATUS_PATTERN_MISMATCH;
    }

    const uint8_t* data = (const uint8_t*)buffer;
    for (uint32_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
    {
        uint32_t byte = data[byteIndex];
        uint32_t nibbleIndex = byteIndex << 1;

        if ((compiledPattern->nibbleData[nibbleIndex] & PATTERN_WILDCARD_BIT) == 0)
        {
            if ((byte >> 4) != compiledPattern->nibbleData[nibbleIndex])
            {
                *firstByteMismatchOut = byteIndex;
                return RPL_STATUS_PATTERN_MISMATCH;
            }
        }

        if ((nibbleIndex + 1) == compiledPattern->nibbleCount)
            break;

        if ((compiledPattern->nibbleData[nibbleIndex + 1] & PATTERN_WILDCARD_BIT) == 0)
        {
            if ((byte & 0x0F) != compiledPattern->nibbleData[nibbleIndex + 1])
            {
                *firstByteMismatchOut = byteIndex;
                return RPL_STATUS_PATTERN_MISMATCH;
            }
        }
    }
    return RPL_STATUS_SUCCESS;
}

RPLStatus RPLPatternCompare(RPLCompiledPattern* compiledPattern, const void* buffer, uint32_t bufferSize)
{
    uint32_t mismatch = 0;
    return RPLPatternCompareEx(compiledPattern, buffer, bufferSize, &mismatch);
}

RPLStatus RPLPatternSearch(RPLCompiledPattern* compiledPattern, const void* buffer, uint32_t bufferSize, uint32_t* matchStartByteOut)
{
    uint32_t offset = 0;
    const uint8_t* data = (const uint8_t*)buffer;
    while (offset < bufferSize)
    {
        uint32_t remaining = bufferSize - offset;
        RPLStatus status = RPLPatternCompare(compiledPattern, data + offset, remaining);
        if (status == RPL_STATUS_SUCCESS)
        {
            *matchStartByteOut = offset;
            return RPL_STATUS_SUCCESS;
        }
        ++offset;
    }
    return RPL_STATUS_PATTERN_MISMATCH;
}

RPLStatus RPLFreeCompiledPattern(RPLCompiledPattern* compiledPattern)
{
    if (compiledPattern == NULL)
        return RPL_STATUS_INVALID_ARGUMENT;
    free(compiledPattern);
    return RPL_STATUS_SUCCESS;
}