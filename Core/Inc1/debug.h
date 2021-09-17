//------------------------------------------------------------------------------
//
//
//  debug.h
//                                                    	              19.07.2021
//
//------------------------------------------------------------------------------

#ifndef DEBUG_H_
#define DEBUG_H_

void debug_init(void);
void d_printf(const char *format, ...);

void Debug_UART_IRQHandler(void);

//#define PIN_DBG

//#define PIN_DBG1
//#define PIN_DBG2


#ifdef PIN_DBG1

#define DBG_OUT1_TGL LL_GPIO_TogglePin(DBG_OUT1_GPIO_Port, DBG_OUT1_Pin)
#define DBG_OUT1_HI LL_GPIO_SetOutputPin(DBG_OUT1_GPIO_Port, DBG_OUT1_Pin)
#define DBG_OUT1_LO LL_GPIO_ResetOutputPin(DBG_OUT1_GPIO_Port, DBG_OUT1_Pin)

#endif

#ifdef PIN_DBG2

#define DBG_OUT2_TGL LL_GPIO_TogglePin(DBG_OUT2_GPIO_Port, DBG_OUT2_Pin)
#define DBG_OUT2_HI LL_GPIO_SetOutputPin(DBG_OUT2_GPIO_Port, DBG_OUT2_Pin)
#define DBG_OUT2_LO LL_GPIO_ResetOutputPin(DBG_OUT2_GPIO_Port, DBG_OUT2_Pin)

#endif

#define DBG_UART USART2

#endif /* DEBUG_H_ */
