; Loaded by boot.asm to physical 0x1000; entered in 32-bit protected mode (DS/SS set).

[org 0x1000]
[bits 32]

start:
    mov edi, 0xb8000         ; VGA text buffer, mode 3 (set by bootloader)
    mov esi, hello

.print:
    mov al, [esi]
    test al, al
    jz .done
    mov ah, 0x0f             ; white on black
    mov [edi], ax
    add edi, 2
    inc esi
    jmp .print

.done:
    hlt
    jmp .done

hello:
    db "Hello from kernel @ 0x1000!", 0
