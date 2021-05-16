cmake_minimum_required(VERSION 3.10)

project(epochx64)

file(GLOB_RECURSE kernel_SOURCES "src/kernel/*.cpp")
file(GLOB_RECURSE kernel_HEADERS "src/include/*.h")
file(GLOB_RECURSE kernel_kernel_HEADERS "src/kernel/include/*.h")
file(GLOB_RECURSE kernel_ASSEMBS "src/kernel/*.s")

file(GLOB_RECURSE defs_HEADERS "defs/*.h")

add_executable(epochx64 src/boot/boot.c
        ${kernel_HEADERS}
        ${kernel_SOURCES}
        ${kernel_kernel_HEADERS}
        src/lib/epstring.cpp
        src/lib/math/math.cpp
        src/include/math/math.h
        src/include/math/conversion.h
        src/kernel/acpi.cpp
        src/lib/mem.cpp
        src/include/mem.h
        src/kernel/include/acpi.h
        src/kernel/include/typedef.h
        src/kernel/scheduler.cpp
        src/kernel/include/scheduler.h
        src/lib/linkedlist.cpp
        src/include/linkedlist.h
        src/kernel/gui/window.cpp
        src/kernel/include/window.h
        src/fs/ext2.cpp
        src/include/fs/ext2.h
        src/lib/elf/relocation.cpp
        src/include/elf/relocation.h
        src/proc/test/epoch.h
        src/proc/test/epoch.cpp
        src/kernel/process.cpp
        src/kernel/include/process.h
        src/kernel/keyboard.cpp
        src/kernel/include/keyboard.h
        src/kernel/pic.cpp
        src/kernel/include/pic.h
        )

target_include_directories(epochx64 PUBLIC src src/include src/kernel/include src/include/elf src/include/uefi src/include/uefi/X64 src/include/uefi/Protocol)