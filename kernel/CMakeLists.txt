set(START_ASM "${CMAKE_CURRENT_SOURCE_DIR}/start_kernel.asm")
set(START_O   "${CMAKE_CURRENT_BINARY_DIR}/start_kernel.o")
set(ISR_ASM   "${CMAKE_CURRENT_SOURCE_DIR}/cpu/isr.asm")
set(ISR_O     "${CMAKE_CURRENT_BINARY_DIR}/isr.o")
set(KERNEL_ELF "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf")
set(KERNEL_BIN "${CMAKE_CURRENT_BINARY_DIR}/kernel.bin")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

add_custom_command(
    OUTPUT "${START_O}"
    COMMAND "${XOS_NASM}" -f elf32 "${START_ASM}" -o "${START_O}"
    DEPENDS "${START_ASM}"
    COMMENT "Assembling kernel entry (elf32)"
    VERBATIM
)

add_custom_target(start_kernel_o DEPENDS "${START_O}")

add_custom_command(
    OUTPUT "${ISR_O}"
    COMMAND "${XOS_NASM}" -f elf32 "${ISR_ASM}" -o "${ISR_O}"
    DEPENDS "${ISR_ASM}"
    COMMENT "Assembling CPU ISRs (elf32)"
    VERBATIM
)

add_custom_target(isr_o DEPENDS "${ISR_O}")

add_executable(kernel_elf "${START_O}" "${ISR_O}" kernel.c cpu/idt.c cpu/isr.c cpu/pic.c drivers/vga/vga.c mem/kmem.c log/kprint.c)

target_include_directories(kernel_elf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_dependencies(kernel_elf start_kernel_o isr_o)
set_target_properties(kernel_elf PROPERTIES
    OUTPUT_NAME kernel
    SUFFIX .elf
    LINKER_LANGUAGE C
)
target_compile_options(kernel_elf PRIVATE
    -ffreestanding
    -mgeneral-regs-only
    -mno-red-zone
    -Wformat
)
target_link_options(kernel_elf PRIVATE
    -T "${LINKER_SCRIPT}"
    -nostdlib
    -Wl,--build-id=none
)
target_link_libraries(kernel_elf PRIVATE gcc)

add_custom_command(
    OUTPUT "${KERNEL_BIN}"
    COMMAND "${XOS_OBJCOPY}" -O binary "$<TARGET_FILE:kernel_elf>" "${KERNEL_BIN}"
    DEPENDS kernel_elf
    COMMENT "kernel.elf -> kernel.bin"
    VERBATIM
)

add_custom_target(kernel_bin ALL DEPENDS "${KERNEL_BIN}")
add_dependencies(kernel_bin kernel_elf)

set(KERNEL_BIN "${KERNEL_BIN}" PARENT_SCOPE)
