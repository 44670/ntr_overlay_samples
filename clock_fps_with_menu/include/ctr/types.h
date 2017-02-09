#ifndef TYPES_H
#define TYPES_H

	#include <stdint.h>
	#include <stdbool.h>

	#define U64_MAX	UINT64_MAX

	typedef uint8_t u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;

	typedef int8_t s8;
	typedef int16_t s16;
	typedef int32_t s32;
	typedef int64_t s64;

	typedef volatile u8 vu8;
	typedef volatile u16 vu16;
	typedef volatile u32 vu32;
	typedef volatile u64 vu64;

	typedef volatile s8 vs8;
	typedef volatile s16 vs16;
	typedef volatile s32 vs32;
	typedef volatile s64 vs64;

	typedef u32 Handle;
	typedef s32 Result;
	typedef void (*ThreadFunc)(u32);

	
/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res)   ((res)>=0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)      ((res)<0)
/// Returns the level of a result code.
#define R_LEVEL(res)       (((res)>>27)&0x1F)
/// Returns the summary of a result code.
#define R_SUMMARY(res)     (((res)>>21)&0x3F)
/// Returns the module ID of a result code.
#define R_MODULE(res)      (((res)>>10)&0xFF)
/// Returns the description of a result code.
#define R_DESCRIPTION(res) ((res)&0x3FF)

#endif
