#include "dwm.h"

/**********************************************************************
 *  Local variables
 *********************************************************************/

static DWM_DESCRIPTOR dwmDescriptor;

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

/* Keyboard and mouse states */
static KEYBOARD_STATE dwmKeyboardState = { 0 };
static MOUSE_STATE dwmMouseState = { 0 };

static EVENT_HOOK_NODE *dwmEventHookLists[2] = { nullptr };

/* Cursor screen positions */
static UINT64 cursorX = 420;
static UINT64 cursorY  = 420;

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

    /* Terminal windows have a designated callback for keyboard interrupts */
    //if ((node->properties->type == WINDOW_TYPE_TERMINAL) && (node->properties->KeyboardEventCallback == nullptr))
    //{
    //    node->properties->KeyboardEventCallback = (void(*)(UINT8, void *))&DwmTerminalWindowKbdEvent;
    //}

    /* If the list is empty */
    if ((dwmWindowListHead == nullptr) || (dwmWindowListTail == nullptr))
    {
        dwmWindowListHead = node;
        dwmWindowListTail = node;

        return properties->handle;
    }

    /* Insert at top of list if the window is focused */
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
    /* Maximum refresh rate */
    dwmDescriptor.refreshRate = 144;

    /* Initialize double buffer */
    dwmDescriptor.frameBufferSize = keSysDescriptor->gopInfo.Width * keSysDescriptor->gopInfo.Height * sizeof(UINT32);
    dwmDescriptor.frameBufferInfo = keSysDescriptor->gopInfo;
    dwmDescriptor.frameBufferInfo.pFrameBuffer = (UINT64)(KeSysMalloc(dwmDescriptor.frameBufferSize));

    /* Initialize function list */
    dwmDescriptor.DwmCreateWindow = &DwmApiCreateWindow;
    dwmDescriptor.DwmKeyboardEvent = &DwmKeyboardEvent;
    dwmDescriptor.DwmMouseEvent = &DwmMouseEvent;

    /* Set the system descriptor DWM pointer */
    keSysDescriptor->pDwmDescriptor = (UINT64)&dwmDescriptor;

    /* Clear the double buffer */
    memset64(dwmDescriptor.frameBufferInfo.pFrameBuffer, dwmDescriptor.frameBufferSize, 0);

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
    memset64(dstBuf, size, 0);
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

    /* Draw mouse cursor */
    graphics::PutPixel(cursorX, cursorY, &(dwmDescriptor.frameBufferInfo), COLOR_WHITE);
    graphics::PutPixel(cursorX - 1, cursorY, &(dwmDescriptor.frameBufferInfo), COLOR_WHITE);
    graphics::PutPixel(cursorX + 1, cursorY, &(dwmDescriptor.frameBufferInfo), COLOR_WHITE);
    graphics::PutPixel(cursorX, cursorY - 1, &(dwmDescriptor.frameBufferInfo), COLOR_WHITE);
    graphics::PutPixel(cursorX, cursorY + 1, &(dwmDescriptor.frameBufferInfo), COLOR_WHITE);
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
    switch(properties->type)
    {
        case WINDOW_TYPE_TERMINAL: DwmDrawTerminalWindow(properties); break;
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
void DwmDrawTerminalWindow(DWM_WINDOW_PROPERTIES *properties) {
    UINT64 lines = (properties->height) / 16;
    UINT64 columns = (properties->width) / 8;
    STDOUT *stdout = (STDOUT *) properties->pStdout;
    UINT64 offsetIter = stdout->offset;

    auto x = properties->x;
    auto y = properties->y;
    auto pitch = dwmDescriptor.frameBufferInfo.Pitch;
    auto height = properties->height;
    auto width = properties->width;

    auto iter = (UINT32 *) (dwmDescriptor.frameBufferInfo.pFrameBuffer + y * pitch + x * 4);

    /* Fill background color */
    for (UINT64 i = 0; i < height; i++) {
        for (UINT64 j = 0; j < width - 1; j++) {
            *(iter + j) = TERMINAL_BACKGROUND_COLOR;
        }

        iter += (pitch / 4);
    }

    /* Start at the end of the stdout, walk back until enough data loaded to fill whole terminal
     * TODO: This +2 -2 is a bad fix*/
    bool done = false;
    for (UINT64 i = 0; i < lines; i++) {
        for (UINT64 j = 0; j < columns - 1; j++) {
            char c = *(stdout->pData + offsetIter);

            if (offsetIter == stdout->originOffset) {
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
    for (UINT64 i = 0; i < lines; i++) {
        for (UINT64 j = 0; j < columns - 1; j++) {
            char c = *(stdout->pData + offsetIter);

            offsetIter = (offsetIter + 1) % STDOUT_DEFAULT_SIZE;
            if (offsetIter == (stdout->offset + 1)) return;

            if (c == '\n') break;

            DwmTerminalPutChar(properties, c, i, j);
        }
    }
}

/**********************************************************************
 *  @details Called on every keyboard interrupt
 *  @param data - PS/2 scancode byte from the keyboard
 *********************************************************************/
void DwmKeyboardEvent(UINT8 data)
{
    static UINT64 keyStates[] = {
            0,
            offsetof(KEYBOARD_STATE, esc),
            offsetof(KEYBOARD_STATE, num[0][0]),
            offsetof(KEYBOARD_STATE, num[0][1]),
            offsetof(KEYBOARD_STATE, num[0][2]),
            offsetof(KEYBOARD_STATE, num[0][3]),
            offsetof(KEYBOARD_STATE, num[0][4]),
            offsetof(KEYBOARD_STATE, num[0][5]),
            offsetof(KEYBOARD_STATE, num[0][6]),
            offsetof(KEYBOARD_STATE, num[0][7]),
            offsetof(KEYBOARD_STATE, num[0][8]),
            offsetof(KEYBOARD_STATE, num[0][9]),
            offsetof(KEYBOARD_STATE, dash[0]),
            offsetof(KEYBOARD_STATE, equal),
            offsetof(KEYBOARD_STATE, backspace),
            offsetof(KEYBOARD_STATE, tab),
            offsetof(KEYBOARD_STATE, letter[16]),
            offsetof(KEYBOARD_STATE, letter[22]),
            offsetof(KEYBOARD_STATE, letter[4]),
            offsetof(KEYBOARD_STATE, letter[17]),
            offsetof(KEYBOARD_STATE, letter[19]),
            offsetof(KEYBOARD_STATE, letter[24]),
            offsetof(KEYBOARD_STATE, letter[20]),
            offsetof(KEYBOARD_STATE, letter[8]),
            offsetof(KEYBOARD_STATE, letter[14]),
            offsetof(KEYBOARD_STATE, letter[15]),
            offsetof(KEYBOARD_STATE, openBracket),
            offsetof(KEYBOARD_STATE, closeBracket),
            offsetof(KEYBOARD_STATE, enter[0]),
            offsetof(KEYBOARD_STATE, ctrl[0]),
            offsetof(KEYBOARD_STATE, letter[0]),
            offsetof(KEYBOARD_STATE, letter[18]),
            offsetof(KEYBOARD_STATE, letter[3]),
            offsetof(KEYBOARD_STATE, letter[5]),
            offsetof(KEYBOARD_STATE, letter[6]),
            offsetof(KEYBOARD_STATE, letter[7]),
            offsetof(KEYBOARD_STATE, letter[9]),
            offsetof(KEYBOARD_STATE, letter[10]),
            offsetof(KEYBOARD_STATE, letter[11]),
            offsetof(KEYBOARD_STATE, semicolon),
            offsetof(KEYBOARD_STATE, apostrophe),
            offsetof(KEYBOARD_STATE, tilda),
            offsetof(KEYBOARD_STATE, shift[0]),
            offsetof(KEYBOARD_STATE, backslash),
            offsetof(KEYBOARD_STATE, letter[25]),
            offsetof(KEYBOARD_STATE, letter[23]),
            offsetof(KEYBOARD_STATE, letter[2]),
            offsetof(KEYBOARD_STATE, letter[21]),
            offsetof(KEYBOARD_STATE, letter[1]),
            offsetof(KEYBOARD_STATE, letter[13]),
            offsetof(KEYBOARD_STATE, letter[12]),
            offsetof(KEYBOARD_STATE, comma),
            offsetof(KEYBOARD_STATE, period[0]),
            offsetof(KEYBOARD_STATE, slash[0]),
            offsetof(KEYBOARD_STATE, shift[1]),
            offsetof(KEYBOARD_STATE, asterisk),
            offsetof(KEYBOARD_STATE, alt[0]),
            offsetof(KEYBOARD_STATE, space),
            offsetof(KEYBOARD_STATE, caps),
            offsetof(KEYBOARD_STATE, func[0]),
            offsetof(KEYBOARD_STATE, func[1]),
            offsetof(KEYBOARD_STATE, func[2]),
            offsetof(KEYBOARD_STATE, func[3]),
            offsetof(KEYBOARD_STATE, func[4]),
            offsetof(KEYBOARD_STATE, func[5]),
            offsetof(KEYBOARD_STATE, func[6]),
            offsetof(KEYBOARD_STATE, func[7]),
            offsetof(KEYBOARD_STATE, func[8]),
            offsetof(KEYBOARD_STATE, func[9]),
            offsetof(KEYBOARD_STATE, numLock),
            offsetof(KEYBOARD_STATE, scrollLock),
            offsetof(KEYBOARD_STATE, num[1][7]),
            offsetof(KEYBOARD_STATE, num[1][8]),
            offsetof(KEYBOARD_STATE, num[1][9]),
            offsetof(KEYBOARD_STATE, dash[1]),
            offsetof(KEYBOARD_STATE, num[1][4]),
            offsetof(KEYBOARD_STATE, num[1][5]),
            offsetof(KEYBOARD_STATE, num[1][6]),
            offsetof(KEYBOARD_STATE, plus),
            offsetof(KEYBOARD_STATE, num[1][1]),
            offsetof(KEYBOARD_STATE, num[1][2]),
            offsetof(KEYBOARD_STATE, num[1][3]),
            offsetof(KEYBOARD_STATE, num[1][0]),
            offsetof(KEYBOARD_STATE, period[1]),
            offsetof(KEYBOARD_STATE, func[10]),
            offsetof(KEYBOARD_STATE, func[11]),
    };

    static UINT64 e0KeyStates[] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            offsetof(KEYBOARD_STATE, ctrl[1]), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            offsetof(KEYBOARD_STATE, slash[2]), 0, 0, offsetof(KEYBOARD_STATE, alt[1]), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
            offsetof(KEYBOARD_STATE, home), offsetof(KEYBOARD_STATE, arrowUp),
            offsetof(KEYBOARD_STATE, pageUp), 0,
            offsetof(KEYBOARD_STATE, arrowLeft), 0,
            offsetof(KEYBOARD_STATE, arrowRight), 0,
            offsetof(KEYBOARD_STATE, end),
            offsetof(KEYBOARD_STATE, arrowDown),
            offsetof(KEYBOARD_STATE, pageDown),
            offsetof(KEYBOARD_STATE, insert),
    };

    static bool isE0Key = false;

    /*
     * TODO: Typing on keyboard and using mouse simultaneously will sometimes
     *       cause generated keyboard/mouse IRQs to get handled by the wrong respective ISR
     *       Appears to only happen in QEMU
     */

    if (data == 0xE0)
    {
        isE0Key = true;
    }
    else
    {
        /* If bit 7 is set, this is a release event */
        UINT64 offset = data - (data & 0x80);

        /* Get key state pointers */
        auto keyState = (STATE*)((UINT64)&dwmKeyboardState + keyStates[offset]);
        auto e0KeyState = (STATE*)((UINT64)&dwmKeyboardState + e0KeyStates[offset]);

        /* Set the key states in the corresponding structure */
        if (isE0Key)
        {
            *e0KeyState = (data & 0x80)? STATE_RELEASED : STATE_PRESSED;
            isE0Key = false;
        }
        else *keyState = (data & 0x80)? STATE_RELEASED : STATE_PRESSED;
    }

    /* Pass this event to everything that has it hooked */
    for (EVENT_HOOK_NODE *node = dwmEventHookLists[KEYBOARD_EVENT]; node != nullptr; node = node->pNext)
    {
        KeScheduleTask((UINT64)node->EventCallBack, KeGetTime(), false, 0, 2, data, &dwmKeyboardState);
    }

    /* Pass this event to the currently focused window */
    auto KeyboardEvent = dwmWindowListHead->properties->KeyboardEventCallback;
    if (KeyboardEvent != nullptr)
    {
        KeScheduleTask((UINT64)KeyboardEvent, KeGetTime(), false, 0, 2, data, &dwmKeyboardState);
    }
}

/**********************************************************************
 *  @details Called on every mouse interrupt
 *  @param data - PS/2 scancode byte from the mouse
 *********************************************************************/
void DwmMouseEvent(UINT8 data, UINT64 mousePacketSize)
{
    static UINT8 mouseCycle{0};
    static UINT8 mouseData[3];

    /* Mouse sends 3 separate interrupts per event */
    mouseData[mouseCycle] = data;

    /* Once we have all the data */
    if(mouseCycle == 2)
    {
        cursorX = cursorX + ((int)mouseData[1] - (int)(((UINT16)mouseData[0] << 4) & 0x0100));
        cursorY = cursorY - ((int)mouseData[2] - (int)(((UINT16)mouseData[0] << 3) & 0x0100));
    }

    mouseCycle = (mouseCycle + 1) % mousePacketSize;
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