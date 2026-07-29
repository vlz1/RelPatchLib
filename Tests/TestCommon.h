#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdbool.h>

typedef bool (*TestFunction)();

typedef struct
{
    const char* name;
    TestFunction function;
} Test;

void PrintInfo(const char* fmt, ...);
void PrintError(const char* fmt, ...);

#endif