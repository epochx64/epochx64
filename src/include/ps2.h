#ifndef PS2_H
#define PS2_H

#include <defs/int.h>

#define STATE_PRESSED  1
#define STATE_RELEASED 0

typedef UINT8 STATE;

typedef struct
{
    /* Repeatable keys */
    STATE esc;
    STATE func[12];

    STATE tilda;
    STATE num[2][10];   /* [0][]: left, [1][]: right */
    STATE dash;
    STATE equal;
    STATE backspace;

    STATE tab;
    STATE openBracket;
    STATE closeBracket;
    STATE backslash;
    STATE semicolon;
    STATE apostrophe;
    STATE enter[2];     /* 0: left, 1: right */
    STATE comma;
    STATE period[2];
    STATE slash[2];

    STATE letter[26];

    STATE del;
    STATE insert;
    STATE printScr;
    STATE home;
    STATE end;
    STATE pageUp;
    STATE pageDown;
    STATE pause;

    /* Non-repeatable keys */
    STATE caps;
    STATE shift[2]; /* 0: left, 1: right */
    STATE ctrl[2];  /* 0: left, 1: right */
    STATE alt[2];   /* 0: left, 1: right */
    STATE super[2]; /* 0: left, 1: right */

    STATE scrollLock;
    STATE numLock;
} KEYBOARD_STATE;

typedef struct
{
    STATE leftMouse;
    STATE rightMouse;
    STATE scrollUp;
    STATE scrollDown;
} MOUSE_STATE;

UINT8 UpdateKbdState(UINT8 scanCode, KEYBOARD_STATE *kbdState);
UINT8 UpdateMouseState(UINT8 scanCode, MOUSE_STATE *mouseState);

#endif
