#include <epoch.h>

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
    static UINT8 i = 0;
    printf("TEST %u\n", i++);
}

int main()
{
    printf("HELLO WORLD FROM TEST.elf\n");

    //KeScheduleTask((UINT64)&testMsg, KeGetTime(), true, 0.5e9, 0);

    //KeSuspendCurrentTask();
}