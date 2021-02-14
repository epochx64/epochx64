NAME = epochx64

# For compiling the kernel
INCLUDES=-Isrc -Isrc/lib
SRC_LOC=./src

CXX = x86_64-elf-g++.exe

CFLAGS =  -ffreestanding -O2 -w -Wall -Wextra -fno-exceptions -fno-rtti -Wunused-parameter

CC = x86_64-elf-gcc.exe
LDFLAGS = -T $(SRC_LOC)/kernel/linker.ld -z max-page-size=4096 -ffreestanding -O2 -nostdlib -lgcc -Wunused-parameter

NASM = nasm.exe
NASMFLAGS = -felf64

# For compiling the EFI bootloader
EFI_C = x86_64-w64-mingw32-gcc.exe
EFI_CFLAGS = -ffreestanding
EFI_LINKFLAGS = -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main  -lgcc
EFI_INCLUDES = -Isrc/lib -Isrc -Isrc/lib/uefi/Include -Isrc/lib/uefi/Include/X64 -Isrc/lib/elf -Isrc/lib/uefi/Include/Protocol

KERNEL_TARGETS = kernel log interrupt acpi graphics scheduler process gui/window
LIB_TARGETS = string mem math/math elf/relocation ../fs/ext2
ASM_TARGETS = lib/asm/asm kernel/interrupt

#TODO:	So far this will only work with one target
PROC_TARGETS = test

all: efi $(KERNEL_TARGETS) $(LIB_TARGETS) $(ASM_TARGETS) $(PROC_TARGETS) link

run: all exec

# EFI Components
efi:
	@$(EFI_C) $(EFI_CFLAGS) $(EFI_INCLUDES) -c -o obj/boot/boot.obj $(SRC_LOC)/boot/boot.c
	@$(EFI_C) $(EFI_LINKFLAGS) -o bin/BOOTX64.EFI obj/boot/boot.obj

# Kernel components
$(KERNEL_TARGETS):
	@$(CXX) -c $(SRC_LOC)/kernel/$@.cpp $(CFLAGS) $(INCLUDES) -o ./obj/$(lastword $(subst /, ,$@)).obj
	@echo "CXX $@.cpp"

# Lib components
$(LIB_TARGETS):
	@$(CXX) -c $(SRC_LOC)/lib/$@.cpp $(CFLAGS) $(INCLUDES) -o ./obj/$(lastword $(subst /, ,$@)).obj
	@echo "CXX $@.cpp"

$(ASM_TARGETS):
	@$(NASM) $(NASMFLAGS) $(SRC_LOC)/$@.s -o ./obj/$(lastword $(subst /, ,$@))_s.obj
	@echo "ASM $@.s"

$(PROC_TARGETS):
	@$(MAKE) -C $(SRC_LOC)/proc/$@
	@cp $(SRC_LOC)/proc/$@/bin/$@.elf $(SRC_LOC)/fs/Epoch-RAM-Disk-Generator/cmake-build-debug
	@rm -f $(SRC_LOC)/fs/Epoch-RAM-Disk-Generator/cmake-build-debug/initrd.ext2
	@cd $(SRC_LOC)/fs/Epoch-RAM-Disk-Generator/cmake-build-debug &&\
	./Epoch_RAM_Disk_Generator.exe &&\
	cd ../../../../
	@cp $(SRC_LOC)/fs/Epoch-RAM-Disk-Generator/cmake-build-debug/initrd.ext2 ./img

# Final link command
link:
	@$(CC) $(LDFLAGS) ./obj/$(lastword $(subst /, ,*)).obj -o ./bin/$(NAME).elf
	@echo "Compile finished: $(NAME).elf"
	bash.exe bashscript.sh

exec:
	qemu-system-x86_64.exe -smp cores=4,threads=1,sockets=1 -m 8G -serial stdio -L OVMF/ -bios OVMF-pure-efi.fd -cdrom img/epochx64.iso