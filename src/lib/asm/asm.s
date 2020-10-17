    section .apbootstrap
    bits 16
    align 4096

    global APBootstrap
APBootstrap:
    ;   Just some random instructions to use 100% of the core
    mov ax, 0x16
    mov dx, ax
    xchg dx, cx
    jmp APBootstrap

    section .text
    bits 64

; ----------------------------------------------------------
;               C-callable asm functions                   |
; ----------------------------------------------------------

    global sti
    global cli
sti:
    sti
    ret

cli:
    cli
    ret

    global hlt
hlt:
    hlt
    jmp hlt

    global cpuid
cpuid:
    mov rax, [rdi]
    mov rbx, [rsi]
    mov r9, rdx
    mov r8, rcx
    mov rcx, [r9]
    mov rdx, [r8]

    cpuid

    mov [rdi], rax
    mov [rsi], rbx
    mov [r9], rcx
    mov [r8], rdx

    ret

    global ReadMSR
ReadMSR:
    xor rdx, rdx
    xor rax, rax

    mov rcx, rdi    ;   MSRID
    rdmsr

    shl rdx, 32
    or rdx, rax
    mov [rsi], rdx

    ret

    global SetMSR
SetMSR:
    xor rdx, rdx
    xor rax, rax

    ;   Place rsi into edx:eax
    mov rax, rsi
    mov rbx, 0x00000000FFFFFFFF
    and rax, rbx

    shr rsi, 32
    mov rdx, rsi

    mov rcx, rdi    ;   MSRID
    wrmsr

    ret

    global IOWait
IOWait:
    jmp .1
.1:
    jmp .2
.2:
    ret

    global ReadRFLAGS
ReadRFLAGS:
    pushf   ; push RFLAGS onto stack
    mov rsi, [rsp+0]
    mov [rdi], rsi
    popf
    ret

    section .bss
    align 16
SSEINFO: resb 512
    section .text

;   Sets up floating point math
    global EnableSSE
EnableSSE:
    ;   Enable floating point
    mov rax, cr0
    mov rbx, 0xFFFFFFFFFFFFFFFB
    and rax, rbx            ; Clear the EM flag
    or rax, 0x00000002      ; Set the MP flag
    mov cr0, rax

    ;   Set OSFXSR, OSXMMEXCPT bits
    mov rax, cr4
    or rax, 3 << 9

    ;   OSXSAVE bit
    ;or rax, 1 << 18

    mov cr4, rax

    ;   Save the current FPU state
    fxsave [SSEINFO]

    ;   Grab the MXCSR value
    mov eax, [SSEINFO + 0x18]

    ;   Mask all floating point exeptions
    or eax, 0x00001F80
    mov [SSEINFO + 0x18], eax

    fxrstor [SSEINFO]

    ret

    global inb
inb:
    xor rax, rax
    mov dx, di  ;  Port #
    in al, dx

    ret ; Returns al in C

    global outb
outb:
    push rdx
    push rax

    mov dx, di
    mov rax, rsi
    and rax, 0x00000000000000FF

    out dx, al

    pop rax
    pop rdx

    ret
