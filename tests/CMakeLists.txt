cmake_minimum_required(VERSION 3.20)

project(XOSTests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

set(XOS_KERNEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../kernel")

# Kernel .c files linked into host tests (add paths as coverage grows; not the full kernel).
set(XOS_KERNEL_LIB_SOURCES
    "${XOS_KERNEL_DIR}/mem/kmem_utils.c"
    "${XOS_KERNEL_DIR}/mem/phys_map.c"
)

add_library(xos_kernel_library STATIC ${XOS_KERNEL_LIB_SOURCES})

target_include_directories(xos_kernel_library PUBLIC "${XOS_KERNEL_DIR}")
target_compile_options(xos_kernel_library PRIVATE -Wall -Wextra)

set(XOS_TEST_SOURCES
    kmem/kmem-utils-test.cpp
    mem/phys-map-test.cpp
)

add_executable(xos_test ${XOS_TEST_SOURCES})

target_include_directories(xos_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mem")

target_link_libraries(xos_test PRIVATE xos_kernel_library GTest::gtest_main)

gtest_discover_tests(xos_test)
