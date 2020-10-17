    section .apbootstrapdata
; -------------------------------------------------
;               IDT and GDT Structures
; -------------------------------------------------
    align 4096
    global IDT64_PTR
IDT64_PTR:
    times 4096 db 0
IDT64_end:

    align 4096
    global IDTR64
IDTR64:
limit:  dw 4095
offset: dq IDT64_PTR

    align 4096
GDT64:                           ; Global Descriptor Table (64-bit).
.Null: equ $ - GDT64         ; The null descriptor.
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 0                         ; Access.
    db 1                         ; Granularity.
    db 0                         ; Base (high).
.Code: equ $ - GDT64         ; The code descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10011010b                 ; Access (exec/read).
    db 10101111b                 ; Granularity, 64 bits flag, limit19:16.
    db 0                         ; Base (high).
.Data: equ $ - GDT64         ; The data descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10010010b                 ; Access (read/write).
    db 00000000b                 ; Granularity.
    db 0                         ; Base (high).
    global GDTR64
GDTR64:                          ; The GDT-pointer.
    dw $ - GDT64 - 1             ; Limit.
    dq GDT64                     ; Base.

    align 4096
GDT32:                           ; Global Descriptor Table (32-bit).
.Null: equ $ - GDT32         ; The null descriptor.
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 0                         ; Access.
    db 1                         ; Granularity.
    db 0                         ; Base (high).
.Code: equ $ - GDT32         ; The code descriptor.
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10011010b                 ; Access (exec/read).
    db 11001111b                 ; Granularity, 64 bits flag, limit19:16. OLD VALUE: 10101111b
    db 0                         ; Base (high).
.Data: equ $ - GDT32         ; The data descriptor.
    dw 0xFFFF                    ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10010010b                 ; Access (read/write).
    db 11001111b                 ; Granularity.
    db 0                         ; Base (high).

GDTR32:                          ; The GDT-pointer.
    dw $ - GDT32 - 1             ; Limit.
    dd GDT32                     ; Base.

    global CR3Value
CR3Value: dd 0

    global pFramebuffer
pFramebuffer: dq 0

    section .apbootstrap
    bits 16

    align 4096
    global APBootstrap
APBootstrap:
    cli

    ; Load the GDT and IDT
    lgdt [GDTR32]

    ; Set Protection Enable (PE) bit
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; Clear 16 bit instruction prefetch queue
    jmp ClearPrefetchQueue
    nop
    nop
ClearPrefetchQueue:
    ; Set data segments to GDT descriptor
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to protected mode
    db 0x66
    db 0xEA
    dd ProtectedMode
    dw 0x0008

    bits 32
ProtectedMode:
    ;   Enable Physical Address Extension (PAE)
    mov eax, cr4
    or eax, 1 << 5  ;   6th bit
    mov cr4, eax

    ;   Address of the PML4
    mov eax, [CR3Value]
    mov cr3, eax

    ;   Set Extended Features Enable (EFER) in Model Specific Register (MSR)
    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8  ;   LM Bit
    wrmsr

    ;   Enable Paging
    mov eax, cr0
    or eax, 1 << 31 ; PG bit
    mov cr0, eax

    lgdt [GDTR64]
    jmp 0x08:APLongMode

    section .text
    bits 64

APLongMode:
    cli

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;   Write some stuff to the frame buffer
    mov rcx, 0
    mov rax, [pFramebuffer]
testloop:
    mov dword [rax], 0xFFFFFFFF

    inc rcx
    add rax, 4

    cmp rcx, 4096
    jge halt
    jmp testloop

halt:
    hlt
    jmp halt

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

    ; Some hardware, for no apparent reason, needs this here
    push rbx

    ;   Place rsi into edx:eax
    mov rax, rsi
    mov rbx, 0x00000000FFFFFFFF
    and rax, rbx
    mov rdx, rsi
    shr rdx, 32

    mov rcx, rdi    ;   MSRID
    wrmsr

    pop rbx

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

    global GetCR3Value
GetCR3Value:
    xor rax, rax
    mov rax, cr3
    mov [rdi], rax

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
    ;mov cr4, rax

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
