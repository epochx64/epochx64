#ifndef KERNEL_H
#define KERNEL_H

#include <defs/int.h>
#include <asm/asm.h>
#include <mem.h>
#include <kernel/graphics.h>
#include <kernel/interrupt.h>
#include <kernel/acpi.h>
#include <kernel/log.h>

extern "C" void KernelMain(KERNEL_BOOT_INFO*);

namespace kernel
{

}
#endif