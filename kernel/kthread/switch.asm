; Cooperative context switch (i686, cdecl).
; Linked as elf32 — see kernel/CMakeLists.txt.
;
; void thread_switch(uint32_t *save_sp, uint32_t new_sp);
;
; Saves callee-saved registers on the current stack, stores ESP in *save_sp,
; loads ESP from new_sp, restores the other context, and ret's to its caller.
;
; v2 TODO: stack guard pages (unmap below stack) via dedicated per-kthread VA
; mappings — not kmalloc stacks; needs paging unmap / not-present PTEs.

bits 32

section .text

global thread_switch
thread_switch:
    mov eax, [esp + 4]      ; save_sp
    mov edx, [esp + 8]      ; new_sp

    push ebp
    push ebx
    push esi
    push edi

    mov [eax], esp

    mov esp, edx

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
