cmake_minimum_required(VERSION 3.20)
project(RelPatchLib LANGUAGES C ASM)

set(ZYDIS_FEATURE_ENCODER OFF CACHE BOOL "Disable Zydis Encoder")
add_subdirectory(Zydis)

option(RPL_BUILD_TESTS "Build unit tests" ON)

add_library(RelPatchLib STATIC
    Include/RelPatch.h
    Source/RelPatch.c
    Source/RelX86Util.c
    Source/RelX86Util.h
    Source/RelDispatch.c
    Source/RelDispatch.h
    Source/Pattern.c
    Source/RelPlatform.h)

if (WIN32)
    target_sources(RelPatchLib PRIVATE Source/RelPlatformWin32.c)
endif ()

target_include_directories(RelPatchLib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Include)
target_link_libraries(RelPatchLib PUBLIC Zydis)

if (RPL_BUILD_TESTS)
    add_executable(RelPatchTests
        Tests/Entry.c
        Tests/Pattern.c)
    target_link_libraries(RelPatchTests PRIVATE RelPatchLib)

    enable_testing()
    add_test(NAME ExactPatternMatch COMMAND RelPatchTests ExactPatternMatch)
endif ()