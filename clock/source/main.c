
#include "global.h"
#include "font.h"

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



void ovDrawTranspartBlackRect(u32 addr, u32 stride, u32 format, int r, int c, int h, int w, u8 level) {
	format &= 0x0f;
	int  posC;
	for (posC = c; posC < c + w; posC ++ ) {
		if (format == 2) {
			u16* sp = (u16*)(addr + stride * posC + 240 * 2 - 2 * (r + h - 1));
			u16* spEnd = sp + h;
			while (sp < spEnd) {
				u16 pix = *sp;
				u16 r = (pix >> 11) & 0x1f;
				u16 g = (pix >> 5) & 0x3f;
				u16 b = (pix & 0x1f);
				pix = ((r >> level) << 11) | ((g >> level) << 5) | (b >> level);
				*sp = pix;
				sp++;
			}
		}
		else if (format == 1) {
			u8* sp = (u16*)(addr + stride * posC + 240 * 3 - 3 * (r + h - 1));
			u8* spEnd = sp +  3 * h;
			while (sp < spEnd) {
				sp[0] >>= level;
				sp[1] >>= level;
				sp[2] >>= level;
				sp += 3;
			}
		}
	}
}

void ovDrawPixel(u32 addr, u32 stride, u32 format, int posR, int posC, u32 r, u32 g, u32 b) {
	format &= 0x0f;	
	if (format == 2) {
		u16 pix =  ((r ) << 11) | ((g ) << 5) | (b );
		*(u16*)(addr + stride * posC + 240 * 2 -2 * posR) = pix;
	} else {
		u8* sp = (u16*)(addr + stride * posC + 240 * 3 - 3 * posR);
		sp[0] = b;
		sp[1] = g;
		sp[2] = r;
	}
}

void ovDrawRect(u32 addr, u32 stride, u32 format, int posR, int posC, int h, int w, u32 r, u32 g, u32 b) {
	int r_, c_;
	for (c_ = posC; c_ < posC + w; c_ ++) {
		for (r_ = posR; r_ < posR + h; r_ ++) {
			ovDrawPixel(addr, stride, format, r_, c_, r, g, b);
		}
	}
}

void ovDrawChar(u32 addr, u32 stride, u32 format, u8 letter,int y, int x, u32 r, u32 g, u32 b){

  int i;
  int k;
  int c;
  unsigned char mask;
  unsigned char* _letter;
  unsigned char l; 

	if ((letter < 32) || (letter > 127)) {
		letter = '?';
	}

  c=(letter-32)*8;

  for (i = 0; i < 8; i++){
    mask = 0b10000000;
    l = font[i+c];
    for (k = 0; k < 8; k++){
      if ((mask >> k) & l){
        ovDrawPixel(addr, stride, format, i+y, k+x ,r,g,b);
      }     
    }
  }
}

#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60


void drawWidget(int batteryLevel, u32 addr, u32 stride, u32 format, u8 hour, u8 min, int colOffset) {

	char buf[30];

	xsprintf(buf, "%02d:%02d", hour, min);



	int batval = 0;
	if (batteryLevel == 1) {
		batval = 2;
	} else if (batteryLevel == 2) {
		batval = 6;
	} else if (batteryLevel == 3) {
		batval = 9;
	} else if (batteryLevel == 4) {
		batval = 13;
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

	int i;
	for (i = 0; i < 5; i++) {
		ovDrawChar(addr, stride, format, buf[i], 11, colOffset + 4 + i * 8, 255, 255, 255);
	}
}


/*
Overlay Callback.
isBottom: 1 for bottom screen, 0 for top screen.
addr: writable cached framebuffer virtual address, should flush data cache after modifying.
addrB: right-eye framebuffer for top screen, undefined for bottom screen.
width: framebuffer stride in bytes, at least 240*bytes_per_pixel.
format: framebuffer format, see https://www.3dbrew.org/wiki/GPU/External_Registers for details.

return 0 on success. return 1 when nothing in framebuffer was modified.
*/

u32 overlayCallback(u32 isBottom, u32 addr, u32 addrB, u32 width, u32 format) {
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
	
		drawWidget(batteryLevel, addr, width, format, hour, min, 334);
		if ((addrB) && (addrB != addr))  {
			drawWidget(batteryLevel, addrB, width, format, hour, min, 330);
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

