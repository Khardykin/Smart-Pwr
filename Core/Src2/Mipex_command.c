/*
 * Mipex_command.c
 *
 *
 */
#include "main.h"
#include "defines.h"
#include "modbus.h"
#include "Mipex_command.h"
#include "modbus_lpuart.h"
#include "ring_buffer.h"

#include "device.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

//==============================================================================
static void 			Mipex_TXEmpty_Callback(void);
static void 			Mipex_RXNEmpty_Callback(void);

static ErrorStatus 		Mipex_parcing_commmand(CommandMipexType command);
static void 			transmit_UART(char* data, uint8_t len);
static void 			transmit_UART_IT(char* data, uint8_t len);
static uint8_t 			receive_UART(void);

#define BUF_SIZE 100
static uint8_t 			tx_buffer[BUF_SIZE];
static uint8_t 			rx_buffer[BUF_SIZE];
static CommandMipexType command_for_IT;
static uint16_t 		Mipexstate;

ring_buffer 			Mipex_ring_buff_tx;
ring_buffer 			Mipex_ring_buff_rx;

ErrorStatus 			Mipex_status_parcing = ERROR;

uint16_t 				TimerRxMipex;		// Таймер на отклик Mipex
uint8_t 				Mipex_repeater = 0;	// Байт повторителя, 1 вкл, 0 выкл
uint16_t				Password = 0;		// Пароль для доступа в MIPEX по умолчанию
//==============================================================================
void Mipex_init_buff(void){
  Mipex_ring_buff_tx = ring_buffer_init(tx_buffer, BUF_SIZE);
  Mipex_ring_buff_rx = ring_buffer_init(rx_buffer, BUF_SIZE);
}

void Mipex_Init(void){
	Mipex_init_buff();
	for(uint8_t count = 0; (count < 5) && (Mipex_status_parcing == ERROR); count++){
		Mipex_transmit_commmand(COMMAND_OEM_XXXX);
	}
	Mipex_transmit_commmand(COMMAND_SRAL);
//	Mipex_transmit_commmand(COMMAND_AZERO);
//	Mipex_transmit_commmand(COMMAND_AZERO_ON);
}

void Mipex_Restart(void){
	Mipex_transmit_commmand(COMMAND_INIT);
}

//==============================================================================
void Mipex_UART_IRQHandler(void){
	if(LL_LPUART_IsEnabledIT_TXE(MIPEX_UART) && LL_LPUART_IsActiveFlag_TXE(MIPEX_UART)){
		Mipex_TXEmpty_Callback();
	}

	if(LL_LPUART_IsEnabledIT_RXNE(MIPEX_UART) && LL_LPUART_IsActiveFlag_RXNE(MIPEX_UART)){
		Mipex_RXNEmpty_Callback();
	}

	if(LL_LPUART_IsActiveFlag_ORE(MIPEX_UART)){
		LL_LPUART_ClearFlag_ORE(MIPEX_UART);
	}
}

static void Mipex_TXEmpty_Callback(void){
  if(ring_buffer_is_empty(&Mipex_ring_buff_tx)){
    LL_LPUART_DisableIT_TXE(MIPEX_UART);
  }else{
    LL_LPUART_TransmitData8(MIPEX_UART, ring_buffer_get(&Mipex_ring_buff_tx));
  }
}

static void Mipex_RXNEmpty_Callback(void){
	uint8_t data;
	data = LL_LPUART_ReceiveData8(MIPEX_UART);
	ring_buffer_put(&Mipex_ring_buff_rx, data);
	if(data == 0x0D){
		Mipex_status_parcing = Mipex_parcing_commmand(command_for_IT);
	}
}

//==============================================================================
static uint8_t Mipex_crc(uint8_t *buf, int len){
	uint8_t crc = (uint8_t)buf[0];
	for (int pos = 1; pos < len; pos++){
		crc ^= (uint8_t)buf[pos];
	}
	return crc;
}

static int strcompare(const char* str1, const char* str2, uint8_t len){
    uint16_t i = 0;
    for(i = 0; str1[i] && str2[i] && i < len; i++)
	{
		if(str1[i] != str2[i]){
			return 0;
		}
	}
    if(i != len){
    	return 0;
    }
    return i;
}

