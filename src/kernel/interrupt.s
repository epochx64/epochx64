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

    bits 64

    section .text

; ----------------------------------------------------------
;                           IRQ handlers                   |
; ----------------------------------------------------------

    ;   ERR_INFO is pointer to KE_TASK_DESCRIPTOR struct
    extern ERR_INFO
    extern GenericExceptionHandler
    global GenericException
GenericException:
    fxsave [ERR_INFO + 0]

    mov [ERR_INFO + 512], r15
    mov [ERR_INFO + 520], r14
    mov [ERR_INFO + 528], r13
    mov [ERR_INFO + 536], r12
    mov [ERR_INFO + 544], r11
    mov [ERR_INFO + 552], r10
    mov [ERR_INFO + 560], r9
    mov [ERR_INFO + 568], r8
    mov [ERR_INFO + 576], rbp
    mov [ERR_INFO + 584], rsp
    mov [ERR_INFO + 592], rsi
    mov [ERR_INFO + 600], rdi
    mov [ERR_INFO + 608], rdx
    mov [ERR_INFO + 616], rcx
    mov [ERR_INFO + 624], rbx
    mov [ERR_INFO + 632], rax

    ;   Error code literally never shows up so don't mind the janky offsets
    mov rdi, [rsp-8]    ;   (Nonexistent) error code if it did exist
    mov rsi, [rsp+0]    ;   CS
    mov rdx, [rsp+8]    ;   RIP
    mov rcx, [rsp+16]   ;   RFLAGS

    call GenericExceptionHandler
    iretq

    global int49
int49:
    int 49
    ret

    extern ISR49Handler
    global ISR49
ISR49:
    pushaq
    call ISR49Handler
    popaq
    iretq

    extern ISR32TimerHandler
    global ISR32
ISR32:
    ;   Timer interrupt handler uses floating point
    fxsave [FXSAVE]
    pushaq
    call ISR32TimerHandler
    popaq
    fxrstor [FXSAVE]
    iretq

    extern ISR33KeyboardHandler
    global ISR33
ISR33:
    pushaq
    call ISR33KeyboardHandler
    popaq
    iretq

    extern ISR255SpuriousHandler
    global ISR255
ISR255:
    pushaq
    call ISR255SpuriousHandler
    popaq
    iretq

    ;   -------------------------------------------------------------------------
    ;
    ;  APIC Timer handler, is responsible for scheduling and some other stuff
    ;
    ;   -------------------------------------------------------------------------

    global ISR48
ISR48:
    extern keTasks
    extern ISR48APICTimerHandler
    extern APICID

    push rax
    push rbx

    call APICID
    mov rbx, rax
    imul rbx, 8

    ; CTaskInfos is an array of pointers to KE_TASK_DESCRIPTOR structs, one for every logical processor
    mov rax, [keTasks]

    add rax, rbx
    mov rax, [rax]

    pop rbx

    ;   rax is now a pointer to a KE_TASK_DESCRIPTOR struct

    ;   Save all the registers
    fxsave [rax + 0]

    mov [rax + 512], r15
    mov [rax + 520], r14
    mov [rax + 528], r13
    mov [rax + 536], r12
    mov [rax + 544], r11
    mov [rax + 552], r10
    mov [rax + 560], r9
    mov [rax + 568], r8
    mov [rax + 576], rbp
    mov [rax + 592], rsi
    mov [rax + 600], rdi
    mov [rax + 608], rdx
    mov [rax + 616], rcx
    mov [rax + 624], rbx

    mov rbx, rax
    pop rax
    mov [rbx + 632], rax

    ;   Save the IRETQ stack parameters
    ;   currentTaskInfo changes after this function
    mov rdi, [rsp + 0]      ;   RIP
    mov rsi, [rsp + 8]      ;   CS
    mov rdx, [rsp + 16]     ;   RFLAGS
    mov rcx, [rsp + 24]     ;   RSP

    mov [rbx + 640], rdi    ;   RIP
    mov [rbx + 648], rsi    ;   CS
    mov [rbx + 656], rdx    ;   RFLAGS
    mov [rbx + 584], rcx    ;   RSP

    call ISR48APICTimerHandler

    ;   Now restore the processor state
    call APICID
    mov rbx, rax

    mov rax, [keTasks]

    imul rbx, 8
    add rax, rbx
    mov rax, [rax]

    fxrstor [rax + 0]

    mov r15, [rax + 512]
    mov r14, [rax + 520]
    mov r13, [rax + 528]
    mov r12, [rax + 536]
    mov r11, [rax + 544]
    mov r10, [rax + 552]
    mov r9, [rax + 560]
    mov r8, [rax + 568]
    mov rbp, [rax + 576]

    ;   Restore the IRETQ stack parameters
    ;   we have to jankily put this here since
    ;   ISR48APICTimerHandler affects rdi, rsi, rdx...
    mov rdi, [rax + 640] ;   RIP
    mov rsi, [rax + 648] ;   CS
    mov rdx, [rax + 656] ;   RFLAGS
    mov rcx, [rax + 584] ;   RSP

    mov [rsp + 0], rdi      ;   RIP
    mov [rsp + 8], rsi      ;   CS
    mov [rsp + 16], rdx     ;   RFLAGS
    mov [rsp + 24], rcx     ;   RSP

    mov rsi, [rax + 592]
    mov rdi, [rax + 600]
    mov rdx, [rax + 608]
    mov rcx, [rax + 616]
    mov rbx, [rax + 624]
    mov rax, [rax + 632]

    iretq

;-------------------------------------------------

    extern ISR44MouseHandler
    global ISR44
ISR44:
    pushaq
    call ISR44MouseHandler
    popaq
    iretq

    extern IDTR64
    extern GDTR64

; TODO: Make this take an argument for the IDTR
    global lidt
lidt:
    lidt [IDTR64]
    ret

    global lgdt
lgdt:
    lgdt [GDTR64]

    ;   Changes CS register to 0x08
    ;   (code segment descriptor in new GDT)
    push 0x08
    push done
    retfq

done:
    ;   GDT data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret

    section .data
    align 16
FXSAVE: times 512 db 0