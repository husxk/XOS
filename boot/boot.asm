[org 0x7c00]                 ; BIOS loads boot sector at 0x7C00
KERNEL_LOCATION equ 0x1000   ; where we load + jump to the next-stage kernel

; --- real mode setup ---

mov [BOOT_DISK], dl          ; DL = boot drive on entry; INT 13h needs it later

xor ax, ax                   ; zero AX
mov es, ax                   ; ES:BX = read buffer for disk load
mov ds, ax                   ; flat seg 0 for simple addressing in real mode
mov bp, 0x8000
mov sp, bp                   ; stack below boot sector (0x7C00), grows down

; --- load kernel from disk (sector 2+, right after this 512-byte sector) ---
; KERNEL_SECTORS: set at build from kernel.bin size (see boot/CMakeLists.txt).

%ifndef KERNEL_SECTORS
    %define KERNEL_SECTORS 0x20
%endif

mov ah, 0x02                 ; INT 13h: read sectors
mov al, KERNEL_SECTORS       ; AL = number of sectors to read
mov ch, 0x00                 ; cylinder 0
mov dh, 0x00                 ; head 0
mov cl, 0x02                 ; start at sector 2 (sector 1 is this boot block)
mov dl, [BOOT_DISK]          ; drive number
mov bx, KERNEL_LOCATION      ; where to load
int 0x13

; TODO: Check return value

mov ah, 0x0
mov al, 0x3
int 0x10                     ; switch text mode (clears screen)

; --- protected mode: flat 32-bit code/data at 0 ---

CODE_SEG equ GDT_code - GDT_start   ; CS selector offset into GDT
DATA_SEG equ GDT_data - GDT_start   ; DS/SS/... selector

cli                          ; no IDT yet — don't take IRQs in PM
lgdt [GDT_descriptor]
mov eax, cr0
or eax, 1                    ; CR0.PE = protected mode on (bit 0)
mov cr0, eax
jmp CODE_SEG:start_protected_mode   ; far jump (+ flush CPU pipeline)

jmp $                        ; unreachable (safety if far jump ever fails)

BOOT_DISK: db 0

GDT_start:
    GDT_null:                ; required null descriptor
        dd 0x0
        dd 0x0

    GDT_code:                ; base 0, limit 4G → flat 32-bit code
        dw 0xffff            ; limit bits 0-15 (0xFFFF)
        dw 0x0               ; base bits 0-15
        db 0x0               ; base bits 16-23
        db 0b10011010         ; access: Present 1, Privilege 00, code seg 1,
                              ;         executable 1, conforming 0, readable 1, accessed 0
        db 0b11001111         ; flags: granularity 4K, 32-bit (D=1), limit bits 16-19 = 0xF
        db 0x0               ; base bits 24-31

    GDT_data:                ; same flat map for data/stack
        dw 0xffff            ; limit bits 0-15 (0xFFFF)
        dw 0x0               ; base bits 0-15
        db 0x0               ; base bits 16-23
        db 0b10010010         ; access: Present 1, Privilege 00, data seg 1,
                              ;         expand-up 0, writable 1, accessed 0
        db 0b11001111         ; flags: granularity 4K, 32-bit (D=1), limit bits 16-19 = 0xF
        db 0x0               ; base bits 24-31

GDT_end:

GDT_descriptor:
    dw GDT_end - GDT_start - 1 ; GDT size in bytes, minus 1 (LGDT rule)
    dd GDT_start               ; linear address of GDT (seg 0 in real mode)

; --- 32-bit entry: same flat segments, new stack, run loaded kernel ---

[bits 32]
start_protected_mode:
    mov ax, DATA_SEG         ; load data selector into segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp             ; PM stack away from real-mode stack + low mem

    jmp KERNEL_LOCATION      ; near jump to code we read at 0x1000

; --- boot sector must be exactly 512 bytes with IBM PC signature ---

times 510-($-$$) db 0
dw 0xaa55
