  .global _start
  .text
_start:
  xor  %rbp, %rbp                 # sysV -> rbp should be 0 at start
  and  $0xfffffffffffffff0, %rsp  # sysV -> rsp should be 16-byte aligned at start

  call main

  mov  %rax, %rdi
  mov  $60, %rax
  syscall

main:
  push %rbp
  mov  %rsp, %rbp

  pop  %rbp
  xor  %rax, %rax
  ret
