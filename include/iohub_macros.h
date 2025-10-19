#ifndef _MACROS_H_
#define _MACROS_H_

#define ASSERT(xx)          		do{ if (!(xx)) log_assert(__FUNCTION__, __LINE__, #xx); }while(0)

/* -------------------------------------------------------------- */

#define PIN_INVALID                	~0

#define IS_BIT_SET(aVar, aBit)								((aVar) & (0x1 << (aBit)))
#define BIT_SET(aVar, aBit)									do { (aVar) |= (0x1 << aBit); }while(0)
#define BIT_CLEAR(aVar, aBit)								do { (aVar) &= ~(0x1 << aBit); }while(0)

#endif