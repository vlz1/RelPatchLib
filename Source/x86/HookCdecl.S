.extern RPTDispatchCommon

.global RPTPrologueHookCdecl_x86
RPTPrologueHookCdecl_x86:
    ; Find what code page we're on
    call _get_eip
_get_eip:
    pop eax
    sub eax, 5

    lea edx, dword ptr [esp + 8]

    push edx
    push eax
    call dword ptr [RPTDispatchCommon_ptr]
    add esp, 8
    
    ; Return jump is added after the relocated code 

.global RPTPrologueHookCdecl_x86_Length
RPTPrologueHookCdecl_x86_Length:
    .long . - RPTPrologueHookCdecl_x86

RPTDispatchCommon_ptr:
    .long RPTDispatchCommon