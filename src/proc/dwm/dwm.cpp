#include "dwm.h"

/**********************************************************************
 *  Global variables
 *********************************************************************/

DWM_DESCRIPTOR dwmDescriptor;

/**********************************************************************
 *  Local variables
 *********************************************************************/

/* Used for measuring the time delta between consecutive ticks of DwmGfxRoutine */
static KE_TIME prevTime = 0;
static KE_TIME currentTime = 0;

/* Incremented every time a new window is created */
static UINT64 handleCounter = 1;

/* Windows that get rendered are in this list */
static WINDOW_LIST_NODE *dwmWindowListHead = nullptr;
static WINDOW_LIST_NODE *dwmWindowListTail = nullptr;

/* Windows that are minimized are here */
static WINDOW_LIST_NODE *dwmInactiveWindowList = nullptr;

/**********************************************************************
 *  Function definitions
 *********************************************************************/

/**********************************************************************
 *  @details DWM API for creating a window
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
HANDLE DwmApiCreateWindow(DWM_WINDOW_PROPERTIES *properties)
{
    /* Generate handle */
    properties->handle = handleCounter++;

    /* Create a window list node */
    auto node = new WINDOW_LIST_NODE;
    node->pNext = nullptr;
    node->pPrev = nullptr;
    node->properties = new DWM_WINDOW_PROPERTIES;
    *(node->properties) = *properties;

    /* If the list is empty */
    if ((dwmWindowListHead == nullptr) || (dwmWindowListTail == nullptr))
    {
        dwmWindowListHead = node;
        dwmWindowListTail = node;

        return properties->handle;
    }

    /* Insert at top of list */
    if(properties->type == WINDOW_STATE_FOCUED)
    {
        node->pNext = dwmWindowListHead;
        dwmWindowListHead->pPrev = node;
        dwmWindowListHead = node;

        return properties->handle;
    }

    /* Insert second from the top */

    node->pPrev = dwmWindowListHead;

    /* If there is only one node */
    if (dwmWindowListHead->pNext == nullptr)
    {
        dwmWindowListTail = node;
    }
    else
    {
        node->pNext = dwmWindowListHead->pNext;
        dwmWindowListHead->pNext->pPrev = node;
    }

    dwmWindowListHead->pNext = node;

    return properties->handle;
}

/**********************************************************************
 *  @details Initializes DWM
 *********************************************************************/
void DwmInit()
{
    /* Frames per second */
    dwmDescriptor.refreshRate = 90;

    /* Initialize double buffer */
    dwmDescriptor.frameBufferSize = keSysDescriptor->gopInfo.Width * keSysDescriptor->gopInfo.Height * sizeof(UINT32);
    dwmDescriptor.frameBufferInfo = keSysDescriptor->gopInfo;
    dwmDescriptor.frameBufferInfo.pFrameBuffer = (UINT64)(KeSysMalloc(dwmDescriptor.frameBufferSize));

    /* Initialize function list */
    dwmDescriptor.dwmCreateWindow = &DwmApiCreateWindow;

    /* Set the system descriptor DWM pointer */
    keSysDescriptor->pDwmDescriptor = (UINT64)&dwmDescriptor;

    /* Clear the double buffer */
    for(UINT64 i = 0; i < dwmDescriptor.frameBufferSize; i += 8)
    {
        *(UINT64*)(dwmDescriptor.frameBufferInfo.pFrameBuffer + i) = 0;
    }

    /* Initialize timekeeping variables */
    currentTime = KeGetTime();
    prevTime = KeGetTime();
}

/**********************************************************************
 *  @details Writes the contents of dwmFrameBuffer to the GOP framebuffer
 *********************************************************************/
void DwmSwapBuffers(UINT64 *dstBuf, UINT64 *srcBuf, UINT64 size)
{
    for (UINT64 i = 0; i < (size/8); i++)
    {
        *(dstBuf + i) = *(srcBuf + i);
    }
}

