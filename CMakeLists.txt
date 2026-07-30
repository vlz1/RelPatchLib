cmake_minimum_required(VERSION 3.20)
project(RelPatchLib LANGUAGES C ASM)

option(RPL_BUILD_TESTS  "Build unit tests" ON)
option(RPL_BUILD_X86_32 "Build for 32-bit x86, even if building with a 64-bit compiler" OFF)

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" HOST_PROCESSOR)
if (HOST_PROCESSOR MATCHES "x86" OR HOST_PROCESSOR MATCHES "x86_64" OR HOST_PROCESSOR MATCHES "amd64")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "Target Architecture: x86_64 (64-bit)")
        set(RPL_X86_64 TRUE)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(STATUS "Target Architecture: x86 (32-bit)")
        set(RPL_X86_32 TRUE)
    endif()
endif ()

set(ZYDIS_FEATURE_ENCODER OFF CACHE BOOL "Disable Zydis Encoder")
add_subdirectory(Zydis)

add_library(RelPatchLib STATIC
    Include/RelPatch.h
    Source/RelPatch.c
    Source/RelX86Util.c
    Source/RelX86Util.h
    Source/RelTrampoline.c
    Source/RelTrampoline.h
    Source/RelDispatch.c
    Source/RelDispatch.h
    Source/RelPattern.c
    Source/RelPlatform.h)

if (WIN32)
    target_sources(RelPatchLib PRIVATE Source/RelPlatformWin32.c)
else ()

endif ()

if (RPL_X86_64)
    if (MSVC)
        target_sources(RelPatchLib PRIVATE Source/x64/TrampolineWin64.asm)
    else ()
        target_sources(RelPatchLib PRIVATE Source/x64/TrampolineWin64.S)
    endif ()
    target_sources(RelPatchLib PRIVATE Source/x64/RelocateX64.c Source/x64/HookContextX64.c)
endif ()

target_include_directories(RelPatchLib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Include)
target_link_libraries(RelPatchLib PUBLIC Zydis)

if (RPL_BUILD_TESTS)
    add_executable(RelPatchTests
        Tests/Entry.c
        Tests/Pattern.c
        Tests/Hook.c)
    target_link_libraries(RelPatchTests PRIVATE RelPatchLib)

    enable_testing()
    add_test(NAME ExactPatternMatch COMMAND RelPatchTests ExactPatternMatch)
    add_test(NAME PrologueHook COMMAND RelPatchTests PrologueHook)
endif ()