#include "global.h"

static u8 *menuEntries[7] =
{
    "Disable Clock",
    "Display Clock only",
    "Display Clock + Battery",
    "Disable FPS Counter",
    "Display FPS Counter on both screens",
    "Display FPS Counter on Top Screen",
    "Display FPS COunter on Bottom Screen"
};

extern u32         g_clockMode; // 0 = none, 1 = Clock Only, 2 = Clock + Battery
extern u32         g_fpsCounter; // 0 = disabled, 1 = both screen, 2 = top screen only, 3 = bottom screen only

int     OverlayMenu(void)
{
    static int displayMenu = 0;
    static int selector = 0;

    u8 buf[200];
    int res = 0;
    
    if (getKey() == (BUTTON_L | BUTTON_X | BUTTON_Y))
    {
        waitKeyRelease();
        displayMenu = 1;
    }

    if (!displayMenu)
        return (0);

    res = showMenu("Overlay tools menu", 7, menuEntries, &selector);
    if (res >= 0 && res <= 2)
        g_clockMode = res;
    else if (res >= 3 && res <= 6)
        g_fpsCounter = res - 3;

    if (res == -1)
    {
        displayMenu = 0;
        selector = 0;
    }
    return (1);
}
