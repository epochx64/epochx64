NAME = epochx64

# For compiling the kernel
INCLUDES=-I../src/include -I../src/kernel/include
SRC_LOC=../src

CXX = x86_64-elf-g++

CFLAGS =  -ffreestanding -O2 -w -Wall -Wextra -fno-exceptions -fno-rtti -Wunused-parameter

CC = x86_64-elf-gcc
LDFLAGS = -T $(SRC_LOC)/kernel/linker.ld -z max-page-size=4096 -ffreestanding -O2 -nostdlib -lgcc -Wunused-parameter

NASM = nasm
NASMFLAGS = -felf64

# For compiling the EFI bootloader
EFI_C = x86_64-w64-mingw32-gcc
EFI_CFLAGS = -ffreestanding
EFI_LINKFLAGS = -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main  -lgcc
EFI_INCLUDES = -I../src/include -I../src/include/uefi -I../src/include/uefi/X64 -I../src/include/elf -I../src/include/uefi/Protocol

KERNEL_TARGETS = kernel log interrupt acpi pic graphics scheduler
LIB_TARGETS = epstring mem math/math elf/relocation ../fs/ext2 io algorithm fault
ASM_TARGETS = lib/asm/asm kernel/interrupt kernel/kernel lib/ipc/ipc

#TODO:	ramdisk generator needs to be manually updated before adding new targets here
PROC_TARGETS = terminal dwm

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
	./Epoch_RAM_Disk_Generator &&\
	cd ../../../../
	@cp $(SRC_LOC)/fs/Epoch-RAM-Disk-Generator/cmake-build-debug/initrd.ext2 ./img

# Final link command
link:
	@$(CC) $(LDFLAGS) ./obj/$(lastword $(subst /, ,*)).obj -o ./bin/$(NAME).elf
	@echo "Compile finished: $(NAME).elf"
	bash bashscript.sh

exec:
	qemu-system-x86_64 -smp cores=4,threads=1,sockets=1 -m 8G -serial stdio -L OVMF/ -bios OVMF-pure-efi.fd -cdrom img/epochx64.iso
