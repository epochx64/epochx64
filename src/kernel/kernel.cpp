#include "kernel.h"

namespace kernel
{
    //  TODO:   make these pointers
    KERNEL_BOOT_INFO *KernelBootInfo;
    KERNEL_DESCRIPTOR KernelDescriptor;
}

/*
 * Temporary
 */
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

        double divisions = 2400.0;
        double dtheta = 2 * PI / divisions;

        //  Draw a fat circle
        for (double theta = 0.0; theta < 2*PI; theta += dtheta) {
            for (int j = 0; j < radius; j++)
                PutPixel(x + j*cos(theta), y + j*sin(theta), &(KernelBootInfo->FramebufferInfo), c);
        }

        c.u8_R += 10;
        c.u8_G += 20;
        c.u8_B += 10;
    }
}

void KernelMain(KERNEL_BOOT_INFO *KernelInfo)
{
    using namespace kernel;

    /*
     * Setup IDT, SSE, PIT, and PS2
     */
    KernelBootInfo = new KERNEL_BOOT_INFO;
    mem::copy((byte*)KernelInfo, (byte*)KernelBootInfo, sizeof(KERNEL_BOOT_INFO));

    {
        using namespace interrupt;
        using namespace ASMx64;
        using namespace PIC;

        EnableSSE();

        cli();
        FillIDT64();
        InitPS2();
        InitPIT();
    }

    /*
     *  So far this only initializes APIC timer
     */
    ACPI::InitACPI(KernelBootInfo, &(KernelDescriptor.KernelACPIInfo));

    /*
     * Setup a scheduler before the APIC timer is enabled
     * and add a couple of tasks
     */
    using namespace scheduler;
    {
        Scheduler0 = new scheduler::Scheduler(true, 0);

        Scheduler0->AddTask(new Task(
                (UINT64)&gfxroutine,
                true,
                new TASK_ARG[3] {
                        (TASK_ARG)(new double(200.0)),
                        (TASK_ARG)(new double(200.0)),
                        (TASK_ARG)(new int(100))
                }
        ));

        Scheduler0->AddTask(new Task(
                (UINT64)&gfxroutine,
                true,
                new TASK_ARG[3] {
                        (TASK_ARG)(new double(900.0)),
                        (TASK_ARG)(new double(300.0)),
                        (TASK_ARG)(new int(130))
                }
        ));
    }

    ASMx64::sti();

    /*
     * No code runs beyond here
     * This is now an idle task
     */
    ASMx64::hlt();

    /*
     * Test graphics routine (draws a circle)
     * TODO:   Double buffering
     */
    gfxroutine(new scheduler::TASK_ARG[3] {
        (TASK_ARG)(new double(500.0)),
        (TASK_ARG)(new double(500.0)),
        (TASK_ARG)(new int(100))
    });
}