/**********************************************************************
 *  @details Clears the double buffer before drawing
 *********************************************************************/
void DwmClearFramebuffer(UINT64 *dstBuf, UINT64 size)
{
    for (UINT64 i = 0; i < (size/8); i++)
    {
        *(dstBuf + i) = 0;
    }
}

/**********************************************************************
 *  @details Ran at the specified refresh rate
 *********************************************************************/
void DwmGfxRoutine()
{
    static KE_TIME totalTime = 0;

    /* Update time */
    currentTime = KeGetTime();
    auto deltaTime = currentTime - prevTime;
    totalTime = (totalTime + deltaTime) % (UINT64)1e9;
    prevTime = currentTime;

    /* Clear the screen */
    DwmClearFramebuffer((UINT64*)dwmDescriptor.frameBufferInfo.pFrameBuffer,dwmDescriptor.frameBufferSize);

    /* Draw all active windows */
    DwmDrawWindows();

    /* Write the dual buffer to the GOP framebuffer */
    DwmSwapBuffers((UINT64*)keSysDescriptor->gopInfo.pFrameBuffer,
                   (UINT64*)dwmDescriptor.frameBufferInfo.pFrameBuffer,
                   dwmDescriptor.frameBufferSize);
}

/**********************************************************************
 *  @details Draws all currently active windows
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
void DwmDrawWindows()
{
    /* TODO: Optimization? z-buffer maybe? */
    auto node = dwmWindowListTail;

    while (node != nullptr)
    {
        DwmDrawWindow(node->properties);
        node = node->pPrev;
    }
}

/**********************************************************************
 *  @details Draws a window and its border
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
void DwmDrawWindow(DWM_WINDOW_PROPERTIES *properties)
{
    /* TODO: Perform optimization */
    auto pFrameBuffer = (UINT32*)dwmDescriptor.frameBufferInfo.pFrameBuffer;
    auto pitch = dwmDescriptor.frameBufferInfo.Pitch;
    auto x = properties->x;
    auto y = properties->y;
    auto height = properties->height;
    auto width = properties->width;

    auto iter1 = (UINT32*)((UINT64)pFrameBuffer + (x-1)*4 + (y-30)*pitch);
    auto iter2 = (UINT32*)((UINT64)pFrameBuffer + (x-1)*4 + (y+height)*pitch);

    /* Draw top of window */
    for (UINT64 i = 0; i < WINDOW_ACCENT_HEIGHT; i++)
    {
        for (UINT64 j = 0; j < width; j++)
        {
            *(iter1 + j) = WINDOW_ACCENT_COLOR;
        }

        /* Draw a red rectangle */
        for (UINT64 j = width - WINDOW_ACCENT_CLOSE_WIDTH; j < width; j++)
        {
            *(iter1 + j) = WINDOW_ACCENT_CLOSE_COLOR;
        }
        iter1 += (pitch/4);
    }

    iter1 = (UINT32*)((UINT64)pFrameBuffer + (x-1)*4 + (y-31)*pitch);

    /* Draw window border */
    for (UINT64 i = 0; i < width + 1; i++)
    {
        *(iter1 + i) = WINDOW_BORDER_COLOR_FOCUSED;
        *(iter2 + i) = WINDOW_BORDER_COLOR_FOCUSED;
    }

    iter2 = iter1 + width;

    for (UINT64 i = 1; i < height + 1 + WINDOW_ACCENT_HEIGHT; i++)
    {
        *(iter1 + i*(pitch/4)) = WINDOW_BORDER_COLOR_FOCUSED;
        *(iter2 + i*(pitch/4)) = WINDOW_BORDER_COLOR_FOCUSED;
    }

    /* Draw inner window */
    if (properties->type == WINDOW_TYPE_TERMINAL)
    {
        DwmDrawTerminalWindow(properties);
    }
}

