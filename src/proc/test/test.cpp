#include "epoch.h"

void gfxroutine(scheduler::TASK_ARG *TaskArgs)
{
    double x = *((double*)TaskArgs[0]);
    double y = *((double*)TaskArgs[1]);
    int radius = *((int*)TaskArgs[2]);

    graphics::Color c(128, 192, 128, 0);

    while (true)
    {
        using namespace graphics;
        using namespace kernel;
        using namespace math;

        double divisions = 120.0;
        double dtheta = 2 * PI / divisions;

        //  Draw a fat circle
        for (double theta = 0.0; theta < 2*PI; theta += dtheta) {
            for (int j = 0; j < radius; j++)
                PutPixel(RoundDouble<UINT64>(x + j*cos(theta)),RoundDouble<UINT64>(y + j*sin(theta)),
                         &(KernelDescriptor->GOPInfo), c);
        }

        c.u8_R += 10;
        c.u8_G += 20;
        c.u8_B += 10;
    }
}

int main()
{
    log::kout << "HELLO WORLD FROM test.elf    \n";

    auto T = new scheduler::Task(
            (UINT64)&gfxroutine,
            true,
            new TASK_ARG[3] {
                    (TASK_ARG)(new double(600.0 + 42.0)),
                    (TASK_ARG)(new double(140.0)),
                    (TASK_ARG)(new int(16))
            });

    scheduler::Scheduler::ScheduleTask(T);

    return 0;
}