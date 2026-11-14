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
    "${XOS_KERNEL_DIR}/mem/kmem.c"
    "${XOS_KERNEL_DIR}/mem/kmem_utils.c"
    "${XOS_KERNEL_DIR}/mem/phys_map.c"
    "${XOS_KERNEL_DIR}/mem/phys_mem.c"
    "${XOS_KERNEL_DIR}/util/min_heap.c"
    "${XOS_KERNEL_DIR}/util/ring_buffer.c"
)

add_library(xos_kernel_library STATIC ${XOS_KERNEL_LIB_SOURCES})

target_include_directories(xos_kernel_library PUBLIC "${XOS_KERNEL_DIR}")
target_compile_options(xos_kernel_library PRIVATE -Wall -Wextra)

set(XOS_TEST_SOURCES
    kmem/kmem-utils-test.cpp
    mem/kheap-test.cpp
    mem/phys-map-test.cpp
    mem/phys-mem-test.cpp
    util/min-heap-test.cpp
    util/ring-buffer-test.cpp
    util/ring-buffer-stress-test.cpp
    support/panic_host.c
)

add_executable(xos_test ${XOS_TEST_SOURCES})

target_include_directories(xos_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mem")

target_link_libraries(xos_test PRIVATE xos_kernel_library GTest::gtest_main)

gtest_discover_tests(xos_test)

# Sanitizer build (opt-in): cmake -S tests -B build-san -DXOS_SANITIZE=ON
# Instruments the test binary and the shared kernel sources it links with both
# ThreadSanitizer and UndefinedBehaviorSanitizer. UBSan catches signed overflow,
# out-of-bounds, misaligned access and bad shifts; its -fno-sanitize-recover makes
# any UB an immediate nonzero exit, not a warning the run ignores. The plain --tests
# build stays uninstrumented and fast.
option(XOS_SANITIZE "Instrument host tests with ThreadSanitizer + UBSan" OFF)
if(XOS_SANITIZE)
    foreach(t xos_kernel_library xos_test)
        target_compile_options(${t} PRIVATE
            -fsanitize=thread,undefined -fno-sanitize-recover=undefined -g -O1)
        target_link_options(${t} PRIVATE -fsanitize=thread,undefined)
    endforeach()
endif()
