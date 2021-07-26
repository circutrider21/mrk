[bits 64]

section .text

global __lgdt
__lgdt:   
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi
    mov rax, 0x08
    push rax
    push rdi
    retfq

; Dummy function to make LD warnings go away
global _start
_start:
    cli
    hlt
    jmp _start

; XSAVE and XRSTOR defs
global asm_xsave
asm_xsave:
    push rax
    push rdx

    xor rax, rax
    xor rdx, rdx
    mov eax, 0xFFFFFFFF ; Set the Requested Feature Bitmap (RFBM) to enable all functions enabled in xcr0
    mov edx, 0xFFFFFFFF

    xsave [rdi]

    pop rdx
    pop rax
    ret

global asm_xrstor
asm_xrstor:
    push rax
    push rdx

    xor rax, rax
    xor rdx, rdx
    mov eax, 0xFFFFFFFF ; Set the Requested Feature Bitmap (RFBM) to enable all functions enabled in xcr0
    mov edx, 0xFFFFFFFF

    xrstor [rdi]

    pop rdx
    pop rdx
    ret
