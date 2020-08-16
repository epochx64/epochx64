bits 32

HEAP_SIZE       equ 524288  ; 64 KiB
STACK_SIZE      equ 65536   ; 64 KiB

;   Initial paging tables
PML4_ADDR       equ 0x1000
PML3_ADDR       equ 0x2000
PML2_ADDR       equ 0x3000
PML1_ADDR       equ 0x4000

;   Page map member flags
PRESENT         equ 0b001
RW              equ 0b010
USER            equ 0b100

;   Set aside space for the page directory and page table
;   these datastructures are important for mapping physical memory
;   to virtual memory location
;   -    Page directory and table are 4KiB in size; each having 1024 4byte entries
;   -    Further tables will be used if the kernel's size exceeds 3MiB

;   Declare constants for the multiboot header.
MAGIC           equ  0xE85250D6     ;   'magic number' lets bootloader find the header */
ARCHITECHTURE   equ  0
HEADER_LENGTH   equ  multiboot_header_end - multiboot_header
CHECKSUM        equ  -(MAGIC + ARCHITECHTURE + HEADER_LENGTH)

;   Declare a multiboot header that marks the program as a kernel. These are magic
;   values that are documented in the multiboot standard. The bootloader will
;   search for this signature in the first 8 KiB of the kernel file, aligned at a
;   32-bit boundary. The signature is in its own section so the header can be
;   forced to be within the first 8 KiB of the kernel file.

;   VESA graphics settings are specified in the multiboot header
;   1600x900, 32 bit color
global _kernel_start

section .multiboot
_kernel_start:

multiboot_header:
align 8
    dd MAGIC
    dd ARCHITECHTURE
    dd HEADER_LENGTH
    dd CHECKSUM

framebuffer_tag:
align 8
    dw 5
    dw 0
    dd framebuffer_tag_end - framebuffer_tag
    dd 1600
    dd 900
    dd 32
framebuffer_tag_end:

align 8
	dw 0
	dw 0
	dd 8
multiboot_header_end:

section .bootstrap
global _start

;   Entry point called by GRUB
_start:
    ;   TODO: So you don't do any checks (long mode, floating point arithmetic...) before enabling stuff

    ;   Prevent maskable external interrupts
    cli

    ;   Setup the paging structures
    ;   PML4[0] -> PML3[0] -> PML2[0] -> PML1[0-511] -> 0x000000 -> 0x200000
    mov eax, dword PML3_ADDR
    or eax, (PRESENT | RW | USER)
    mov [PML4_ADDR], eax
    mov [PML4_ADDR + 4], dword 0x0

    mov eax, dword PML2_ADDR
    or eax, (PRESENT | RW| USER)
    mov [PML3_ADDR], eax
    mov [PML3_ADDR + 4], dword 0x0

    mov eax, dword PML1_ADDR
    or eax, (PRESENT | RW| USER)
    mov [PML2_ADDR], eax
    mov [PML2_ADDR + 4], dword 0x0

    ;   Now we identity map PML1[0 -> 511]
    mov ecx, 512
    mov edx, (PRESENT | RW | USER)
    mov edi, PML1_ADDR

.setentry:
    mov [edi], edx
    mov [edi + 4], dword 0x0
    add edi, 8
    add edx, 0x1000
    loop .setentry

    ;   Enable Physical Address Extension (PAE)
    mov eax, cr4
    or eax, 1 << 5  ;   6th bit
    mov cr4, eax

    ;   TODO: Enable A20 line if not enabled. probably is but who really knows

    mov eax, PML4_ADDR
    mov cr3, eax

    ;   Set Extended Features Enable (EFER) in Model Specific Register (MSR)
    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8  ;   LM Bit
    wrmsr

    ;   Enable 128 precision floating point
    mov eax, cr0
    and eax, 0xFFFFFFFB     ; Clear the EM flag
    or eax, 0x00000002      ; Set the MP flag
    mov cr0, eax

    mov eax, cr4
    or eax, 3 << 9  ;   Set OSFXSR and OSXMMEXCPT bits
    mov cr4, eax

    ;   Enable Paging
    mov eax, cr0
    or eax, 1 << 31 ; PG bit
    mov cr0, eax

    ;   Load the GDT and jump to the 64-bit kernel entrypoint
    lgdt [GDT64.Pointer]
    jmp GDT64.Code:kernel_start

bits 64
section .text
kernel_start:
    ;   Set segment registers to point at GDT data segment

    cli
    mov ax, GDT64.Data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stack_top
    mov rbp, rsp

    extern kernel_main

    mov rcx, heap_top
    mov rdx, HEAP_SIZE
    mov rsi, PML4_ADDR
    mov rdi, rbx            ;   Pointer to multiboot2_info struct
    call kernel_main


section .data
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
    .Pointer:                    ; The GDT-pointer.
    dw $ - GDT64 - 1             ; Limit.
    dq GDT64                     ; Base.

section .bss
align 4096

heap_top:
    resb HEAP_SIZE
heap_bottom:

stack_bottom:
    resb STACK_SIZE
stack_top:

global _kernel_end
_kernel_end: