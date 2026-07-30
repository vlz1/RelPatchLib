#include "TestCommon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define TEST_COUNT (sizeof(s_Tests) / sizeof(Test))

bool TEST_ExactPatternMatch();
bool TEST_PrologueHook();

static const Test s_Tests[] = {
    { "ExactPatternMatch", TEST_ExactPatternMatch },
    { "PrologueHook", TEST_PrologueHook }
};

void PrintUsage(const char* name)
{
    printf("Usage: %s [test]\n", name);
    puts("Available tests:");
    for (int i = 0; i < TEST_COUNT; ++i)
    {
        printf("  %s\n", s_Tests[i].name);
    }
}

void PrintMessage(const char* prefix, const char* fmt, va_list args)
{
    char buffer[2048] = { 0 };
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    fprintf(stderr, "%s: %s\n", prefix, buffer);
}

void PrintInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PrintMessage("INFO", fmt, args);
    va_end(args);
}

void PrintError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PrintMessage("ERROR", fmt, args);
    va_end(args);
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < TEST_COUNT; ++i)
    {
        if (strcmp(argv[1], s_Tests[i].name) != 0)
            continue;
        if (!s_Tests[i].function())
        {
            PrintError("Test \"%s\" failed.", argv[1]);
            return EXIT_FAILURE;
        }
        PrintInfo("Test \"%s\" passed.", argv[1]);
        return EXIT_SUCCESS;
    }

    PrintError("Test \"%s\" not found!", argv[1]);
    return 0;
}