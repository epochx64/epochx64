#ifndef _DWM_H
#define _DWM_H

/**********************************************************************
 *  Includes
 *********************************************************************/
#include <epoch.h>
#include <mem.h>
#include <math/math.h>
#include <window_common.h>
#include <font_rom.h>
#include <ps2.h>
#include <typedef.h>

/**********************************************************************
 *  Definitions
 *********************************************************************/

typedef struct WINDOW_LIST_NODE
{
    DWM_WINDOW_PROPERTIES *properties;
    WINDOW_LIST_NODE *pNext;
    WINDOW_LIST_NODE *pPrev;

} WINDOW_LIST_NODE;

enum EVENT_HOOK_LIST_ID
{
    KEYBOARD_EVENT = 0,
    MOUSE_EVENT = 1
};

typedef struct EVENT_HOOK_NODE
{
    void (*EventCallBack)(UINT8 data, void *state);
    EVENT_HOOK_NODE *pNext;
} EVENT_HOOK_NODE;

#define WINDOW_ACCENT_HEIGHT 30
#define WINDOW_ACCENT_COLOR 0xFF2A242A

#define WINDOW_BORDER_COLOR_FOCUSED 0xFFFFFFFF
#define WINDOW_BORDER_COLOR_UNFOCUSED WINDOW_ACCENT_COLOR

#define WINDOW_ACCENT_CLOSE_COLOR 0xFFFF1111
#define WINDOW_ACCENT_CLOSE_WIDTH 36

#define TERMINAL_BACKGROUND_COLOR 0xFF141014

/**********************************************************************
 *  Function declarations
 *********************************************************************/

/**********************************************************************
 *  @details DWM API for creating a window
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
HANDLE DwmApiCreateWindow(DWM_WINDOW_PROPERTIES *properties);

/**********************************************************************
 *  @details Initializes DWM
 *********************************************************************/
void DwmInit();

/**********************************************************************
 *  @details Writes the contents of dwmFrameBuffer to the GOP framebuffer
 *********************************************************************/
void DwmSwapBuffers(UINT64 *dstBuf, UINT64 *srcBuf, UINT64 size);

/**********************************************************************
 *  @details Clears the double buffer before drawing
 *********************************************************************/
void DwmClearFramebuffer(UINT64 *dstBuf, UINT64 size);

/**********************************************************************
 *  @details Ran at the specified refresh rate
 *********************************************************************/
void DwmGfxRoutine();

/**********************************************************************
 *  @details Draws all currently active windows
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
void DwmDrawWindows();

/**********************************************************************
 *  @details Draws a window and its border
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
void DwmDrawWindow(DWM_WINDOW_PROPERTIES *properties);

/**********************************************************************
 *  @details Terminal-specific draw function
 *  @param properties - Pointer to window properties struct
 *********************************************************************/
void DwmDrawTerminalWindow(DWM_WINDOW_PROPERTIES *properties);

/**********************************************************************
 *  @details Called on every keyboard interrupt
 *  @param data - PS/2 scancode byte from the keyboard
 *********************************************************************/
void DwmKeyboardEvent(UINT8 data);

/**********************************************************************
 *  @details Called on every mouse interrupt
 *  @param data - PS/2 scancode byte from the mouse
 *********************************************************************/
void DwmMouseEvent(UINT8 data, UINT64 mousePacketSize);

#endif
