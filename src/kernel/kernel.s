;   FYI:    A lot of this is legacy code and unused since moving to UEFI

;bits 32
;
;; ----------------------------------------------------------
;;                  32 bit kernel bootstrap                  |
;; ----------------------------------------------------------
;
;section .text
;;   We should not need to call this function since UEFI already sets up long mode
;;   However, I want to keep this just incase there's any steps that were missed
;;   that are not neccessary for basic long mode
;LongModeBootstrap:
;    ; Paging should be setup up here
;
;    ;   Enable Physical Address Extension (PAE)
;    mov eax, cr4
;    or eax, 1 << 5  ;   6th bit
;    mov cr4, eax
;
;    ;   TODO: Enable A20 line if not enabled. probably is but who really knows
;
;    mov eax, 0x1000 ; PML4_ADDR while we were using legacy BIOS
;    mov cr3, eax
;
;    ;   Set Extended Features Enable (EFER) in Model Specific Register (MSR)
;    mov ecx, 0xc0000080
;    rdmsr
;    or eax, 1 << 8  ;   LM Bit
;    wrmsr
;
;    ;   Post UEFI update note: if there's something that UEFI didn't
;    ;   already do for us, this would be it
;    ;   Enable 128 precision floating point
;    mov eax, cr0
;    and eax, 0xFFFFFFFB     ; Clear the EM flag
;    or eax, 0x00000002      ; Set the MP flag
;    mov cr0, eax
;    mov eax, cr4
;    or eax, 3 << 9  ;   Set OSFXSR and OSXMMEXCPT bits
;    mov cr4, eax
;
;    ;   Enable Paging
;    mov eax, cr0
;    or eax, 1 << 31 ; PG bit
;    mov cr0, eax
;
;    ;   Load the GDT and jump to the 64-bit kernel entrypoint
;    lgdt [GDT64.Pointer]
;    jmp GDT64.Code:kernel_start
;
;; ----------------------------------------------------------
;;            64 bit kernel entry point                      |
;; ----------------------------------------------------------
;
;bits 64
;section .text
;kstart:
;    ;   Set segment registers to point at GDT data segment
;    cli
;    mov ax, GDT64.Data
;    mov ds, ax
;    mov es, ax
;    mov fs, ax
;    mov gs, ax
;    mov ss, ax

;    mov rsp, stack_top
;    mov rbp, rsp

;    extern kernel_main

;    mov rcx, heap_top
;    mov rdx, HEAP_SIZE
;    mov rsi, PML4_ADDR
;    mov rdi, rbx            ;   Pointer to multiboot2_info struct
;    call kernel_main

; ----------------------------------------------------------
;                       GDT Structure                      |
; ----------------------------------------------------------



; ----------------------------------------------------------
;                      Heap and stack                      |
; ----------------------------------------------------------
    bits 64

    HEAP_SIZE  equ 524288  ; 512 KiB
    STACK_SIZE equ 65536   ; 64 KiB

    section .bss
    align 4096

stack_bottom:
    resb STACK_SIZE
stack_top:

; ----------------------------------------------------------
;                      ASM Bootstrap                        |
; ----------------------------------------------------------
    section .text
    global kmain
    extern KernelMain