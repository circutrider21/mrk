[bits 64]

%macro SCALL 1
extern __syscall_%1
dq __syscall_%1
%endmacro

section .rodata

syscall_count equ ((__syscall_table.end - __syscall_table) / 8)

align 16
__syscall_table:

SCALL noop

.end:

section .text

global syscall_handler
syscall_handler:
  cmp rax, syscall_count
  jae .unknown

  ; Swap GS and stacks
  swapgs
  mov QWORD [gs:0008], rsp
  mov rsp, QWORD [gs:0000]
  sti
  cld

  ; Act like a interrupt happened so that we can use struct cpu_ctx
  push 0x1b              ; SS
  push qword [gs:0008]   ; RSP
  push r11               ; RFLAGS
  push 0x23              ; CS
  push rcx               ; RIP

  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  ; Set SMAP guard if supported
  mov rdi, QWORD [gs:0024]
  cmp rdi, 1
  jne .call_handler
  clac ; Clear RFLAGS.AC, which activates SMAP

.call_handler:
  ; Pass the stack and call the syscall handler
  mov rdi, rsp
  lea rbx, [rel __syscall_table]
  call [rbx + rax * 8]

  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  pop rbp
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15

  add rsp, 40 ; Pop fake state we created earlier
  
  ; Restore user stack and pass errno in rbx
  mov rbx, QWORD [gs:0016]
  cli
  mov rsp, QWORD [gs:0008]
  swapgs

  o64 sysret

.unknown:
  mov rax, -1
  mov rbx, 1051 ; ENOSYS = 1051
  o64 sysret


