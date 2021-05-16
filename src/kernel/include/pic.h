#ifndef _PIC_H
#define _PIC_H

#include <typedef.h>
#include <asm/asm.h>
#include <log.h>

extern UINT8 MouseID;
extern UINT8 MousePacketSize;

void KeInitPS2();
void KeInitPIC();

UINT8 KePS2Read();
void KePS2Write(UINT8 Port, UINT8 Value);

#endif