static ErrorStatus Mipex_parcing_commmand(CommandMipexType command){
	ErrorStatus status = SUCCESS;
	uint8_t rc_buf[20];
	uint8_t len_rc_buf = 0;
	for(uint8_t i = 0; !ring_buffer_is_empty(&Mipex_ring_buff_rx); i++){
		if(Mipex_repeater){
			mbs_pkt_tx[i] = ring_buffer_get(&Mipex_ring_buff_rx);
		}
		else{
			rc_buf[i] = ring_buffer_get(&Mipex_ring_buff_rx);
			len_rc_buf++;
		}
	}

	if(Mipex_repeater){
		LL_USART_DisableDirectionRx(MBS_LPUART);
		LL_USART_EnableIT_TXE(MBS_LPUART);
	}
	else{
		if(strcompare((char*)rc_buf, "OEM", 3)){

		}
		else if(strcompare((char*)rc_buf, "USER", 4)){
			if(command == COMMAND_OEM_XXXX){
				status = ERROR;
			}
		}
		else if(strcompare((char*)rc_buf, "INIT", 4)){
			if(!strcompare((char*)&rc_buf[5], "OK", 2)){
				status = ERROR;
			}
		}
		else if(strcompare((char*)rc_buf, "ZERO2", 5)){
			if(!strcompare((char*)&rc_buf[6], "OK", 2)){
				status = ERROR;
			}
		}
		else if(strcompare((char*)rc_buf, "CALB", 4)){
			if(!strcompare((char*)&rc_buf[9], "OK", 2)){
				status = ERROR;
			}
		}
		else if(strcompare((char*)rc_buf, "AZERO", 5)){

		}
		else if(Mipex_crc(rc_buf, 4) == rc_buf[4]){
			dev.RegInput.Value = (rc_buf[0]<<8) + rc_buf[1];
			Mipexstate = (rc_buf[2]<<8) + rc_buf[3];
			if(Mipexstate & MIPEX_STATE_ERR){
				dev.Status |= (1 << STATUS_BIT_SENSOR_ERROR);
			}
			else{
				dev.Status &= ~(1 << STATUS_BIT_SENSOR_ERROR);
			}
		}
		else if(command == COMMAND_ID){
			for(uint8_t i = 0; i < 5; i++){

			}
		}
		else if(command == COMMAND_RT){
			for(uint8_t i = 0; i < 5; i++){

			}
		}
		else if(command == COMMAND_SRAL){
			for(uint8_t i = 0; i < 4; i++){
				dev.Config.SNum[i] = (rc_buf[i*2]<<8) + rc_buf[i*2 + 1];
			}
		}
		else{
			status = ERROR;
		}

	}
	return status;
}

static void transmit_UART(char* data, uint8_t len){
	LL_LPUART_EnableIT_RXNE(MIPEX_UART);
	NVIC_DisableIRQ(USART2_IRQn);
	for(uint16_t i = 0; i < len; i++){
		while(!LL_LPUART_IsActiveFlag_TXE(MIPEX_UART));
		LL_LPUART_TransmitData8(MIPEX_UART, data[i]);
	}
	while(!LL_LPUART_IsActiveFlag_TXE(MIPEX_UART));
	LL_LPUART_TransmitData8(MIPEX_UART, 0x0D);
}

static void transmit_UART_IT(char* data, uint8_t len){
	LL_LPUART_EnableIT_RXNE(MIPEX_UART);
	NVIC_EnableIRQ(USART2_IRQn);
	for(uint16_t i = 0; i < len; i++){
		ring_buffer_put(&Mipex_ring_buff_tx, data[i]);
	}
	ring_buffer_put(&Mipex_ring_buff_tx, 0x0D);
	LL_LPUART_EnableIT_TXE(MIPEX_UART);
}

static uint8_t receive_UART(void){
	uint8_t data = 0;
	TimerRxMipex = TIME_RX_MIPEX;

	while(TimerRxMipex && (data != 0x0D)){
		if(LL_LPUART_IsActiveFlag_RXNE(MIPEX_UART)){
			data = LL_LPUART_ReceiveData8(MIPEX_UART);
			ring_buffer_put(&Mipex_ring_buff_rx, data);
			TimerRxMipex = TIME_RX_MIPEX;
		}
		if(LL_LPUART_IsActiveFlag_ORE(MIPEX_UART)){
			LL_LPUART_ClearFlag_ORE(MIPEX_UART);
		}
	}
	if(data == 0x0D){
		return 1;
	}
	return 0;
}

