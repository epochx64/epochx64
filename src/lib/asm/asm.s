    %macro pushaq 0
        push r15
        push r14
        push r13
        push r12
        push r11
        push r10
        push r9
        push r8
        push rbp
        push rsi
        push rdi
        push rdx
        push rcx
        push rbx
        push rax
    %endmacro

    %macro popaq 0
        pop rax
        pop rbx
        pop rcx
        pop rdx
        pop rdi
        pop rsi
        pop rbp
        pop r8
        pop r9
        pop r10
        pop r11
        pop r12
        pop r13
        pop r14
        pop r15
    %endmacro

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

    global APICID
APICID:
    push rbx
    push rcx
    push rdx

    mov rax, 0x01
    cpuid
    shr rbx, 24
    and rbx, 0xFF
    mov rax, rbx

    pop rdx
    pop rcx
    pop rbx

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
    pushaq

    lea rax, [rel .next]
    push 0x08
    push rax
    retfq
.next:
    times 4 nop
    pause

    lea rax, [rel .next2]
    push 0x08
    push rax
    retfq
.next2:
    times 4 nop
    pause

    lea rax, [rel .next3]
    push 0x08
    push rax
    retfq
.next3:
    times 4 nop
    pause

    lea rax, [rel .next4]
    push 0x08
    push rax
    retfq
.next4:
    out 0x80, al
    out 0x80, al
    popaq
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

    ret

    global SetCR3Value
SetCR3Value:
    mov rax, rdi
    mov cr3, rax
    ret

   global GetCR4Value
GetCR4Value:
    mov rax, cr4
    ret

    global GetCR2Value
GetCR2Value:
    mov rax, cr2
    ret

    global SetRSP
SetRSP:
    sub rdi, 8
    mov rax, [rsp]
    mov [rdi], rax
    mov rsp, rdi

    ret

    global GetRSP
GetRSP:
    mov rax, rsp
    ret

    global GetRBP
GetRBP:
    mov rax, rbp
    ret

;   Sets up floating point math
    global EnableSSE
EnableSSE:
    fninit

    ;   Enable floating point
    mov rax, cr0
    and ax, 0xFFFB            ; Clear the EM flag
    or ax, 0x2      ; Set the MP flag
    mov cr0, rax

    ;   Set OSFXSR, OSXMMEXCPT bits
    mov rax, cr4
    or rax, 3 << 9
    mov cr4, rax

    ;   Save the current FPU state
    fxsave [rdi]

    ;   Grab the MXCSR value
    mov eax, [rdi + 0x18]

    ;   Mask all floating point exeptions
    or eax, 0x00001F80
    mov [rdi + 0x18], eax

    fxrstor [rdi]

    ret

    global nop
nop:
    nop
    ret

    global inb
inb:
    push rdx

    xor rax, rax
    mov dx, di  ;  Port #
    in al, dx

    ; Perform a delay
    jmp .1
.1:
    jmp .2
.2:

    pop rdx

    ret

    global ind
ind:
    push rdx

    xor rax, rax
    mov dx, di  ;  Port #
    in eax, dx

    ; Perform a delay
    jmp .1
.1:
    jmp .2
.2:

    pop rdx

    ret

    global outb
outb:
    push rdx
    push rax

    mov dx, di
    mov rax, rsi
    and rax, 0x00000000000000FF

    out dx, al

    ; Perform a delay
    jmp .1
.1:
    jmp .2
.2:

    pop rax
    pop rdx

    ret

    global outd
outd:
    push rdx
    push rax
    push rbx

    mov dx, di
    mov rax, rsi
    mov rbx, 0x00000000FFFFFFFF
    and rax, rbx

    out dx, eax

    ; Perform a delay
    jmp .1
.1:
    jmp .2
.2:

    pop rbx
    pop rax
    pop rdx

    ret

    global memset128
memset128:

    ret

    global memset64
memset64:
    push rcx
    push rax
    push rdi

    mov rcx, rsi
    mov rax, rdx
    shr rcx, 3
    rep stosq

    pop rdi
    pop rax
    pop rcx
    ret

    global memset32
memset32:
    push rcx
    push rax
    mov rcx, rsi
    mov rax, rdx
    shr rcx, 2
    rep stosd
    pop rax
    pop rcx
    ret

    global memset16
memset16:
    push rcx
    push rax
    mov rcx, rsi
    mov rax, rdx
    shr rcx, 1
    rep stosw
    pop rax
    pop rcx
    ret

    global memset8
memset8:
    push rcx
    push rax
    mov rcx, rsi
    mov rax, rdx
    rep stosb
    pop rax
    pop rcx
    ret

