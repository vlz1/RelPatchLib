#include "RelPlatform.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static inline HMODULE RPLWin32GetModuleFromAddress(void* address)
{
    MEMORY_BASIC_INFORMATION info = { 0 };
    if (VirtualQueryEx(GetCurrentProcess(), address, &info, sizeof(info)) == 0)
        return NULL;

    if (info.State != MEM_COMMIT || info.Type != MEM_IMAGE)
        return NULL;

    return info.AllocationBase;
}

static inline DWORD RPLWin32FromPageProtection(RPLPageProtect protection)
{
    if (protection == RPL_PAGE_READ)
        return PAGE_READONLY;
    else if (protection == RPL_PAGE_READ_WRITE)
        return PAGE_READWRITE;
    else if (protection == RPL_PAGE_READ_EXECUTE)
        return PAGE_EXECUTE_READ;
    else if (protection == RPL_PAGE_READ_WRITE_EXECUTE)
        return PAGE_EXECUTE_READWRITE;
    return 0;
}

static inline RPLPageProtect RPLWin32ToPageProtection(DWORD flProtect)
{
    if (flProtect == PAGE_READONLY)
        return RPL_PAGE_READ;
    else if (flProtect == PAGE_READWRITE)
        return RPL_PAGE_READ_WRITE;
    else if (flProtect == PAGE_EXECUTE_READ)
        return RPL_PAGE_READ_EXECUTE;
    else if (flProtect == PAGE_EXECUTE_READWRITE)
        return RPL_PAGE_READ_WRITE_EXECUTE;
    return 0;
}

typedef BOOL(WINAPI *pfnGetProcessMitigationPolicy)(
    HANDLE                    hProcess,
    PROCESS_MITIGATION_POLICY MitigationPolicy,
    PVOID                     lpBuffer,
    SIZE_T                    dwLength
);

static bool s_SearchedForGetProcessMitigationPolicy = false;
static pfnGetProcessMitigationPolicy s_fnGetProcessMitigationPolicy = NULL;

bool RPTWin32IsLargeAddressAware()
{
    HMODULE hModule = GetModuleHandleW(NULL);
    if (!hModule)
        return 0;

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    return (ntHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) != 0;
}

bool RPTWin32IsDynamicCodeAvailable()
{
    if (s_fnGetProcessMitigationPolicy == NULL)
    {
        if (s_SearchedForGetProcessMitigationPolicy)
            return true;
        s_SearchedForGetProcessMitigationPolicy = true;

        HMODULE kernel32 = GetModuleHandle(TEXT("KERNEL32.dll"));
        if (kernel32 == NULL)
            return false;
        s_fnGetProcessMitigationPolicy = (pfnGetProcessMitigationPolicy)GetProcAddress(kernel32, "GetProcessMitigationPolicy");
        if (s_fnGetProcessMitigationPolicy == NULL)
            return true; // Process mitigation policies aren't even implemented, so we're fine
    }

    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY policy = { 0 };
    if (!s_fnGetProcessMitigationPolicy(
        GetCurrentProcess(),
        ProcessDynamicCodePolicy,
        &policy,
        sizeof(policy)))
    {
        return false;
    }

    if (policy.ProhibitDynamicCode)
        return false;

    return true;
}

RPLStatus RPLPlatformVirtualAlloc(void* baseAddress, size_t bytes, RPLPageProtect protection, void** addressOut)
{
    if (addressOut == NULL)
        return RPL_STATUS_INVALID_ARGUMENT;

    DWORD flProtect = RPLWin32FromPageProtection(protection);
    if (flProtect == 0)
        return RPL_STATUS_INVALID_ARGUMENT;

    *addressOut = VirtualAlloc(baseAddress, bytes, MEM_COMMIT | MEM_RESERVE, flProtect);
    return (*addressOut != NULL)
        ? RPL_STATUS_SUCCESS
        : RPL_STATUS_PLATFORM_ERROR;
}