void Mipex_transmit_commmand(CommandMipexType command){
	Mipex_status_parcing = ERROR;
	uint8_t tr_buf[15];
	uint16_t data = 0;
	if(!Mipex_repeater){
		switch(command){
		case COMMAND_OEM_XXXX:
			data = Password;
			tr_buf[0] = 'O';tr_buf[1] = 'E';tr_buf[2] = 'M';tr_buf[3] = ' ';
			for(uint8_t i = 0; i < 4; i++){
				tr_buf[6 - i] = (data % 10) + 0x30;
				data /= 10;
			}
			transmit_UART((char*)tr_buf, 8);
			break;
		case COMMAND_ID:
			transmit_UART("ID?", 3);
			break;
		case COMMAND_RT:
			transmit_UART("RT?", 3);
			break;
		case COMMAND_SRAL:
			transmit_UART("SRAL?", 5);
			break;
		case COMMAND_AZERO:
			transmit_UART("AZERO?", 6);
			break;
		case COMMAND_AZERO_OFF:
			transmit_UART("AZERO OFF", 9);
			break;
		case COMMAND_AZERO_ON:
			transmit_UART("AZERO ON", 8);
			break;
		case COMMAND_INIT:
			transmit_UART("INIT", 4);
			break;
		case COMMAND_ZERO2:
			transmit_UART("ZERO2", 5);
			break;
		case COMMAND_CALB_AAAA:
			data = dev.Config.ValueCalib;
//			//--------------------------------------------------------------------
//			// Единица измерения
//			if(dev.Config.Unit & (1 << CFG_UNIT_VALUE_lel)){
//				data = (data*10)/dev.Config.ScaleKoef;
//			}
//			//--------------------------------------------------------------------
//			// Дискретность
//			uint8_t disc = (dev.Config.Unit & 0x03000)>>16;
//			if(disc > MIPEX_DISC){
//				for(uint8_t i = MIPEX_DISC; i < disc; i++){
//					data /= 10;
//				}
//			}
//			else{
//				for(uint8_t i = disc; i < MIPEX_DISC; i++){
//					data *= 10;
//				}
//			}
			tr_buf[0] = 'C';tr_buf[1] = 'A';tr_buf[2] = 'L';tr_buf[3] = 'B';tr_buf[4] = ' ';
			for(uint8_t i = 0; i < 4; i++){
				tr_buf[8 - i] = (data % 10) + 0x30;
				data /= 10;
			}
			transmit_UART((char*)tr_buf, 9);
			break;
		case COMMAND_DATAE2:
			transmit_UART("DATAE2", 6);
			break;
		default:
			break;
		}
		if(receive_UART()){
			Mipex_status_parcing = Mipex_parcing_commmand(command);
		}
	}
}

void Mipex_transmit_commmand_IT(CommandMipexType command){
	command_for_IT = command;
	Mipex_status_parcing = ERROR;
	uint8_t tr_buf[15];
	uint16_t data = 0;
	if(!Mipex_repeater){
		switch(command){
		case COMMAND_OEM_XXXX:
			data = Password;
			tr_buf[0] = 'O';tr_buf[1] = 'E';tr_buf[2] = 'M';tr_buf[3] = ' ';
			for(uint8_t i = 0; i < 4; i++){
				tr_buf[6 - i] = (data % 10) + 0x30;
				data /= 10;
			}
			transmit_UART_IT((char*)tr_buf, 8);
			break;
		case COMMAND_ID:
			transmit_UART_IT("ID?", 3);
			break;
		case COMMAND_RT:
			transmit_UART_IT("RT?", 3);
			break;
		case COMMAND_SRAL:
			transmit_UART_IT("SRAL?", 5);
			break;
		case COMMAND_INIT:
			transmit_UART_IT("INIT", 4);
			break;
		case COMMAND_ZERO2:
			transmit_UART_IT("ZERO2", 5);
			break;
		case COMMAND_CALB_AAAA:
			data = dev.Config.ValueCalib;
			tr_buf[0] = 'C';tr_buf[1] = 'A';tr_buf[2] = 'L';tr_buf[3] = 'B';tr_buf[4] = ' ';
			for(uint8_t i = 0; i < 4; i++){
				tr_buf[8 - i] = (data % 10) + 0x30;
				data /= 10;
			}
			transmit_UART_IT((char*)tr_buf, 9);
			break;
		case COMMAND_DATAE2:
			transmit_UART_IT("DATAE2", 6);
			break;
		default:
			break;
		}
	}
}

void Mipex_transmit_commmand_repeat(char* data, uint8_t len){
	if(Mipex_repeater){
		transmit_UART(data, len);
		receive_UART();
		Mipex_parcing_commmand(REPEATER);
	}
}

void Mipex_transmit_commmand_repeat_IT(char* data, uint8_t len){
	if(Mipex_repeater){
		transmit_UART_IT(data, len);
		command_for_IT = REPEATER;
	}
}

