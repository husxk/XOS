set(_KERNEL_ELF_SOURCES
    kernel.c cpu/idt.c cpu/isr.c cpu/pic.c drivers/timer/pit.c timer/timer.c
    drivers/keyboard/keyboard.c drivers/vga/vga.c mem/kmem.c log/kprint.c
)

if(NOT MB2_HEADER_O)
    set(MB2_HEADER_O "${CMAKE_BINARY_DIR}/boot/grub/multiboot2_header.o")
endif()
list(APPEND _KERNEL_ELF_SOURCES "${MB2_HEADER_O}")

set(START_ASM "${CMAKE_CURRENT_SOURCE_DIR}/start_kernel.asm")
set(START_O   "${CMAKE_CURRENT_BINARY_DIR}/start_kernel.o")
set(ISR_ASM   "${CMAKE_CURRENT_SOURCE_DIR}/cpu/isr.asm")
set(ISR_O     "${CMAKE_CURRENT_BINARY_DIR}/isr.o")
set(KERNEL_ELF "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf")
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

add_executable(kernel_elf "${START_O}" "${ISR_O}" ${_KERNEL_ELF_SOURCES})

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
)
target_link_options(kernel_elf PRIVATE
    -T "${LINKER_SCRIPT}"
    -nostdlib
    -Wl,--build-id=none
)

execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-libgcc-file-name
    OUTPUT_VARIABLE XOS_LIBGCC
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT EXISTS "${XOS_LIBGCC}")
    message(FATAL_ERROR "libgcc not found (compiler -print-libgcc-file-name): ${XOS_LIBGCC}")
endif()
target_link_libraries(kernel_elf PRIVATE "${XOS_LIBGCC}")

set(KERNEL_ELF "${KERNEL_ELF}" PARENT_SCOPE)
