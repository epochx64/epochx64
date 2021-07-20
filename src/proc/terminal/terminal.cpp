/**********************************************************************
 *  Includes
 *********************************************************************/

#include <epoch.h>
#include <ps2.h>

/**********************************************************************
 *  Local variables
 *********************************************************************/

/**********************************************************************
 *  Function declarations
 *********************************************************************/

/**********************************************************************
 *  @details Callback function for keyboard interrupt for a terminal window
 *  @param data - PS/2 scancode byte from the keyboard
 *  @param kbdState - Keyboard state descriptor
 *********************************************************************/
static void termKeyboardEventCallback(UINT8 data, KEYBOARD_STATE *kbdState)
{
    static char charMap[] = {
            /* Map with no shift press */
            0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
            97 + 16, 97 + 22, 97 + 4, 97 + 17, 97 + 19, 97 + 24, 97 + 20, 97 + 8, 97 + 14, 97 + 15,
            '[', ']', '\n', 0,
            97 + 0,97 + 18,97 + 3,97 + 5,97 + 6,97 + 7,97 + 9,97 + 10,97 + 11,
            ';', '\'', '`', 0, '\\',
            97 + 25, 97 + 23, 97 + 2, 97 + 21, 97 + 1, 97 + 13, 97 + 12,
            ',', '.', '/', 0, '*', 0, ' ',
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0',
            '.', 0, 0,

            /* Map with shift press */
            0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
            65 + 16, 65 + 22, 65 + 4, 65 + 17, 65 + 19, 65 + 24, 65 + 20, 65 + 8, 65 + 14, 65 + 15,
            '{', '}', '\n', 0,
            65 + 0,65 + 18,65 + 3,65 + 5,65 + 6,65 + 7,65 + 9,65 + 10,65 + 11,
            ':', '"', '~', 0, '|',
            65 + 25, 65 + 23, 65 + 2, 65 + 21, 65 + 1, 65 + 13, 65 + 12,
            '<', '>', '?', 0, '*', 0, ' ',
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0',
            '.', 0, 0,
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

    /* Check shift, alt, ctrl statuses */
    bool shift, alt, ctrl;
    if ( (STATE_PRESSED == kbdState->shift[0]) || (STATE_PRESSED == kbdState->shift[1]) ) shift = true;
    else shift = false;
    if ( (STATE_PRESSED == kbdState->alt[0]) || (STATE_PRESSED == kbdState->alt[1]) ) alt = true;
    else alt = false;
    if ( (STATE_PRESSED == kbdState->ctrl[0]) || (STATE_PRESSED == kbdState->ctrl[1]) ) ctrl = true;
    else ctrl = false;

    /* Check for E0 key */
    if (data == 0xE0)
    {
        isE0Key = true;
        return;
    }
    if (isE0Key)
    {
        isE0Key = false;

        return;
    }

    /* If bit 7 is set, this is a release event */
    if (data & 0x80)
    {
        return;
    }

    if (shift)
    {
        data += sizeof(charMap)/2;
    }
    data = charMap[data];

    if (data == 0)
    {
        return;
    }

    /* Write char to stdout and stdin */
    putchar(data, IOSTREAM_ID_STDOUT);
    putchar(data, IOSTREAM_ID_STDIN);
}

/**********************************************************************
 *  @details Main loop for the terminal
 *  @param None
 *********************************************************************/
static void termLoop()
{

}

int main()
{
     /* Create the terminal window */
     DWM_WINDOW_PROPERTIES properties = { 0 };
     properties.pStdout = (UINT64)getStdout();
     properties.pStdin = (UINT64)getStdin();
     properties.height = 400;
     properties.width = 600;
     properties.state = WINDOW_STATE_FOCUED;
     properties.type = WINDOW_TYPE_TERMINAL;
     properties.KeyboardEventCallback = (void(*)(UINT8, void *))termKeyboardEventCallback;
     properties.x = 200;
     properties.y = 100;
     DwmCreateWindow(&properties);
     printf("HELLO WORLD FROM TERMINAL.elf\n");

     //KeScheduleTask((UINT64)&testMsg, KeGetTime(), true, 0.5e9, 0);
     //KeSuspendCurrentTask();
}