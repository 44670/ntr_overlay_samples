
#include "global.h"
#include "ov.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

#define CALLBACK_OVERLAY (101)

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




#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60


void drawWidget(int batteryLevel, u32 addr, u32 stride, u32 format, u8 hour, u8 min, int colOffset) {

	char buf[30];

	



	int batval = 0;
	if (batteryLevel == 1) {
		batval = 2;
	} else if (batteryLevel == 2) {
		batval = 5;
	} else if (batteryLevel == 3) {
		batval = 8;
	} else if (batteryLevel == 4) {
		batval = 11;
	} else if (batteryLevel == 5) {
		batval = 16;
	}


	ovDrawTranspartBlackRect(addr, stride, format, 9, colOffset, 12, 62 + 4, 1);
	ovDrawRect(addr, stride, format, 11, colOffset + 46, 1, 16, 255, 255, 255);
	ovDrawRect(addr, stride, format, 17, colOffset + 46, 1, 16, 255, 255, 255);
	ovDrawRect(addr, stride, format, 11, colOffset + 46, 7, 1, 255, 255, 255);
	ovDrawRect(addr, stride, format, 11, colOffset + 61, 7, 1, 255, 255, 255);
	ovDrawRect(addr, stride, format, 11, colOffset + 46, 7, batval, 255, 255, 255);
	ovDrawRect(addr, stride, format, 13, colOffset + 62, 3, 1, 255, 255, 255);

	xsprintf(buf, "%02d:%02d", hour, min);
	ovDrawString(addr, stride, format, 400, 11, colOffset + 4, 255, 255, 255, buf);
}


/*
Overlay Callback.
isBottom: 1 for bottom screen, 0 for top screen.
addr: writable cached framebuffer virtual address, should flush data cache after modifying.
addrB: right-eye framebuffer for top screen, undefined for bottom screen.
stride: framebuffer stride(pitch) in bytes, at least 240*bytes_per_pixel.
format: framebuffer format, see https://www.3dbrew.org/wiki/GPU/External_Registers for details.

return 0 on success. return 1 when nothing in framebuffer was modified.
*/

u32 overlayCallback(u32 isBottom, u32 addr, u32 addrB, u32 stride, u32 format) {
	static u32 count = 0;
	static u8 batteryLevel = 0;

	u32 height = isBottom ? 320 : 400;
	if (isBottom == 0) {
		if (count == 0) {
			PTMU_GetBatteryLevel(&batteryLevel);
		}
		count ++;
		if (count > 100) {
			count = 0;
		}

		u64 timeInSeconds = osGetTime() / 1000;
		u64 dayTime = timeInSeconds % SECONDS_IN_DAY;
		u8 hour = dayTime / SECONDS_IN_HOUR;
		u8 min = (dayTime % SECONDS_IN_HOUR) / SECONDS_IN_MINUTE;
		u8 seconds = dayTime % SECONDS_IN_MINUTE;
	
		drawWidget(batteryLevel, addr, stride, format, hour, min, 332);
		if ((addrB) && (addrB != addr))  {
			drawWidget(batteryLevel, addrB, stride, format, hour, min, 328);
		}
		return 0;
	}
	return 1;
}

int main() {
	u32 retv;
	
	initSharedFunc();
	initSrv();
	ptmuInit();
	plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
	return 0;
}

