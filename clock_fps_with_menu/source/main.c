
#include "global.h"
#include "ov.h"

FS_archive  sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle      fsUserHandle = 0;
u32         IoBasePad;

u32         g_clockMode = 2; // 0 = none, 1 = Clock Only, 2 = Clock + Battery
u32         g_fpsCounter = 1; // 0 = disabled, 1 = both screen, 2 = top screen only, 3 = bottom screen only

Result  ptmuInit(void);
Result  APT_CheckNew3DS(bool* out);
int     DrawClockAndBattery(void);
int     DrawClockOnly(void);
int     DrawFPSCounter(u32 isBottom);
int     OverlayMenu(void);

/*
Overlay Callback.
isBottom: 1 for bottom screen, 0 for top screen.
addr: writable cached framebuffer virtual address.
addrB: right-eye framebuffer for top screen, undefined for bottom screen.
stride: framebuffer stride(pitch) in bytes, at least 240*bytes_per_pixel.
format: framebuffer format, see https://www.3dbrew.org/wiki/GPU/External_Registers for details.

NTR will invalidate data cache of the framebuffer before calling overlay callbacks. NTR will flush data cache after the callbacks were called and at least one overlay callback returns zero.

return 0 when the framebuffer was modified. return 1 when nothing in the framebuffer was modified.
*/

u32     OverlayCallback(u32 isBottom, u32 addr, u32 addrB, u32 stride, u32 format) 
{
    // Set settings for draw functions
    OvSettings(addr, addrB, stride, format, !isBottom);

    int framebufWasModified = 0;

    if (!isBottom)
    {
        // Check for menu
        framebufWasModified |= OverlayMenu();

        if (g_clockMode == 1)
            framebufWasModified |= DrawClockOnly();
        else if (g_clockMode == 2)
            framebufWasModified |= DrawClockAndBattery();       
    }

    if (g_fpsCounter == 1 || (g_fpsCounter == 2 && !isBottom) || (g_fpsCounter == 3 && isBottom))
        framebufWasModified |= DrawFPSCounter(isBottom);

    return (framebufWasModified);
}

int     main() 
{
    u32 retv;
    bool isNew3DS = false;
    
    initSharedFunc();

    // Init srv client and ptmu client for monitoring battery status.
    initSrv();
    ptmuInit();

    APT_CheckNew3DS(&isNew3DS);

    // Init pad
    if (isNew3DS)
        IoBasePad = 0xfffc2000;
    else
        IoBasePad = 0xfffc6000;

    // Register overlay callback.
    plgRegisterCallback(CALLBACK_OVERLAY, (void *)OverlayCallback, 0);

    return 0;
}

