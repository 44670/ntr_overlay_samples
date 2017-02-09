#include "global.h"
#include "ov.h"

#define TICKS_PER_MSEC (268123.480)


int DrawFPSCounter(u32 isBottom) 
{  
    static int frameCount[2];
    static u64 lastUpdatedTick[2];
    static int fps[2]; 
    char buf[30];

    frameCount[isBottom]++;
    if (frameCount[isBottom] >= 64) 
    {
        frameCount[isBottom] = 0;
        u64 tickNow = svc_getSystemTick();
        u64 diff = tickNow - lastUpdatedTick[isBottom];
        lastUpdatedTick[isBottom] = tickNow;
        fps[isBottom] = 64.0 / ((double)(diff) / TICKS_PER_MSEC / 1000.0) * 10.0;
    }


    OvDrawTranspartBlackRect(14, 9, 84, 12, 1);

    
    xsprintf(buf, "fps: %02d.%02d", fps[isBottom] / 10, fps[isBottom] % 10);
    OvDrawString(buf, 16, 11, 255, 255, 255);
    return (1);
}