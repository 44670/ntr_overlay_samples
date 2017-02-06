
#include "global.h"
#include "ov.h"

FS_archive sdmcArchive = { 0x9, (FS_path){ PATH_EMPTY, 1, (u8*)"" } };
Handle fsUserHandle = 0;

#define CALLBACK_OVERLAY (101)



#define TICKS_PER_MSEC (268123.480)


int frameCount[2];
u64 lastUpdatedTick[2];
int fps[2];


void drawWidget(int calculateFps, int isBottom, u32 addr, u32 stride, u32 format, u32 colOffset) {
	u32 height = isBottom ? 320 : 400;
	char buf[30];
	if (calculateFps) {
		frameCount[isBottom] ++;
		if (frameCount[isBottom] >= 64) {
			frameCount[isBottom] = 0;
			u64 tickNow = svc_getSystemTick();
			u64 diff = tickNow - lastUpdatedTick[isBottom] ;
			lastUpdatedTick[isBottom] = tickNow;
			fps[isBottom] = 64.0 / ((double) (diff) / TICKS_PER_MSEC / 1000.0) * 10.0;
			//fps[isBottom] = ((double)64.0) / ((double) (diff) / TICKS_PER_MSEC / 1000.0) * 10.0;
		}
	}


	ovDrawTranspartBlackRect(addr, stride, format, 9, colOffset, 12, 80 + 4, 1);
	xsprintf(buf, "fps: %d.%d", fps[isBottom] / 10, fps[isBottom] % 10);
	ovDrawString(addr, stride, format, height, 11, colOffset + 4, 255, 255, 255, buf);
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
	drawWidget(1, isBottom, addr, stride, format, 14);
	if ((isBottom == 0) && (addrB) && (addrB != addr))  {
		drawWidget(0, isBottom, addrB, stride, format, 10);
	}
	return 0;
}

int main() {
	u32 retv;
	
	initSharedFunc();
	plgRegisterCallback(CALLBACK_OVERLAY, (void*) overlayCallback, 0);
	return 0;
}

