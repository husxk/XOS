set(BOOT_ASM "${CMAKE_CURRENT_SOURCE_DIR}/boot.asm")
set(BOOT_BIN "${CMAKE_CURRENT_BINARY_DIR}/boot.bin")

if(NOT KERNEL_BIN)
    message(FATAL_ERROR "KERNEL_BIN must be set (add_subdirectory(kernel) before boot).")
endif()

set(_ASSEMBLE_BOOT_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/assemble_boot.sh")

add_custom_command(
    OUTPUT "${BOOT_BIN}"
    COMMAND "${_ASSEMBLE_BOOT_SCRIPT}"
        --kernel "${KERNEL_BIN}"
        --boot-asm "${BOOT_ASM}"
        --output "${BOOT_BIN}"
        --nasm "${XOS_NASM}"
    DEPENDS "${BOOT_ASM}" "${KERNEL_BIN}" "${_ASSEMBLE_BOOT_SCRIPT}"
    COMMENT "Assembling boot sector (KERNEL_SECTORS from kernel.bin size)"
    VERBATIM
)

add_custom_target(boot_bin ALL DEPENDS "${BOOT_BIN}")
add_dependencies(boot_bin kernel_bin)

set(BOOT_BIN "${BOOT_BIN}" PARENT_SCOPE)
