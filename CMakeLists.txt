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

add_subdirectory(kernel)
add_subdirectory(boot)
