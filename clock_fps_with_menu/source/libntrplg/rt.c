
#include "global.h"
#include <ctr/SOC.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

void rtInitLock(RT_LOCK *lock) {
	lock->value = 0;
}

void rtAcquireLock(RT_LOCK *lock) {
	while(lock->value != 0) {
		svc_sleepThread(1000000);
	}
	lock->value = 1;
}

void rtReleaseLock(RT_LOCK *lock) {
	lock->value = 0;
}

u32 rtAlignToPageSize(u32 size) {
	return ((size / 0x1000) + 1) * 0x1000;
}

u32 rtGetPageOfAddress(u32 addr) {
	return (addr / 0x1000) * 0x1000;
}


u32 rtCheckRemoteMemoryRegionSafeForWrite(Handle hProcess, u32 addr, u32 size) {
	u32 ret;
	u32 startPage, endPage, page;
	
	startPage = rtGetPageOfAddress(addr);
	endPage = rtGetPageOfAddress(addr + size - 1);

	for (page = startPage; page <= endPage; page += 0x1000) {
		ret = protectRemoteMemory(hProcess, (void*) page, 0x1000);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

u32 rtSafeCopyMemory(u32 dst, u32 src, u32 size) {
	u32 ret;
	
	ret = rtCheckRemoteMemoryRegionSafeForWrite(0xffff8001, dst, size) ;
	if (ret != 0) {
		return ret;
	}
	ret = rtCheckRemoteMemoryRegionSafeForWrite(0xffff8001, src, size) ;
	if (ret != 0) {
		return ret;
	}
	memcpy((void*) dst, (void*) src, size);
	return 0;
}

#if USE_SOCKET

 
int rtRecvSocket(u32 sockfd, u8 *buf, int size)
{
	int ret, pos=0;
	int tmpsize=size;

	while(tmpsize)
	{
		if((ret = recv(sockfd, &buf[pos], tmpsize, 0))<=0)
		{
			if(ret<0)ret = SOC_GetErrno();
			if(ret == -EWOULDBLOCK)continue;
			return ret;
		}
		pos+= ret;
		tmpsize-= ret;
	}

	return size;
}

int rtSendSocket(u32 sockfd, u8 *buf, int size)
{
	int ret, pos=0;
	int tmpsize=size;

	while(tmpsize)
	{
		if((ret = send(sockfd, &buf[pos], tmpsize, 0))<0)
		{
			ret = SOC_GetErrno();
			if(ret == -EWOULDBLOCK)continue;
			return ret;
		}
		pos+= ret;
		tmpsize-= ret;
	}

	return size;
}

u16 rtIntToPortNumber(u16 x) {
	u8* buf;
	u8 tmp;
	
	buf = (void*)&x;
	tmp = buf[0];
	buf[0] = buf[1];
	buf[1] = tmp;
	return *((u16*)buf);
}

#endif	

u32 rtGetFileSize(u8* fileName) {
	u32 hFile, size, ret;
	u64 size64 ;

	FS_path testPath = (FS_path){PATH_CHAR, strlen(fileName) + 1, fileName};
	ret = FSUSER_OpenFileDirectly(fsUserHandle, &hFile, sdmcArchive, testPath, 7, 0);
	if (ret != 0) {
		nsDbgPrint("openFile failed: %08x\n", ret, 0);
		hFile = 0;
		goto final;
	}
	ret = FSFILE_GetSize(hFile, &size64);
	if (ret != 0) {
		nsDbgPrint("FSFILE_GetSize failed: %08x\n", ret, 0);
		goto final;
	}
	size = size64;

final:
	if (hFile != 0) {
		svc_closeHandle(hFile);
	}
	if (ret != 0) {
		return 0;
	}
	return size;
}

u32 rtLoadFileToBuffer(u8* fileName, u32* pBuf, u32 bufSize) {
	u32 ret;
	u32 hFile, size;
	u64 size64;
	u32 tmp;
	
	FS_path testPath = (FS_path){PATH_CHAR, strlen(fileName) + 1, fileName};
	ret = FSUSER_OpenFileDirectly(fsUserHandle, &hFile, sdmcArchive, testPath, 7, 0);
	if (ret != 0) {
		nsDbgPrint("openFile failed: %08x\n", ret, 0);
		hFile = 0;
		goto final;
	}
	
	ret = FSFILE_GetSize(hFile, &size64);
	if (ret != 0) {
		nsDbgPrint("FSFILE_GetSize failed: %08x\n", ret, 0);
		goto final;
	}

	size = size64;

	if (bufSize < size) {
		nsDbgPrint("rtLoadFileToBuffer: buffer too small\n");
		ret = -1;
		goto final;
	}

	ret = FSFILE_Read(hFile, &tmp, 0, (u32*)pBuf, size);
	if(ret != 0) {
		nsDbgPrint("FSFILE_Read failed: %08x\n", ret, 0);
		goto final;
	}

final:
	if (hFile != 0) {
		svc_closeHandle(hFile);
	}
	if (ret != 0) {
		return 0;
	}

	return size;
}


u32 rtGenerateJumpCode(u32 dst, u32* buf) {
	buf[0] = 0xe51ff004;
	buf[1] = dst;
	return 8;
}

void rtInitHook(RT_HOOK* hook, u32 funcAddr, u32 callbackAddr) {
	hook->model = 0;
	hook->isEnabled = 0;
	hook->funcAddr = funcAddr;

	rtCheckRemoteMemoryRegionSafeForWrite(getCurrentProcessHandle(), funcAddr, 8);
	memcpy(hook->bakCode, (void*) funcAddr, 8);
	rtGenerateJumpCode(callbackAddr, hook->jmpCode);
	memcpy(hook->callCode, (void*) funcAddr, 8);
	rtGenerateJumpCode(funcAddr + 8, &(hook->callCode[2]));
	rtFlushInstructionCache(hook->callCode, 16);
}

u32 rtFlushInstructionCache(void* ptr, u32 size) {
	return svc_flushProcessDataCache(0xffff8001, (u32)ptr, size);
}

void rtEnableHook(RT_HOOK* hook) {
	if (hook->isEnabled) {
		return;
	}
	memcpy((void*) hook->funcAddr, hook->jmpCode, 8);
	rtFlushInstructionCache((void*) hook->funcAddr, 8);
	hook->isEnabled = 1;
}

void rtDisableHook(RT_HOOK* hook) {
	if (!hook->isEnabled) {
		return;
	}
	memcpy((void*) hook->funcAddr, hook->bakCode, 8);
	rtFlushInstructionCache((void*) hook->funcAddr, 8);
	hook->isEnabled = 0;
}