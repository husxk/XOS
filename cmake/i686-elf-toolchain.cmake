set(XOS_CROSS_PREFIX "i686-elf" CACHE STRING "Cross-compilation tool prefix (gcc, objcopy, …)")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER "${XOS_CROSS_PREFIX}-gcc")
set(CMAKE_ASM_COMPILER "${XOS_CROSS_PREFIX}-gcc")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-stack-protector -fno-pic")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib")

find_program(XOS_NASM nasm REQUIRED)
find_program(XOS_OBJCOPY NAMES "${XOS_CROSS_PREFIX}-objcopy" REQUIRED)
