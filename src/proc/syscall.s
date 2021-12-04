%macro system_call 1
    mov [rsp - 16], rax
    mov rax, [rel keSyscallTable]
    add rax, 8*%1
    mov rax, [rax]
    push rax
    mov rax, [rsp - 8]
    ret
%endmacro

    bits 64

extern keSyscallTable

global KeCreateProcess
global KeGetTime
global KeScheduleTask
global KeSuspendTask
global KeSuspendCurrentTask
global KeGetCurrentTaskHandle
global KeResumeTask
global KeQueryTask
global KeQueryScheduler

KeCreateProcess:          system_call 0
KeGetTime:                system_call 1
KeScheduleTask:           system_call 2
KeSuspendTask:            system_call 3
KeSuspendCurrentTask:     system_call 4
KeGetCurrentTaskHandle:   system_call 5
KeResumeTask:             system_call 6
KeQueryTask:              system_call 7
KeQueryScheduler:         system_call 8
