#ifndef KERNEL_H
#define KERNEL_H

#include <defs/int.h>
#include <asm/asm.h>
#include <mem.h>
#include <kernel/graphics.h>
#include <kernel/interrupt.h>
#include <kernel/acpi.h>
#include <kernel/log.h>
#include <boot/shared_boot_defs.h>
#include <kernel/gui/window.h>
#include <fs/ext2.h>
#include <elf/relocation.h>

namespace kernel
{

}

extern "C" void KernelMain(KERNEL_DESCRIPTOR*);

#endif