set(BOOT_ASM "${CMAKE_CURRENT_SOURCE_DIR}/boot.asm")
set(BOOT_BIN "${CMAKE_CURRENT_BINARY_DIR}/boot.bin")

add_custom_command(
    OUTPUT "${BOOT_BIN}"
    COMMAND "${XOS_NASM}" -f bin "${BOOT_ASM}" -o "${BOOT_BIN}"
    DEPENDS "${BOOT_ASM}"
    COMMENT "Assembling boot sector"
    VERBATIM
)

add_custom_target(boot_bin ALL DEPENDS "${BOOT_BIN}")

set(BOOT_BIN "${BOOT_BIN}" PARENT_SCOPE)
