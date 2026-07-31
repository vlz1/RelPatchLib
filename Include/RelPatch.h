#ifndef RELPATCH_H
#define RELPATCH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#if defined(_MSC_VER) && !defined(__clang__)
    #if defined(_M_X64)
        #define RPL_X86_64 1
    #elif defined(_M_IX86)
        #define RPL_X86_32 1
    #else
        #error RelPatchLib only supports x86 and x86_64
    #endif
    #define RPL_NO_INLINE __declspec(noinline)
#else
    #if UINTPTR_MAX == UINT64_MAX
        #define RPL_X86_64 1
    #else
        #define RPL_X86_32 1
    #endif
    #define RPL_NO_INLINE __attribute__((noinline))
#endif

#if defined(RPL_X86_64)
    #if defined(_WIN64)
        #define RPL_DEFAULT_CONVENTION RPL_CALL_X64_MS_ABI
    #else
        #define RPL_DEFAULT_CONVENTION RPL_CALL_X64_SYSV_ABI
    #endif
#else
    #define RPL_DEFAULT_CONVENTION RPL_CALL_X86_CDECL
#endif

#define RPL_PAGE_SIZE 4096
#define RPL_PAGE_MASK ((uintptr_t)(RPL_PAGE_SIZE - 1))
#define RPL_CODE_SIGNATURE 0xC001C0DEU
#define RPL_DISPATCH_TABLE_PAGES 4
#define RPL_DISPATCH_TABLE_ENTRIES (((RPL_PAGE_SIZE * RPL_DISPATCH_TABLE_PAGES) - 8) / sizeof(RPLDispatchEntry))

typedef enum
{
    RPL_STATUS_SUCCESS,
    RPL_STATUS_PLATFORM_ERROR,
    RPL_STATUS_INVALID_ARGUMENT,
    RPL_STATUS_PATTERN_MISMATCH,
    RPL_STATUS_PATTERN_MALFORMED,
    RPL_STATUS_ALLOCATION_FAILED,
    RPL_STATUS_RELOCATION_FAILED,
    RPL_STATUS_DISASSEMBLY_FAILED,
    RPL_STATUS_VIRTUALPROTECT_FAILED,
    RPL_STATUS_DYNAMIC_CODE_PROHIBITED,
    RPL_STATUS_INVALID_CALLING_CONVENTION
} RPLStatus;

typedef intptr_t RPLRegisterInt;
typedef uintptr_t RPLRegisterUInt;

#define RPL_ALIGN_UP(x, multiple) (((uintptr_t)(x) + (multiple) - 1) & ~((multiple) - 1))
#define RPL_SUCCESSFUL(x) ((x) == RPL_STATUS_SUCCESS)

const char* RPLGetStatusString(RPLStatus status);

typedef enum
{
    RPL_CALL_X64_MS_ABI = 0,
    RPL_CALL_X64_SYSV_ABI = 1,
    RPL_CALL_X86_CDECL = 2,
    RPL_CALL_X86_STDCALL = 3
} RPLConvention;

typedef struct RPLHookContext RPLHookContext;

bool RPLIsEpilogueContext(RPLHookContext* context);

RPLRegisterInt RPLGetIntArg(RPLHookContext* context, int argIndex);
void RPLSetIntArg(RPLHookContext* context, int argIndex, RPLRegisterInt value);

RPLRegisterUInt RPLGetUIntArg(RPLHookContext* context, int argIndex);
void RPLSetUIntArg(RPLHookContext* context, int argIndex, RPLRegisterUInt value);

char* RPLGetStringArg(RPLHookContext* context, int argIndex);
void RPLSetStringArg(RPLHookContext* context, int argIndex, char* value);

float RPLGetFloatArg(RPLHookContext* context, int argIndex);
void RPLSetFloatArg(RPLHookContext* context, int argIndex, float value);

double RPLGetDoubleArg(RPLHookContext* context, int argIndex);
void RPLSetDoubleArg(RPLHookContext* context, int argIndex, double value);

typedef void(*RPLHookFunc)(RPLHookContext* context, void* userData);

RPLStatus RPLInstallPrologueHookEx(void* function, RPLHookFunc hook, RPLConvention callingConvention, uint32_t priority, void* userData);
RPLStatus RPLInstallPrologueHook(void* function, RPLHookFunc hook, RPLConvention callingConvention);

// Allocate n read/write pages within +/- 2 GB of the given address. Returns NULL if no pages could be allocated.
void* RPLAllocatePagesWithin2GB(void* address, size_t nPages);
// Disassemble n instructions and write them into a temporary buffer. Returned buffer must be deallocated with free().
char* RPLDumpInstructions(void* address, int nInstructions, bool withAddresses);
// Get the target of a Rel32 Near JMP. Returns NULL if there's no JMP at the given address.
uint8_t* RPLGetJumpTargetRel32(void* address);
// Get the target of a Rel32 Near CALL. Returns NULL if there's no CALL at the given address.
uint8_t* RPLGetCallTargetRel32(void* address);

static inline int32_t RPLCalculateRel32FromAddress(uint8_t* instructionAddress, uint32_t instructionLength, uint8_t* target)
{
    ptrdiff_t rel = (ptrdiff_t)target - (ptrdiff_t)(instructionAddress + instructionLength);
    if (rel > INT32_MAX)
        rel = INT32_MAX;
    else if (rel < INT32_MIN)
        rel = INT32_MIN;
    return (int32_t)rel;
}

static inline uint8_t* RPLCalculateAddressFromRel32(uint8_t* instructionAddress, uint32_t instructionLength, int32_t rel32)
{
    return (uint8_t*)(instructionAddress + instructionLength) + rel32;
}

static inline bool RPLIsAddressPageAligned(void* address)
{
    return ((uintptr_t)address & RPL_PAGE_MASK) == 0;
}

typedef struct RPLCompiledPattern RPLCompiledPattern;

// Compile a pattern into a compact representation and retrieve information about any errors.
RPLStatus RPLCompilePatternEx(const char* pattern, RPLCompiledPattern** compiledPatternOut, char** errorOut, int* errorIndexOut);
// Compile a pattern into a compact representation.
RPLStatus RPLCompilePattern(const char* pattern, RPLCompiledPattern** compiledPatternOut);
// Check if the data in the buffer matches the given pattern and retrieve the position of the first mismatched byte.
// Returns RPL_STATUS_SUCCESS if it matches, RPL_STATUS_PATTERN_MISMATCH otherwise.
RPLStatus RPLPatternCompareEx(RPLCompiledPattern* compiledPattern, const void* buffer, uint32_t bufferSize, uint32_t* firstByteMismatchOut);
// Check if the data in the buffer matches the given pattern.
// Returns RPL_STATUS_SUCCESS if it matches, RPL_STATUS_PATTERN_MISMATCH otherwise.
RPLStatus RPLPatternCompare(RPLCompiledPattern* compiledPattern, const void* buffer, uint32_t bufferSize);
// Search the buffer for the given pattern.
// Returns RPL_STATUS_SUCCESS if a match was found, RPL_STATUS_PATTERN_MISMATCH otherwise.
RPLStatus RPLPatternSearch(RPLCompiledPattern* compiledPattern, const void* buffer, uint32_t bufferSize, uint32_t* matchStartByteOut);
// Free a compiled pattern from RPLCompilePattern or RPLCompilePatternEx.
RPLStatus RPLFreeCompiledPattern(RPLCompiledPattern* compiledPattern);

#ifdef __cplusplus
}
#endif

#endif