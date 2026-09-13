cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR
    "XOS must be configured with the i686-elf toolchain.\n"
    "  cmake -B build -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/i686-elf-toolchain.cmake\n"
    "  ./scripts/build.sh")
endif()

project(XOS LANGUAGES C)

if(NOT DEFINED XOS_CROSS_PREFIX OR NOT XOS_NASM OR NOT XOS_OBJCOPY)
  message(FATAL_ERROR
    "Expected cmake/i686-elf-toolchain.cmake (XOS_CROSS_PREFIX, XOS_NASM, XOS_OBJCOPY).\n"
    "  ./scripts/build.sh")
endif()

get_filename_component(_xos_c_compiler_name "${CMAKE_C_COMPILER}" NAME)
if(NOT _xos_c_compiler_name STREQUAL "${XOS_CROSS_PREFIX}-gcc")
  message(FATAL_ERROR
    "Expected ${XOS_CROSS_PREFIX}-gcc, got: ${CMAKE_C_COMPILER}")
endif()

# boot/CMakeLists.txt sets BOOT_BIN and exports it to this scope (PARENT_SCOPE).
add_subdirectory(boot)

# kernel/CMakeLists.txt sets KERNEL_BIN and exports it to this scope (PARENT_SCOPE).
add_subdirectory(kernel)

set(OS_IMAGE "${CMAKE_BINARY_DIR}/os.img")

add_custom_command(
    OUTPUT "${OS_IMAGE}"
    COMMAND ${CMAKE_COMMAND} -E cat "${BOOT_BIN}" "${KERNEL_BIN}" > "${OS_IMAGE}"
    DEPENDS boot_bin kernel_bin
    COMMENT "Creating os.img (boot + kernel)"
    VERBATIM
)

add_custom_target(os_image ALL DEPENDS "${OS_IMAGE}")
