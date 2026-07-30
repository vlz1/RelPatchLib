#include "RelPatch.h"
#include "TestCommon.h"
#include <stdio.h>
#include <stdint.h>

static volatile int s_PrologueHookCalled = 0;
static volatile int s_PrologueHookIntArgsCorrect = 0;
static volatile int s_PrologueHookFloatArgsCorrect = 0;
static volatile int s_PrologueHookStackArgsCorrect = 0;

typedef void (*TargetFunctionPtr)(int a, int b, float c, float d, int e, int f, int g, int h);

RPL_NO_INLINE void PrologueHookTargetFunction(int a, int b, float c, float d, int e, int f, int g, int h)
{
    printf("%d %d %f %f %d %d %d %d\n", a, b, c, d, e, f, g, h);
}

RPL_NO_INLINE void PrologueHookFunction(RPLHookContext* context, void* userData)
{
    s_PrologueHookCalled = 1;

    int a = RPLGetIntArg(context, 0);
    int b = RPLGetIntArg(context, 1);
    if (a == 1 && b == 2)
        s_PrologueHookIntArgsCorrect = 1;

    float c = RPLGetFloatArg(context, 2);
    float d = RPLGetFloatArg(context, 3);
    if (c == 3.0f && d == 4.0f)
        s_PrologueHookFloatArgsCorrect = 1;

    int g = RPLGetIntArg(context, 6);
    int h = RPLGetIntArg(context, 7);
    if (g == 7 && h == 8)
        s_PrologueHookStackArgsCorrect = 1;
}

bool TEST_PrologueHook()
{
    s_PrologueHookCalled = 0;
    s_PrologueHookIntArgsCorrect = 0;
    s_PrologueHookFloatArgsCorrect = 0;
    s_PrologueHookStackArgsCorrect = 0;

    // Volatile pointer is necessary to stop GCC from being a brat and duplicating the function on optimized builds.
    // You'd think __attribute__((noinline)) would already imply that I only want one instance of it, but apparently not.
    volatile TargetFunctionPtr target = PrologueHookTargetFunction;
    RPLStatus status = RPLInstallPrologueHook(
        target,
        PrologueHookFunction,
        RPL_DEFAULT_CONVENTION
    );
    if (!RPL_SUCCESSFUL(status))
    {
        PrintError("RPLInstallPrologueHook: %s", RPLGetStatusString(status));
        return false;
    }

    target(1, 2, 3.0f, 4.0f, 5, 6, 7, 8);

    if (!s_PrologueHookCalled)
    {
        PrintError("Prologue hook wasn't called");
        return false;
    }

    if (!s_PrologueHookIntArgsCorrect)
    {
        PrintError("Prologue hook received wrong integer arguments");
        return false;
    }

    if (!s_PrologueHookFloatArgsCorrect)
    {
        PrintError("Prologue hook received wrong floating-point arguments");
        return false;
    }

    if (!s_PrologueHookStackArgsCorrect)
    {
        PrintError("Prologue hook received wrong stack arguments");
        return false;
    }

    return true;
}