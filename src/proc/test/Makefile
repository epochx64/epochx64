NAME = test

CXX = x86_64-elf-g++.exe
CFLAGS = -fPIE -ffreestanding -O2 -w -Wall -Wextra -fno-exceptions -fno-rtti -Wunused-parameter

CC = x86_64-elf-gcc.exe
LDFLAGS = -T ./linker.ld -pie -N -z max-page-size=4096 -ffreestanding -O2 -nostdlib -lgcc -Wunused-parameter

INCLUDES=-I ../../lib -I../../

SRC_LOC=../..

KERNEL_TARGETS = log graphics scheduler process
LIB_TARGETS = mem math/math elf/relocation ../fs/ext2
#ASM_TARGETS = lib/asm/asm kernel/interrupt

all: $(KERNEL_TARGETS) $(LIB_TARGETS) epoch $(NAME) link

# Kernel components
$(KERNEL_TARGETS):
	@$(CXX) -c $(SRC_LOC)/kernel/$@.cpp $(CFLAGS) $(INCLUDES) -o ./obj/$(lastword $(subst /, ,$@)).obj
	@echo "CXX $@.cpp"

# Lib components
$(LIB_TARGETS):
	@$(CXX) -c $(SRC_LOC)/lib/$@.cpp $(CFLAGS) $(INCLUDES) -o ./obj/$(lastword $(subst /, ,$@)).obj
	@echo "CXX $@.cpp"

#$(ASM_TARGETS):
#	@$(NASM) $(NASMFLAGS) $(SRC_LOC)/$@.s -o ./obj/$(lastword $(subst /, ,$@))_s.obj
#	@echo "ASM $@.s"

# Epoch components
epoch $(NAME):
	@$(CXX) -c ./$@.cpp $(CFLAGS) $(INCLUDES) -o ./obj/$@.obj
	@echo "CXX $@.cpp"

# Final link command
link:
	@$(CC) $(LDFLAGS) ./obj/$(lastword $(subst /, ,*)).obj -o ./bin/$(NAME).elf
	@echo "Compile finished: $(NAME).elf"
