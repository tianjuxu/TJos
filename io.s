; we use out and in instruction for writing and reading to/from IO ports
; Since operand of out instruction is only 8-bit long and our cursor data is 16-bit long per piece
; we need to send data in two turns.
; the format of out instruction is:
; out addr, data


global outb                       ; make label outb visible outside this file
global inb                        ; make inb visible everywhere

; outb - send a byte to an IO port
; stack: [esp+8] the data type
;        [esp+4] the IO port
;        [esp]   return address
outb:
    mov al,[esp+8]                ; move data into al reg
    mov dx,[esp+4]                ; move address of IO port into dx reg
    out dx,al                     ; send data to the IO port
    ret
; inb - return a bytefrom the given IO port
; stack: [esp+4] the address of the IO port
;        [esp  ] the return address
inb:
    mov dx, [esp + 4]             ; move addr of IO port to dx reg
    in  al, dx                    ; read a byte from the IO and st in al
    ret                           ; return the read byte
