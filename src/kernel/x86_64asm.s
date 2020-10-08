bits 64

section .text

global asm_outb
global asm_outw
global asm_outd
global asm_outq

asm_outb:
    push rdx
    push rax

    mov dx, di
    mov rax, rsi
    and rax, 0x00000000000000FF

    out dx, al

    pop rax
    pop rdx

    ret

asm_outw:
    push rdx
    push rax

    mov dx, di
    mov rax, rsi
    and rax, 0x00000000000000FF

    out dx, al

    pop rax
    pop rdx

    ret

asm_outd:
    push rdx
    push rax

    mov dx, di
    mov rax, rsi
    and rax, 0x00000000000000FF

    out dx, al

    pop rax
    pop rdx

    ret

asm_outq:

    call asm_outb

    shr rsi, 8
    call asm_outb

    shr rsi, 8
    call asm_outb

    shr rsi, 8
    call asm_outb

    shr rsi, 8
    call asm_outb

    shr rsi, 8
    call asm_outb

    shr rsi, 8
    call asm_outb

    shr rsi, 8
    call asm_outb

    ret