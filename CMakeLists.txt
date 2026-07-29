cmake_minimum_required(VERSION 3.20)
project(RelPatchLib LANGUAGES C ASM)

set(ZYDIS_FEATURE_ENCODER OFF CACHE BOOL "Disable Zydis Encoder")
add_subdirectory(Zydis)

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
target_link_libraries(RelPatchLib PRIVATE Zydis)