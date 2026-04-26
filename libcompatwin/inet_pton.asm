;;; nasm -f win32 inet_pton.asm

    global __imp__inet_pton@12
    global inet_pton
    global _inet_pton@12

    extern __inet_pton@12
    extern _compatwin_init_ptr

    section .data

    ;; pointer of inet_pton(), stdcall style
__imp__inet_pton@12:
    dd  init


    section .text

    ;; call api pointer initializer
init:
    pusha
    push dword __imp__inet_pton@12     ; place to save the pointer
    push dword __inet_pton@12          ; wrapper
    push dword api_str
    push dword Ws2_32_dll_str
    call _compatwin_init_ptr
    add  esp,byte 16
    popa

    ;; call api
inet_pton:
_inet_pton@12:
    jmp [__imp__inet_pton@12]


    section .data
Ws2_32_dll_str:
    dw  __utf16__("Ws2_32.dll"), 0
api_str:
    db  "inet_pton",0
