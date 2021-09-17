//===============================================================================
//
//
//
//
//	debug.c
//                                                   				  17.08.2021
//
//==============================================================================

#include "main.h"
#include "debug.h"
#include "ring_buffer.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
///000
#define TX_BUF_SIZE 256
#ifdef TX_BUF_SIZE
	static uint8_t tx_buffer[TX_BUF_SIZE];
	ring_buffer dbg_ring_buff_tx;
#endif

void debug_init(void){
#ifdef TX_BUF_SIZE
  DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;
  DBGMCU->CR |= DBGMCU_CR_DBG_STOP;
  DBGMCU->CR |= DBGMCU_CR_DBG_STANDBY;

  dbg_ring_buff_tx = ring_buffer_init(tx_buffer, TX_BUF_SIZE);

  LL_LPUART_EnableIT_RXNE(DBG_UART);
#endif
}

void d_printf(const char *format, ...){
#ifdef TX_BUF_SIZE
  uint8_t aString[180];
  uint8_t len;

  //return;

  va_list args;

  va_start(args, format);
  vsnprintf((char*)aString,180,format,args);
  va_end(args);

  len = strlen((char*)aString);

  for(uint16_t i=0;i<len;i++)
    ring_buffer_put(&dbg_ring_buff_tx,aString[i]);

  LL_LPUART_EnableIT_TXE(DBG_UART);
#endif
}

//==============================================================================

void Debug_TXEmpty_Callback(void);
void Debug_RXNEmpty_Callback(void);

//==============================================================================

void Debug_UART_IRQHandler(void){
#ifdef TX_BUF_SIZE
	if(LL_LPUART_IsEnabledIT_TXE(DBG_UART) && LL_LPUART_IsActiveFlag_TXE(DBG_UART)){

		Debug_TXEmpty_Callback();
	}

	if(LL_LPUART_IsEnabledIT_RXNE(DBG_UART) && LL_LPUART_IsActiveFlag_RXNE(DBG_UART)){

		Debug_RXNEmpty_Callback();
	}

	if(LL_LPUART_IsActiveFlag_ORE(DBG_UART)){
		LL_LPUART_ClearFlag_ORE(DBG_UART);
	}
#endif
}

void Debug_TXEmpty_Callback(void){
#ifdef TX_BUF_SIZE
  if(ring_buffer_is_empty(&dbg_ring_buff_tx))
  {
    LL_LPUART_DisableIT_TXE(DBG_UART);

  }else{
    LL_LPUART_TransmitData8(DBG_UART,ring_buffer_get(&dbg_ring_buff_tx));
  }
#endif
}

//==============================================================================
/*
void debugMenu(uint8_t cmd){

	switch(cmd){

	case '0':
		CMD = CMD_NOP;
		Magnit = 0;
		break;

	case '1':
		CMD = CMD_DOWN;
		Magnit = 1;
		break;

	case '2':
		CMD = CMD_SET;
		Magnit = 1;
		break;

	case '3':
		CMD = CMD_UP;
		Magnit = 1;
		break;

	case 'q':
		CMD = CMD_DOWN_HOLD_2;
		Magnit = 3;
		break;

	case 'w':
		CMD = CMD_SET_HOLD_2;
		Magnit = 3;
		break;

	case 'e':
		CMD = CMD_UP_HOLD_2;
		Magnit = 3;
		break;

	case 'a':
		CMD = CMD_DOWN_HOLD_5;
		Magnit = 6;
		break;

	case 's':
		CMD = CMD_SET_HOLD_5;
		Magnit = 6;
		break;

	case 'd':
		CMD = CMD_UP_HOLD_5;
		Magnit = 6;
		break;

	case ',':
		if(stImmitation.wConcImmit >= 1)
			stImmitation.wConcImmit--;
		break;

	case '.':
		stImmitation.wConcImmit++;
		break;

	case 'l':
		if(stImmitation.wConcImmit >= 10)
			stImmitation.wConcImmit-=10;
		break;

	case ';':
		stImmitation.wConcImmit+=10;
		break;

	case 'p':
		if(stImmitation.wConcImmit >= 100)
			stImmitation.wConcImmit-=100;
		break;

	case '[':
		stImmitation.wConcImmit+=100;
		break;

	case '-':
		stImmitation.bEnable = FALSE;
		break;

	case '=':
		stImmitation.bEnable = TRUE;
		break;

	case '/':
		HourArhivStore();
		break;

	case 'x':
		xmodem_arh_trans_mode_start();
		break;


	default:
		break;
	}

	DisplayMainWndTO = Cnt10ms;

}
*/

uint8_t dbg_cmd;

void Debug_RXNEmpty_Callback(void){
#ifdef TX_BUF_SIZE
	dbg_cmd = LL_LPUART_ReceiveData8(DBG_UART);
#endif
//	debugMenu(dbg_cmd);
}
