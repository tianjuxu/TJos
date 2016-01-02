;this is a test program to test user mode execution
;this only sets eax to 0xdeadbeef and loops forever
section .text
global _start
_start:
;enter infinite loop
.loop:
mov eax,0xDEADBEEF
jmp .loop
