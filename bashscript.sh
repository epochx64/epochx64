#!/bin/bash

dd if=/dev/zero of=img/epochx64.img bs=1M count=31
mformat -i img/epochx64.img -t 1117 -h 16 -s 63 -F ::
mmd -i img/epochx64.img ::/EFI
mmd -i img/epochx64.img ::/EFI/BOOT
mcopy -i img/epochx64.img bin/BOOTX64.EFI ::/EFI/BOOT
mcopy -i img/epochx64.img bin/epochx64.elf ::/EFI
mcopy -i img/epochx64.img img/initrd.ext2 ::/EFI

cp img/epochx64.img iso
xorriso -as mkisofs -R -f -e epochx64.img -no-emul-boot -o img/epochx64.iso iso