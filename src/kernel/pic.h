#ifndef _PIC_H
#define _PIC_H

#include <kernel/typedef.h>
#include <lib/asm/asm.h>
#include <kernel/log.h>

void InitPS2();
void InitPIT();

UINT8 PS2Read();
void PS2Write(UINT8 Port, UINT8 Value);

#endif
