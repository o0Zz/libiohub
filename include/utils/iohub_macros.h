#pragma once

#define IOHUB_PIN_INVALID                	~0

#define IOHUB_IS_BIT_SET(aVar, aBit)								((aVar) & (0x1 << (aBit)))
#define IOHUB_BIT_SET(aVar, aBit)									do { (aVar) |= (0x1 << aBit); }while(0)
#define IOHUB_BIT_CLEAR(aVar, aBit)								do { (aVar) &= ~(0x1 << aBit); }while(0)

#ifndef MIN
# 	define MIN(x, y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
# 	define MAX(x, y) ((x)>(y)?(x):(y))
#endif
