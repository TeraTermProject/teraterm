;;; nasm -f win32 BCryptGenRandom.asm

    global __imp__BCryptGenRandom@16
    global BCryptGenRandom
    global _BCryptGenRandom@16

    extern __BCryptGenRandom@16
    extern _compatwin_init_ptr

    section .data

    ;; pointer of BCryptGenRandom(), stdcall style
__imp__BCryptGenRandom@16:
    dd  init


    section .text

    ;; call api pointer initializer
init:
    pusha
    push dword __imp__BCryptGenRandom@16     ; place to save the pointer
    push dword __BCryptGenRandom@16          ; wrapper
    push dword api_str
    push dword Bcrypt_dll_str
    call _compatwin_init_ptr
    add  esp,byte 16
    popa

    ;; call api
BCryptGenRandom:
_BCryptGenRandom@16:
    jmp [__imp__BCryptGenRandom@16]


    section .data
Bcrypt_dll_str:
    dw  __utf16__("Bcrypt.dll"), 0
api_str:
    db  "BCryptGenRandom",0
