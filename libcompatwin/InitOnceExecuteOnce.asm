;;; nasm -f win32 InitOnceExecuteOnce.asm

    global __imp__InitOnceExecuteOnce@16
    global InitOnceExecuteOnce
    global _InitOnceExecuteOnce@16

    extern __InitOnceExecuteOnce@16
    extern _compatwin_init_ptr

    section .data

    ;; pointer of InitOnceExecuteOnce(), stdcall style
__imp__InitOnceExecuteOnce@16:
    dd  init


    section .text

    ;; call api pointer initializer
init:
    pusha
    push dword __imp__InitOnceExecuteOnce@16     ; place to save the pointer
    push dword __InitOnceExecuteOnce@16          ; wrapper
    push dword api_str
    push dword Kernel32_dll_str
    call _compatwin_init_ptr
    add  esp,byte 16
    popa

    ;; call api
InitOnceExecuteOnce:
_InitOnceExecuteOnce@16:
    jmp [__imp__InitOnceExecuteOnce@16]


    section .data
Kernel32_dll_str:
    dw  __utf16__("Kernel32.dll"), 0
api_str:
    db  "InitOnceExecuteOnce",0
