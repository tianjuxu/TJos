global loader                       ; the entry symbol for ELF
extern   kmain             ;
MAGIC_NUMBER      equ 0x1BADB002    ; define the magic number constant
ALI               equ 1<<0          ; align loaded modules
MEMINFO           equ 1<<1          ; provide memory map
FLAGS             equ ALI | MEMINFO ; multi-boot flags
CHECKSUM          equ -(MAGIC_NUMBER+FLAGS) 
; calculate the checksum
; magic number + checksum +flags should be 0
KERNEL_STACK_SIZE equ 4096          ; size of stack in bytes

section .bss
alignb 4
kernel_stack:
    resb KERNEL_STACK_SIZE          ; reserve stack for kernel
section .text           ; start of the text section
align 4                  ; the code must be 4 byte(dd) aligned
    dd MAGIC_NUMBER      ; write the magic number to the machine code,
    dd FLAGS                   ; the flags
    dd CHECKSUM                ; and the checksum
loader:                  ; defined as the entry point in linker script
    mov  esp, kernel_stack + KERNEL_STACK_SIZE        ; point esp to the start of the stack(end of memory area)
    push ebx                   ; push multi-boot struct for kmain
    call kmain                 ;
.loop:
    jmp  .loop                 ; loop
