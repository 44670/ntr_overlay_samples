#include "global.h"

#define __datetime_selector        (*(vu32*)0x1FF81000)
#define __datetime0 (*(volatile datetime_t*)0x1FF81020)
#define __datetime1 (*(volatile datetime_t*)0x1FF81040)

#define TICKS_PER_MSEC (268123.480)

typedef struct {
    u64 date_time;
    u64 update_tick;
    //...
} datetime_t;

static Handle ptmuHandle;

Result ptmuInit(void)
{
    Result res = srv_getServiceHandle(NULL, &ptmuHandle, "ptm:u");
    return res;
}

static datetime_t getSysTime(void) {
    u32 s1, s2 = __datetime_selector & 1;
    datetime_t dt;

    do {
        s1 = s2;
        if(!s1)
            dt = __datetime0;
        else
            dt = __datetime1;
        s2 = __datetime_selector & 1;
    } while(s2 != s1);

    return dt;
}

u64 osGetTime(void) {
    datetime_t dt = getSysTime();

    u64 delta = svc_getSystemTick() - dt.update_tick;

    return dt.date_time + (u64)(((double)(delta))/TICKS_PER_MSEC);
}



static inline u32 IPC_MakeHeader(u16 command_id, unsigned normal_params, unsigned translate_params)
{
    return ((u32) command_id << 16) | (((u32) normal_params & 0x3F) << 6) | (((u32) translate_params & 0x3F) << 0);
}

Result PTMU_GetBatteryLevel(u8 *out)
{
    Result ret=0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x7,0,0); // 0x70000

    if((ret = svc_sendSyncRequest(ptmuHandle)))return ret;

    *out = (u8)cmdbuf[2] & 0xFF;

    return (Result)cmdbuf[1];
}



static Result aptGetServiceHandle(Handle* aptuHandle)
{
    static const char* serviceName;
    static const char* const serviceNameTable[3] = {"APT:S", "APT:A", "APT:U"};

    if (serviceName)
        return srv_getServiceHandle(NULL, aptuHandle, (char *)serviceName);

    Result ret;
    int i;
    for (i = 0; i < 3; i ++)
    {
        ret = srv_getServiceHandle(NULL, aptuHandle, (char *)serviceNameTable[i]);
        if (R_SUCCEEDED(ret))
        {
            serviceName = serviceNameTable[i];
            break;
        }
    }

    return ret;
}


static inline int countPrmWords(u32 hdr)
{
    return 1 + (hdr&0x3F) + ((hdr>>6)&0x3F);
}

Result aptSendCommand(u32* aptcmdbuf)
{
    Handle aptuHandle;

    Result res = aptGetServiceHandle(&aptuHandle);
    if (R_SUCCEEDED(res))
    {
        u32* cmdbuf = getThreadCommandBuffer();
        memcpy(cmdbuf, aptcmdbuf, 4*countPrmWords(aptcmdbuf[0]));
        res = svc_sendSyncRequest(aptuHandle);
        if (R_SUCCEEDED(res))
        {
            memcpy(aptcmdbuf, cmdbuf, 4*16);//4*countPrmWords(cmdbuf[0])); // Workaround for Citra failing to emulate response cmdheaders
            res = aptcmdbuf[1];
        }
        svc_closeHandle(aptuHandle);
    }
    return res;
}

static Result APT_CheckNew3DS_System(bool* out)
{
    u32 cmdbuf[16];
    cmdbuf[0]=IPC_MakeHeader(0x102,0,0); // 0x1020000

    Result ret = aptSendCommand(cmdbuf);
    if (R_SUCCEEDED(ret))
        *out = cmdbuf[2] & 0xFF;

    return ret;
}

Result APT_CheckNew3DS(bool* out)
{
    static bool flagInit, flagValue;
    if (!flagInit)
    {
        *out = false;
        Result ret = APT_CheckNew3DS_System(&flagValue);
        if (R_FAILED(ret)) return ret;
        flagInit = true;
    }

    *out = flagValue;
    return 0;
}