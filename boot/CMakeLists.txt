# GRUB / Multiboot2 boot (boot/grub/).

find_program(XOS_GRUB_MKRESCUE NAMES grub-mkrescue REQUIRED)

set(MB2_HEADER_ASM "${CMAKE_CURRENT_SOURCE_DIR}/grub/multiboot2_header.asm")
set(MB2_HEADER_O   "${CMAKE_CURRENT_BINARY_DIR}/grub/multiboot2_header.o")

add_custom_command(
    OUTPUT "${MB2_HEADER_O}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/grub"
    COMMAND "${XOS_NASM}" -f elf32 "${MB2_HEADER_ASM}" -o "${MB2_HEADER_O}"
    DEPENDS "${MB2_HEADER_ASM}"
    COMMENT "Assembling Multiboot2 header (elf32)"
    VERBATIM
)

add_custom_target(multiboot2_header_o ALL DEPENDS "${MB2_HEADER_O}")
set(MB2_HEADER_O "${MB2_HEADER_O}" PARENT_SCOPE)
add_dependencies(kernel_elf multiboot2_header_o)

set(GRUB_CFG "${CMAKE_CURRENT_SOURCE_DIR}/grub/grub.cfg")
set(XOS_ISO "${CMAKE_BINARY_DIR}/xos.iso")
set(_ISO_STAGING "${CMAKE_CURRENT_BINARY_DIR}/iso-staging")
set(_BUILD_ISO_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build_xos_iso.sh")

add_custom_command(
    OUTPUT "${XOS_ISO}"
    COMMAND "${_BUILD_ISO_SCRIPT}"
        --kernel "$<TARGET_FILE:kernel_elf>"
        --grub-cfg "${GRUB_CFG}"
        --output "${XOS_ISO}"
        --staging "${_ISO_STAGING}"
        --grub-mkrescue "${XOS_GRUB_MKRESCUE}"
    DEPENDS kernel_elf "${GRUB_CFG}" "${_BUILD_ISO_SCRIPT}"
    COMMENT "Building bootable ISO (grub-mkrescue)"
    VERBATIM
)

add_custom_target(xos_iso ALL DEPENDS "${XOS_ISO}")
add_dependencies(xos_iso kernel_elf)

set(XOS_ISO "${XOS_ISO}" PARENT_SCOPE)
