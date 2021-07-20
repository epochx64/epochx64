bits 64

section .text

;**********************************************************************
; @details Wait to acquire a lock and spin to wait
;**********************************************************************
global AcquireLock
AcquireLock:
    ; Perform a test-and-set of bit 0 of arg1
    lock bts qword [rdi], 0
    ; Spin if it is acquired
    jc .spin
    ret

.spin:
    pause
    test qword [rdi], 1
    jnz .spin
    jmp AcquireLock

;**********************************************************************
; @details Release a lock
;**********************************************************************
global ReleaseLock
ReleaseLock:
    mov qword [rdi], 0
    ret