RPLStatus RPLPlatformVirtualFree(void* baseAddress, size_t bytes)
{
    // dwSize must be 0 with MEM_RELEASE, so the "bytes" argument doesn't matter
    return VirtualFree(baseAddress, 0, MEM_RELEASE) == TRUE
        ? RPL_STATUS_SUCCESS
        : RPL_STATUS_PLATFORM_ERROR;
}

RPLStatus RPLPlatformVirtualProtect(void* baseAddress, size_t bytes, RPLPageProtect protection, RPLPageProtect* oldProtectionOut)
{
    DWORD flProtect = RPLWin32FromPageProtection(protection);
    if (flProtect == 0)
        return RPL_STATUS_INVALID_ARGUMENT;

    DWORD flOldProtect = 0;
    if (!VirtualProtect(baseAddress, bytes, flProtect, &flOldProtect))
        return RPL_STATUS_PLATFORM_ERROR;

    *oldProtectionOut = RPLWin32ToPageProtection(flOldProtect);
    return RPL_STATUS_SUCCESS;
}

RPLStatus RPLPlatformAllocPagesWithin2GB(void* baseAddress, size_t nPages, void** addressOut)
{
    if (addressOut == NULL)
        return RPL_STATUS_INVALID_ARGUMENT;

    const ptrdiff_t maxDistance = 0x7FFFFFFF;
#if defined(RPL_X86_64)
    HMODULE hModule = RPLWin32GetModuleFromAddress(baseAddress);
    if (hModule == NULL)
    {
        // Address doesn't belong to a module...
        // Might be trying to allocate near JIT'd code?
        // That's not supported yet, so just fail.
        return RPL_STATUS_ALLOCATION_FAILED;
    }

    // On x86_64, there are usually multi-gigabyte gaps between modules.
    // We try to allocate there first, then fall back to more cheeky methods if that fails.
    MEMORY_BASIC_INFORMATION info = { 0 };
    size_t requestedBytes = nPages * RPL_PAGE_SIZE;
    uint8_t* searchAddress = (uint8_t*)hModule;
    for (int retry = 0; retry < 64; ++retry)
    {
        searchAddress -= requestedBytes;
        ptrdiff_t absoluteDistance = searchAddress - (uint8_t*)baseAddress;
        if (absoluteDistance < 0)
            absoluteDistance = -absoluteDistance;
        if (absoluteDistance > maxDistance)
            break; // Distance out of range.

        if (VirtualQueryEx(GetCurrentProcess(), searchAddress, &info, sizeof(info)) == 0)
            return RPL_STATUS_PLATFORM_ERROR; // That's a problem.

        if (info.State != MEM_FREE)
        {
            // Hit another allocation, start from its base and keep working backwards.
            searchAddress = info.AllocationBase;
            continue;
        }

        size_t freePages = info.RegionSize / RPL_PAGE_SIZE;
        if (freePages < nPages)
            continue; // Free space is too small.

        void* ptr = VirtualAlloc(searchAddress, requestedBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (ptr == NULL)
            continue; // Allocation failed, retry.
        *addressOut = ptr;
        return RPL_STATUS_SUCCESS;
    }

    // TODO: Figure something else out...
    // Maybe fall back to VirtualAlloc2 with MEM_ADDRESS_REQUIREMENTS if it's available?
    return RPL_STATUS_ALLOCATION_FAILED;
#elif defined(RPL_X86_32)
    // The address space is much more tightly packed on x86_32.
    // Since there's only 2 GB of user address space, we can allocate from anywhere.
    // TODO: This might work differently if the process is Large Address Aware
    void* ptr = VirtualAlloc(NULL, nPages * PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (ptr == NULL)
        return RPL_STATUS_ALLOCATION_FAILED;
    *addressOut = ptr;
    return RPL_STATUS_SUCCESS;
#endif
}