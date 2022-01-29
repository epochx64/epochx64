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
    db 11001111b                 ; Granularity, limit19:16.
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
    global pAPBootstrapInfo
pAPBootstrapInfo: dq 0
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

    bits 64
APLongMode:
    cli
    lidt [IDTR64]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ;   Get local APIC ID
    mov rax, 0x01
    cpuid
    shr rbx, 24
    and rbx, 0xFF
    ;   Get pointer to the stack using APIC ID
    mov rax, rbx
    imul rax, 8  ;   sizeof(KE_AP_BOOTSTRAP_INFO)
    mov rbx, [pAPBootstrapInfo]
    add rbx, rax
    mov rax, [rbx]
    mov rsp, rax
    mov rbp, rax
    extern C_APBootstrap
    call C_APBootstrap

; Temporary paging structure to load kernel to higher-half
    align 4096
PML4_Entries:
    dq PML3_1_Entries + 3
    times 510 dq 0
    dq PML3_2_Entries + 3

    align 4096
PML3_1_Entries:
; Identity map first 8 GiBs
    dq 0x83
    dq 0x40000083
    dq 0x80000083
    dq 0xC0000083
    dq 0x100000083
    dq 0x140000083
    dq 0x180000083
    dq 0x1C0000083

    align 4096
PML3_2_Entries:
; Map last 2 GiB (higher half) to first 2 GiBs
    times 510 dq 0
    dq 0x83
    dq 0x40000083

; Kernel entry point. Sets up paging structs and jumps to higher half
    global KernelBootstrap
KernelBootstrap:
    ; Take over the paging structures
    push rax
    lea rax, [PML4_Entries]
    add rax, 0
    mov cr3, rax
    pop rax

    extern KeMain
    push KeMain
    ret
