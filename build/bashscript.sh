#!/bin/bash

dd if=/dev/zero of=img/epochx64.img bs=1M count=32
mformat -i img/epochx64.img -t 32768 -h 64 -s 32 -F ::
mmd -i img/epochx64.img ::/EFI
mmd -i img/epochx64.img ::/EFI/BOOT
mcopy -i img/epochx64.img bin/BOOTX64.EFI ::/EFI/BOOT
mcopy -i img/epochx64.img bin/epochx64.elf ::/EFI
mcopy -i img/epochx64.img img/initrd.ext2 ::/EFI

cp img/epochx64.img iso

#mkgpt -o img/epochx64.bin --image-size 4096 --part iso/epochx64.img --type system
mkisofs -R -f -no-emul-boot -o img/epochx64.iso -V EpochX64 -e epochx64.img iso