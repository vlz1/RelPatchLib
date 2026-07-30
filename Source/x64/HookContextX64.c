#include "RelPatch.h"
#include <xmmintrin.h>
#include <emmintrin.h>

struct RPLHookContext
{
    RPLConvention callingConvention;
    uint32_t reserved;
    RPLRegisterInt* stackArgs;
    RPLRegisterInt* integerArgs;
    __m128* vectorArgs;
};

static void* RPLGetIntArgLocation(RPLHookContext* context, int argIndex)
{
    if (context->callingConvention == RPL_CALL_X64_MS_ABI)
    {
        if (argIndex < 4)
            return &context->integerArgs[argIndex];
        return &context->stackArgs[argIndex - 4];
    }
    else if (context->callingConvention == RPL_CALL_X64_SYSV_ABI)
    {
        if (argIndex < 6)
            return &context->integerArgs[argIndex];
        return &context->stackArgs[argIndex - 6];
    }
    return NULL;
}

RPLRegisterInt RPLGetIntArg(RPLHookContext* context, int argIndex)
{
    RPLRegisterInt* pointer = RPLGetIntArgLocation(context, argIndex);
    return *pointer;
}

void RPLSetIntArg(RPLHookContext* context, int argIndex, RPLRegisterInt value)
{
    RPLRegisterInt* pointer = RPLGetIntArgLocation(context, argIndex);
    *pointer = value;
}

RPLRegisterUInt RPLGetUIntArg(RPLHookContext* context, int argIndex)
{
    RPLRegisterUInt* pointer = RPLGetIntArgLocation(context, argIndex);
    return *pointer;
}

void RPLSetUIntArg(RPLHookContext* context, int argIndex, RPLRegisterUInt value)
{
    RPLRegisterUInt* pointer = RPLGetIntArgLocation(context, argIndex);
    *pointer = value;
}

char* RPLGetStringArg(RPLHookContext* context, int argIndex)
{
    RPLRegisterUInt* pointer = RPLGetIntArgLocation(context, argIndex);
    return (char*)*pointer;
}

void RPLSetStringArg(RPLHookContext* context, int argIndex, char* value)
{
    RPLRegisterUInt* pointer = RPLGetIntArgLocation(context, argIndex);
    *pointer = (RPLRegisterUInt)value;
}

float RPLGetFloatArg(RPLHookContext* context, int argIndex)
{
    if (context->callingConvention == RPL_CALL_X64_MS_ABI)
    {
        if (argIndex < 4)
            return _mm_cvtss_f32(context->vectorArgs[argIndex]);
        return *(float*)&context->stackArgs[argIndex - 4];
    }
    // TODO: SysV
    return 0.0f;
}

void RPLSetFloatArg(RPLHookContext* context, int argIndex, float value)
{
    if (context->callingConvention == RPL_CALL_X64_MS_ABI)
    {
        if (argIndex < 4)
        {
            context->vectorArgs[argIndex] = _mm_set_ss(value);
            return;
        }
        *(float*)&context->stackArgs[argIndex - 4] = value;
    }
    // TODO: SysV
}

double RPLGetDoubleArg(RPLHookContext* context, int argIndex)
{
    if (context->callingConvention == RPL_CALL_X64_MS_ABI)
    {
        if (argIndex < 4)
            return _mm_cvtsd_f64(_mm_castps_pd(context->vectorArgs[argIndex]));
        return *(double*)&context->stackArgs[argIndex - 4];
    }
    // TODO: SysV
    return 0.0;
}

void RPLSetDoubleArg(RPLHookContext* context, int argIndex, double value)
{
    if (context->callingConvention == RPL_CALL_X64_MS_ABI)
    {
        if (argIndex < 4)
        {
            context->vectorArgs[argIndex] = _mm_castpd_ps(_mm_set_sd(value));
            return;
        }
        *(double*)&context->stackArgs[argIndex - 4] = value;
    }
    // TODO: SysV
}