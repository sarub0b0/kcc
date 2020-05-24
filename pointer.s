.intel_syntax noprefix
.data
.LC0:
  .string "abc"
.text
.global main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 48
  mov [rbp-8], r12
  mov [rbp-16], r13
  mov [rbp-24], r14
  mov [rbp-32], r15
  mov r10, offset .LC0
  lea r11, [rbp-40]
  mov [r11], r10
  lea r10, [rbp-40]
  mov r10, [r10]
  mov r11, 1
  mov r12, 1
  imul r11, r12
  add r10, r11
  movsx r10, BYTE PTR [r10]
  mov rax, r10
  jmp .L.return.main
  mov r10, 0
  mov rax, r10
  jmp .L.return.main
.L.return.main:
  mov r12, [rbp-8] 
  mov r13, [rbp-16]
  mov r14, [rbp-24]
  mov r15, [rbp-32]
  mov rsp, rbp
  pop rbp
  ret
