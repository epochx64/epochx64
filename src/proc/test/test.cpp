#include "epoch.h"

void gfxroutine(KE_TASK_ARG *TaskArgs)
{
    double x = *((double*)TaskArgs[0]);
    double y = *((double*)TaskArgs[1]);
    int radius = *((int*)TaskArgs[2]);

    graphics::Color c(128, 192, 128, 0);

    while (true)
    {
        using namespace graphics;
        using namespace math;

        double divisions = 120.0;
        double dtheta = 2 * PI / divisions;

        //  Draw a fat circle
        for (double theta = 0.0; theta < 2*PI; theta += dtheta) {
            for (int j = 0; j < radius; j++)
                PutPixel(RoundDouble<UINT64>(x + j*cos(theta)),RoundDouble<UINT64>(y + j*sin(theta)),
                         &(keSysDescriptor->gopInfo), c);
        }

        c.u8_R += 10;
        c.u8_G += 20;
        c.u8_B += 10;
    }
}

void testMsg()
{
    log::kout << "TEST\n";
}

int main()
{
    log::kout << "HELLO WORLDD FROM test.elf    \n";

    KeScheduleTask((UINT64)&testMsg, KeGetTime(), true, 1e9, 0);

    KeScheduleTask((UINT64)&gfxroutine, 0, false, 0,
            new KE_TASK_ARG[3] {
            (KE_TASK_ARG)(new double(600.0 + 42.0)),
            (KE_TASK_ARG)(new double(140.0)),
            (KE_TASK_ARG)(new int(16))
    });

    return 0;
}