NAME = dwm

CXX = x86_64-elf-g++
CFLAGS = -fPIE -ffreestanding -O2 -w -Wall -Wextra -fno-exceptions -fno-rtti -Wunused-parameter

CC = x86_64-elf-gcc
LDFLAGS = -T ./linker.ld -pie -N -z max-page-size=4096 -ffreestanding -O2 -nostdlib -lgcc -Wunused-parameter

NASM = nasm
NASMFLAGS = -felf64

INCLUDES=-I ../../include -I../../kernel/include -I../include

SRC_LOC=../..

KERNEL_TARGETS = log graphics
LIB_TARGETS = mem math/math elf/relocation ../fs/ext2 io fault
ASM_TARGETS = lib/ipc/ipc lib/asm/asm

all: $(KERNEL_TARGETS) $(LIB_TARGETS) $(ASM_TARGETS) ../epoch $(NAME) link

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

# Epoch components
../epoch $(NAME):
	@$(CXX) -c ./$@.cpp $(CFLAGS) $(INCLUDES) -o ./obj/$(lastword $(subst /, ,$@)).obj
	@echo "CXX $@.cpp"

# Final link command
link:
	@$(CC) $(LDFLAGS) ./obj/$(lastword $(subst /, ,*)).obj -o ./bin/$(NAME).elf
	@echo "Compile finished: $(NAME).elf"
