//------------------------------------------------------------------------------
//
//
// defines.h
//                                                					  26.08.2021
//
//------------------------------------------------------------------------------

#ifndef INC1_DEFINES_H_
#define INC1_DEFINES_H_

#include "main.h"

#define BOOL uint8_t

enum{
	FALSE = 0,
	TRUE = 1
};

#define ON TRUE
#define OFF FALSE

//#define HIBYTE_(x) (x >> 8)
//#define LOBYTE_(x) (x & 0xFF)
#define HIBYTE(x) ((uint8_t*)(&x))[1]
#define LOBYTE(x) ((uint8_t*)(&x))[0]

#define HIWORD(x) ((uint16_t*)(&x))[1]
//#define LOWORD(x) ((uint16_t*)(&x))[0]
#define LOWORD(x) (x)

typedef uint16_t WORD;

#define BYTE uint8_t

//#define NULL 0

#define __enter_critical() {uint32_t flag; flag = __get_PRIMASK();__disable_irq();
#define __exit_critical()  __set_PRIMASK(flag);}

#define TESTBIT(x,y) ((x & (1 << y)) != 0)

#endif /* INC1_DEFINES_H_ */
