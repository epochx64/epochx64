cmake_minimum_required(VERSION 3.10)

project(epochx64)

file(GLOB_RECURSE kernel_SOURCES "src/kernel/*.cpp")
file(GLOB_RECURSE kernel_HEADERS "src/kernel/*.h")
file(GLOB_RECURSE kernel_ASSEMBS "src/kernel/*.s")

file(GLOB_RECURSE defs_HEADERS "defs/*.h")

add_executable(epochx64 src/boot/boot.c ${kernel_HEADERS} ${kernel_SOURCES} src/lib/string.cpp src/lib/string.h src/lib/math/math.cpp src/lib/math/math.h src/lib/math/conversion.h src/kernel/acpi.cpp src/lib/mem.cpp src/lib/mem.h src/kernel/acpi.h src/kernel/typedef.h src/kernel/scheduler.cpp src/kernel/scheduler.h src/lib/linkedlist.cpp src/lib/linkedlist.h src/kernel/gui/window.cpp src/kernel/gui/window.h src/fs/ext2.cpp src/fs/ext2.h src/lib/elf/relocation.cpp src/lib/elf/relocation.h src/proc/test/epoch.h src/proc/test/epoch.cpp src/kernel/process.cpp src/kernel/process.h src/kernel/keyboard.cpp src/kernel/keyboard.h src/kernel/pic.cpp src/kernel/pic.h)

target_include_directories(epochx64 PUBLIC src src/lib src/lib/elf src/lib/uefi/Include src/lib/uefi/Include/X64 src/lib/uefi/Include/Protocol)