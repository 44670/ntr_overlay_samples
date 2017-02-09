#include "global.h"
#include "ov.h"


#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60

#define TICKS_PER_SEC 0xFFB3D58
#define TICKS_PER_MIN 0x3BEE260A0UL
#define TICKS_IN_5_MIN 0x12BA6BE320UL

u64     osGetTime(void);
Result  PTMU_GetBatteryLevel(u8 *out);

void    GetTimeString(char *output)
{
    u64 timeInSeconds = osGetTime() / 1000;
    u64 dayTime = timeInSeconds % SECONDS_IN_DAY;
    u8 hour = dayTime / SECONDS_IN_HOUR;
    u8 min = (dayTime % SECONDS_IN_HOUR) / SECONDS_IN_MINUTE;
    u8 seconds = dayTime % SECONDS_IN_MINUTE;

    xsprintf(output, "%02d:%02d", hour, min);
}

int     DrawClockAndBattery(void) 
{
    static u32   batval = 0;
    static u64  tick = 0;

    if (svc_getSystemTick() >= tick)
    {
        u8 batteryLevel = 0;
        PTMU_GetBatteryLevel(&batteryLevel);
        tick = svc_getSystemTick() + TICKS_IN_5_MIN;

        if (batteryLevel == 1) 
        {
            batval = 2;
        } 
        else if (batteryLevel == 2) 
        {
            batval = 5;
        } 
        else if (batteryLevel == 3) 
        {
            batval = 8;
        } 
        else if (batteryLevel == 4) 
        {
            batval = 11;
        } 
        else if (batteryLevel == 5) 
        {
            batval = 16;
        }
    }

    char buf[30] = {0};

    GetTimeString(buf);

    //DrawBackground
    OvDrawTranspartBlackRect(332, 9, 66, 12, 1);

    // Draw battery
    OvDrawRect(378, 11, 16, 1, 255, 255, 255);
    OvDrawRect(378, 17, 16, 1, 255, 255, 255);
    OvDrawRect(378, 11, 1, 7, 255, 255, 255);
    OvDrawRect(393, 11, 1, 7, 255, 255, 255);
    OvDrawRect(378, 11, batval, 7, 255, 255, 255);
    OvDrawRect(394, 13, 1, 3, 255, 255, 255);

    // Draw clock
    OvDrawString(buf, 335, 11, 255, 255, 255);

    return (1);
}

int     DrawClockOnly(void) 
{

    char buf[30] = {0};

    GetTimeString(buf);

    //DrawBackground
    OvDrawTranspartBlackRect(342, 9, 48, 12, 1);
    // Draw clock
    OvDrawString(buf, 346, 11, 255, 255, 255);
    return (1);
}
