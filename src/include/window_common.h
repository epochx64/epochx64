#ifndef WINDOW_COMMON_H
#define WINDOW_COMMON_H

typedef UINT64 WINDOW_TYPE;
typedef UINT64 WINDOW_STATE;

#define WINDOW_STATE_FOCUED     0x0001
#define WINDOW_STATE_MINIMIZED  0x0002
#define WINDOW_STATE_BACKGROUND 0x0004

#define WINDOW_TYPE_TERMINAL    0x0001

typedef struct
{
    WINDOW_TYPE type;
    WINDOW_STATE state;
    HANDLE handle;

    /* Window dimensions */
    UINT64 height;
    UINT64 width;

    /* Window position */
    UINT64 x;
    UINT64 y;

    /* Pointer to STDOUT struct */
    UINT64 pStdout;
    UINT64 pStdin;

    /* Called when a keyboard or mouse event occurs */
    void (*KeyboardEventCallback)(UINT8 data, void *state);
    void (*MouseEventCallback)(UINT8 data, void *state);

    /* Pointer to another struct e.g. TERMINAL_PROPERTIES */
    UINT64 pSecondaryProperties;
} DWM_WINDOW_PROPERTIES;

typedef HANDLE (*DWM_CREATE_WINDOW)(DWM_WINDOW_PROPERTIES *properties);
typedef void (*DWM_KEYBOARD_EVENT)(UINT8 data);
typedef void (*DWM_MOUSE_EVENT)(UINT8 data, UINT64 mousePacketSize);

typedef struct
{
    /* The dual buffer of the GOP framebuffer */
    FRAMEBUFFER_INFO frameBufferInfo;
    UINT64 frameBufferSize;
    UINT64 refreshRate;

    /* DWM function list */
    DWM_CREATE_WINDOW DwmCreateWindow;
    DWM_KEYBOARD_EVENT DwmKeyboardEvent;
    DWM_MOUSE_EVENT DwmMouseEvent;

} DWM_DESCRIPTOR;


#endif