/**********************************************************************
 *  @details Places a char at the position in the terminal
 *  @param c - Character to print
 *  @details
 *********************************************************************/
void DwmTerminalPutChar(DWM_WINDOW_PROPERTIES *properties, char c, UINT64 line, UINT64 column)
{
    auto x = properties->x;
    auto y = properties->y;
    auto pitch = dwmDescriptor.frameBufferInfo.Pitch;

    auto startAddress = dwmDescriptor.frameBufferInfo.pFrameBuffer;
    startAddress +=  (y + 16*line)*pitch;
    startAddress += (x + 8*column + 2)*4;

    auto pixelIter = (UINT32*)startAddress;

    for(UINT64 i = 0; i < 16; i++)
    {
        UINT8 andMask = 0b10000000;
        auto fontRomByte  = (UINT8)(font_rom::buffer[ ((UINT64)c)*16 + i ]);

        /* Left to right, fill in pixel
         * row based on font_rom bits */
        for(UINT64 j = 0; j < 8; j++)
        {
            UINT32 color  = (fontRomByte & andMask )? 0xFFFFFFFF : TERMINAL_BACKGROUND_COLOR;
            andMask >>= (unsigned)1;

            *(pixelIter + j) = color;
        }

        pixelIter += (pitch/4);
    }
}

/**********************************************************************
 *  @details Terminal-specific draw function
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
void DwmDrawTerminalWindow(DWM_WINDOW_PROPERTIES *properties)
{
    UINT64 lines = (properties->height)/16;
    UINT64 columns = (properties->width)/8;
    STDOUT *stdout = (STDOUT*)properties->pStdout;
    UINT64 offsetIter = stdout->offset;

    auto x = properties->x;
    auto y = properties->y;
    auto pitch = dwmDescriptor.frameBufferInfo.Pitch;
    auto height = properties->height;
    auto width = properties->width;

    auto iter = (UINT32*)(dwmDescriptor.frameBufferInfo.pFrameBuffer + y*pitch + x*4);

    /* Fill background color */
    for (UINT64 i = 0; i < height; i++)
    {
        for (UINT64 j = 0; j < width - 1; j++)
        {
            *(iter + j) = TERMINAL_BACKGROUND_COLOR;
        }

        iter += (pitch/4);
    }

    /* Start at the end of the stdout, walk back until enough data loaded to fill whole terminal
     * TODO: This +2 -2 is a bad fix*/
    bool done = false;
    for (UINT64 i = 0; i < lines; i++)
    {
        for (UINT64 j = 0; j < columns-1; j++)
        {
            char c = *(stdout->pData + offsetIter);

            if (offsetIter == stdout->originOffset)
            {
                offsetIter -= 2;
                done = true;
                break;
            }

            if (offsetIter == 0) offsetIter = STDOUT_DEFAULT_SIZE - 1;
            else offsetIter--;

            if (c == '\n') break;
        }

        if (done) break;
    }

    offsetIter += 2;

    /* Step through offset, print the string */
    for (UINT64 i = 0; i < lines; i++)
    {
        for (UINT64 j = 0; j < columns-1; j++)
        {
            char c = *(stdout->pData + offsetIter);

            offsetIter = (offsetIter + 1) % STDOUT_DEFAULT_SIZE;
            if (offsetIter == stdout->offset) return;

            if (c == '\n') break;

            DwmTerminalPutChar(properties, c, i, j);
        }
    }
}

int main()
{
    DwmInit();
    log::kout.pFramebufferInfo = &dwmDescriptor.frameBufferInfo;

    KeScheduleTask((UINT64)&DwmGfxRoutine, KeGetTime(), true, 1e9/dwmDescriptor.refreshRate, 0);

    /* The GFX routine thread is going to continue indefinitely, requiring data from this executable.
     * Suspending it prevents the data from being unallocated. */
    KeSuspendCurrentTask();
}