#include "epoch.h"

void testMsg()
{
    log::kout << "DWM\n";
}

int main()
{
    log::kout << "HELLO WORLDD FROM dwm.elf    \n";

    KeScheduleTask((UINT64)&testMsg, KeGetTime(), true, 1e9, 0);

    return 0;
}